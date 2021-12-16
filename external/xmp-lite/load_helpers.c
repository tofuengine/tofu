/* Extended Module Player
 * Copyright (C) 1996-2021 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "load_helpers.h"

#include <ctype.h>
#include "common.h"
#include "loader.h"

char *libxmp_adjust_string(char *s)
{
	size_t i;

	for (i = 0; i < strlen(s); i++) {
		if (!isprint((unsigned char)s[i]) || ((uint8_t) s[i] > 127))
			s[i] = ' ';
	}

	while (*s && (s[strlen(s) - 1] == ' ')) {
		s[strlen(s) - 1] = 0;
	}

	return s;
}

static void check_envelope(struct xmp_envelope *env)
{
	/* Disable envelope if invalid number of points */
	if (env->npt <= 0 || env->npt > XMP_MAX_ENV_POINTS) {
		env->flg &= ~XMP_ENVELOPE_ON;
	}

	/* Disable envelope loop if invalid loop parameters */
	if (env->lps >= env->npt || env->lpe >= env->npt) {
		env->flg &= ~XMP_ENVELOPE_LOOP;
	}

	/* Disable envelope sustain if invalid sustain */
	if (env->sus >= env->npt || env->sue >= env->npt) {
		env->flg &= ~XMP_ENVELOPE_SUS;
	}
}

void libxmp_load_prologue(struct context_data *ctx)
{
	struct module_data *m = &ctx->m;
	int i;

	/* Reset variables */
	memset(&m->mod, 0, sizeof (struct xmp_module));
	m->rrate = PAL_RATE;
	m->c4rate = C4_PAL_RATE;
	m->volbase = 0x40;
	m->gvol = m->gvolbase = 0x40;
	m->vol_table = NULL;
	m->quirk = 0;
	m->read_event_type = READ_EVENT_MOD;
	m->period_type = PERIOD_AMIGA;
	m->comment = NULL;
	m->scan_cnt = NULL;

	/* Set defaults */
	m->mod.pat = 0;
	m->mod.trk = 0;
	m->mod.chn = 4;
	m->mod.ins = 0;
	m->mod.smp = 0;
	m->mod.spd = 6;
	m->mod.bpm = 125;
	m->mod.len = 0;
	m->mod.rst = 0;

#ifndef LIBXMP_CORE_DISABLE_IT
	m->xsmp = NULL;
#endif

	m->time_factor = DEFAULT_TIME_FACTOR;

	for (i = 0; i < 64; i++) {
		int pan = (((i + 1) / 2) % 2) * 0xff;
		m->mod.xxc[i].pan = 0x80 + (pan - 0x80) * m->defpan / 100;
		m->mod.xxc[i].vol = 0x40;
		m->mod.xxc[i].flg = 0;
	}
}

void libxmp_load_epilogue(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	int i, j;

    	mod->gvl = m->gvol;

	/* Sanity check for module parameters */
	CLAMP(mod->len, 0, XMP_MAX_MOD_LENGTH);
	CLAMP(mod->pat, 0, 257);   /* some formats have an extra pattern */
	CLAMP(mod->ins, 0, 255);
	CLAMP(mod->smp, 0, MAX_SAMPLES);
	CLAMP(mod->chn, 0, XMP_MAX_CHANNELS);

	/* Fix cases where the restart value is invalid e.g. kc_fall8.xm
	 * from http://aminet.net/mods/mvp/mvp_0002.lha (reported by
	 * Ralf Hoffmann <ralf@boomerangsworld.de>)
	 */
	if (mod->rst >= mod->len) {
		mod->rst = 0;
	}

	/* Sanity check for tempo and BPM */
	if (mod->spd <= 0 || mod->spd > 255) {
		mod->spd = 6;
	}
	CLAMP(mod->bpm, XMP_MIN_BPM, 255);

	/* Set appropriate values for instrument volumes and subinstrument
	 * global volumes when QUIRK_INSVOL is not set, to keep volume values
	 * consistent if the user inspects struct xmp_module. We can later
	 * set volumes in the loaders and eliminate the quirk.
	 */
	for (i = 0; i < mod->ins; i++) {
		if (~m->quirk & QUIRK_INSVOL) {
			mod->xxi[i].vol = m->volbase;
		}
		for (j = 0; j < mod->xxi[i].nsm; j++) {
			if (~m->quirk & QUIRK_INSVOL) {
				mod->xxi[i].sub[j].gvl = m->volbase;
			}
		}
	}

	/* Sanity check for envelopes
	 */
	for (i = 0; i < mod->ins; i++) {
		check_envelope(&mod->xxi[i].aei);
		check_envelope(&mod->xxi[i].fei);
		check_envelope(&mod->xxi[i].pei);
	}

	p->filter = 0;
	p->mode = XMP_MODE_AUTO;
	p->flags = p->player_flags;
	libxmp_set_player_mode(ctx);
}

int libxmp_prepare_scan(struct context_data *ctx)
{
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	int i, ord;

	if (mod->xxp == NULL || mod->xxt == NULL)
		return -XMP_ERROR_LOAD;
	ord = 0;
	while (ord < mod->len && mod->xxo[ord] >= mod->pat) {
		ord++;
	}

	if (ord >= mod->len) {
		mod->len = 0;
		return 0;
	}

	m->scan_cnt = (uint8_t **)calloc(mod->len, sizeof(uint8_t *));
	if (m->scan_cnt == NULL)
		return -XMP_ERROR_SYSTEM;

	for (i = 0; i < mod->len; i++) {
		int pat_idx = mod->xxo[i];
		struct xmp_pattern *pat;

		/* Add pattern if referenced in orders */
		if (pat_idx < mod->pat && !mod->xxp[pat_idx]) {
			if (libxmp_alloc_pattern(mod, pat_idx) < 0) {
				return -XMP_ERROR_SYSTEM;
			}
		}

		pat = pat_idx >= mod->pat ? NULL : mod->xxp[pat_idx];
		m->scan_cnt[i] = (uint8_t *)calloc((pat && pat->rows) ? pat->rows : 1, sizeof(uint8_t));
		if (m->scan_cnt[i] == NULL)
			return -XMP_ERROR_SYSTEM;
	}

	return 0;
}

void libxmp_free_scan(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	int i;

	if (m->scan_cnt) {
		for (i = 0; i < mod->len; i++)
			free(m->scan_cnt[i]);

		free(m->scan_cnt);
		m->scan_cnt = NULL;
	}

	free(p->scan);
	p->scan = NULL;
}

/* Process player personality flags */
int libxmp_set_player_mode(struct context_data *ctx)
{
	struct player_data *p = &ctx->p;
	struct module_data *m = &ctx->m;
	int q;

	switch (p->mode) {
	case XMP_MODE_AUTO:
		break;
	case XMP_MODE_MOD:
		m->c4rate = C4_PAL_RATE;
		m->quirk = 0;
		m->read_event_type = READ_EVENT_MOD;
		m->period_type = PERIOD_AMIGA;
		break;
	case XMP_MODE_NOISETRACKER:
		m->c4rate = C4_PAL_RATE;
		m->quirk = QUIRK_NOBPM;
		m->read_event_type = READ_EVENT_MOD;
		m->period_type = PERIOD_MODRNG;
		break;
	case XMP_MODE_PROTRACKER:
		m->c4rate = C4_PAL_RATE;
		m->quirk = QUIRK_PROTRACK;
		m->read_event_type = READ_EVENT_MOD;
		m->period_type = PERIOD_MODRNG;
		break;
	case XMP_MODE_S3M:
		q = m->quirk & (QUIRK_VSALL | QUIRK_ARPMEM);
		m->c4rate = C4_NTSC_RATE;
		m->quirk = QUIRKS_ST3 | q;
		m->read_event_type = READ_EVENT_ST3;
		break;
	case XMP_MODE_ST3:
		q = m->quirk & (QUIRK_VSALL | QUIRK_ARPMEM);
		m->c4rate = C4_NTSC_RATE;
		m->quirk = QUIRKS_ST3 | QUIRK_ST3BUGS | q;
		m->read_event_type = READ_EVENT_ST3;
		break;
	case XMP_MODE_ST3GUS:
		q = m->quirk & (QUIRK_VSALL | QUIRK_ARPMEM);
		m->c4rate = C4_NTSC_RATE;
		m->quirk = QUIRKS_ST3 | QUIRK_ST3BUGS | q;
		m->quirk &= ~QUIRK_RSTCHN;
		m->read_event_type = READ_EVENT_ST3;
		break;
	case XMP_MODE_XM:
		m->c4rate = C4_NTSC_RATE;
		m->quirk = QUIRKS_FT2;
		m->read_event_type = READ_EVENT_FT2;
		break;
	case XMP_MODE_FT2:
		m->c4rate = C4_NTSC_RATE;
		m->quirk = QUIRKS_FT2 | QUIRK_FT2BUGS;
		m->read_event_type = READ_EVENT_FT2;
		break;
	case XMP_MODE_IT:
		m->c4rate = C4_NTSC_RATE;
		m->quirk = QUIRKS_IT | QUIRK_VIBHALF | QUIRK_VIBINV;
		m->read_event_type = READ_EVENT_IT;
		break;
	case XMP_MODE_ITSMP:
		m->c4rate = C4_NTSC_RATE;
		m->quirk = QUIRKS_IT | QUIRK_VIBHALF | QUIRK_VIBINV;
		m->quirk &= ~(QUIRK_VIRTUAL | QUIRK_RSTCHN);
		m->read_event_type = READ_EVENT_IT;
		break;
	default:
		return -1;
	}

	return 0;
}

