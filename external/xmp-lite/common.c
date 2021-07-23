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

#include <ctype.h>

#include "common.h"
#include "xmp.h"
#include "period.h"
#include "loader.h"

int libxmp_init_instrument(struct module_data *m)
{
	struct xmp_module *mod = &m->mod;

	if (mod->ins > 0) {
		mod->xxi = (struct xmp_instrument *)calloc(mod->ins, sizeof(struct xmp_instrument));
		if (mod->xxi == NULL)
			return -1;
	}

	if (mod->smp > 0) {
		int i;
		/* Sanity check */
		if (mod->smp > MAX_SAMPLES) {
			D_(D_CRIT "sample count %d exceeds maximum (%d)",
			   mod->smp, MAX_SAMPLES);
			return -1;
		}

		mod->xxs = (struct xmp_sample *)calloc(mod->smp, sizeof(struct xmp_sample));
		if (mod->xxs == NULL)
			return -1;
		m->xtra = (struct extra_sample_data *)calloc(mod->smp, sizeof(struct extra_sample_data));
		if (m->xtra == NULL)
			return -1;

		for (i = 0; i < mod->smp; i++) {
			m->xtra[i].c5spd = m->c4rate;
		}
	}

	return 0;
}

/* Sample number adjustment (originally by Vitamin/CAIG).
 * Only use this AFTER a previous usage of libxmp_init_instrument,
 * and don't use this to free samples that have already been loaded. */
int libxmp_realloc_samples(struct module_data *m, int new_size)
{
	struct xmp_module *mod = &m->mod;
	struct xmp_sample *xxs;
	struct extra_sample_data *xtra;

	/* Sanity check */
	if (new_size < 0)
		return -1;

	if (new_size == 0) {
		/* Don't rely on implementation-defined realloc(x,0) behavior. */
		mod->smp = 0;
		free(mod->xxs);
		mod->xxs = NULL;
		free(m->xtra);
		m->xtra = NULL;
		return 0;
	}

	xxs = (struct xmp_sample *)realloc(mod->xxs, sizeof(struct xmp_sample) * new_size);
	if (xxs == NULL)
		return -1;
	mod->xxs = xxs;

	xtra = (struct extra_sample_data *)realloc(m->xtra, sizeof(struct extra_sample_data) * new_size);
	if (xtra == NULL)
		return -1;
	m->xtra = xtra;

	if (new_size > mod->smp) {
		int clear_size = new_size - mod->smp;
		int i;

		memset(xxs + mod->smp, 0, sizeof(struct xmp_sample) * clear_size);
		memset(xtra + mod->smp, 0, sizeof(struct extra_sample_data) * clear_size);

		for (i = mod->smp; i < new_size; i++) {
			m->xtra[i].c5spd = m->c4rate;
		}
	}
	mod->smp = new_size;
	return 0;
}

int libxmp_alloc_subinstrument(struct xmp_module *mod, int i, int num)
{
	if (num == 0)
		return 0;

	mod->xxi[i].sub = (struct xmp_subinstrument *)calloc(num, sizeof(struct xmp_subinstrument));
	if (mod->xxi[i].sub == NULL)
		return -1;

	return 0;
}

int libxmp_init_pattern(struct xmp_module *mod)
{
	mod->xxt = (struct xmp_track **)calloc(mod->trk, sizeof(struct xmp_track *));
	if (mod->xxt == NULL)
		return -1;

	mod->xxp = (struct xmp_pattern **)calloc(mod->pat, sizeof(struct xmp_pattern *));
	if (mod->xxp == NULL)
		return -1;

	return 0;
}

int libxmp_alloc_pattern(struct xmp_module *mod, int num)
{
	/* Sanity check */
	if (num < 0 || num >= mod->pat || mod->xxp[num] != NULL)
		return -1;

	mod->xxp[num] = (struct xmp_pattern *)calloc(1, sizeof(struct xmp_pattern) +
        				sizeof(int) * (mod->chn - 1));
	if (mod->xxp[num] == NULL)
		return -1;

	return 0;
}

int libxmp_alloc_track(struct xmp_module *mod, int num, int rows)
{
	/* Sanity check */
	if (num < 0 || num >= mod->trk || mod->xxt[num] != NULL || rows <= 0)
		return -1;

	mod->xxt[num] = (struct xmp_track *)calloc(1, sizeof(struct xmp_track) +
			       sizeof(struct xmp_event) * (rows - 1));
	if (mod->xxt[num] == NULL)
		return -1;

	mod->xxt[num]->rows = rows;

	return 0;
}

int libxmp_alloc_tracks_in_pattern(struct xmp_module *mod, int num)
{
	int i;

	D_(D_INFO "allocating %d tracks w/ %d rows", mod->chn, mod->xxp[num]->rows);
	for (i = 0; i < mod->chn; i++) {
		int t = num * mod->chn + i;
		int rows = mod->xxp[num]->rows;

		if (libxmp_alloc_track(mod, t, rows) < 0)
			return -1;

		mod->xxp[num]->index[i] = t;
	}

	return 0;
}

int libxmp_alloc_pattern_tracks(struct xmp_module *mod, int num, int rows)
{
	/* Sanity check */
	if (rows <= 0 || rows > 256)
		return -1;

	if (libxmp_alloc_pattern(mod, num) < 0)
		return -1;

	mod->xxp[num]->rows = rows;

	if (libxmp_alloc_tracks_in_pattern(mod, num) < 0)
		return -1;

	return 0;
}

char *libxmp_instrument_name(struct xmp_module *mod, int i, uint8_t *r, int n)
{
	CLAMP(n, 0, 31);

	return libxmp_copy_adjust(mod->xxi[i].name, r, n);
}

char *libxmp_copy_adjust(char *s, uint8_t *r, int n)
{
	int i;

	memset(s, 0, n + 1);
	strncpy(s, (char *)r, n);

	for (i = 0; s[i] && i < n; i++) {
		if (!isprint((unsigned char)s[i]) || ((uint8)s[i] > 127))
			s[i] = '.';
	}

	while (*s && (s[strlen(s) - 1] == ' '))
		s[strlen(s) - 1] = 0;

	return s;
}

void libxmp_read_title(HIO_HANDLE *f, char *t, int s)
{
	uint8_t buf[XMP_NAME_SIZE];

	if (t == NULL || s < 0)
		return;

	if (s >= XMP_NAME_SIZE)
		s = XMP_NAME_SIZE -1;

	memset(t, 0, s + 1);

	s = hio_read(buf, 1, s, f);
	buf[s] = 0;
	libxmp_copy_adjust(t, buf, s);
}

int libxmp_copy_name_for_fopen(char *dest, const char *name, int n)
{
	int converted_colon = 0;
	int i;

	/* libxmp_copy_adjust, but make sure the filename won't do anything
	 * malicious when given to fopen. This should only be used on song files.
	 */
	if (!strcmp(name, ".") || strstr(name, "..") ||
	    name[0] == '\\' || name[0] == '/' || name[0] == ':')
		return -1;

	for (i = 0; i < n - 1; i++) {
		uint8_t t = name[i];
		if (!t)
			break;

		/* Reject non-ASCII symbols as they have poorly defined behavior.
		 */
		if (t < 32 || t >= 0x7f)
			return -1;

		/* Reject anything resembling a Windows-style root path. Allow
		 * converting a single : to / so things like ST-01:samplename
		 * work. (Leave the : as-is on Amiga.)
		 */
		if (i > 0 && t == ':' && !converted_colon) {
			uint8_t t2 = name[i + 1];
			if (!t2 || t2 == '/' || t2 == '\\')
				return -1;

			converted_colon = 1;
#ifndef LIBXMP_AMIGA
			dest[i] = '/';
			continue;
#endif
		}

		if (t == '\\') {
			dest[i] = '/';
			continue;
		}

		dest[i] = t;
	}
	dest[i] = '\0';
	return 0;
}

/*
 * Honor Noisetracker effects:
 *
 *  0 - arpeggio
 *  1 - portamento up
 *  2 - portamento down
 *  3 - Tone-portamento
 *  4 - Vibrato
 *  A - Slide volume
 *  B - Position jump
 *  C - Set volume
 *  D - Pattern break
 *  E - Set filter (keep the led off, please!)
 *  F - Set speed (now up to $1F)
 *
 * Pex Tufvesson's notes from http://www.livet.se/mahoney/:
 *
 * Note that some of the modules will have bugs in the playback with all
 * known PC module players. This is due to that in many demos where I synced
 * events in the demo with the music, I used commands that these newer PC
 * module players erroneously interpret as "newer-version-trackers commands".
 * Which they aren't.
 */
void libxmp_decode_noisetracker_event(struct xmp_event *event, uint8_t *mod_event)
{
	int fxt;

	memset(event, 0, sizeof (struct xmp_event));
	event->note = libxmp_period_to_note((LSN(mod_event[0]) << 8) + mod_event[1]);
	event->ins = ((MSN(mod_event[0]) << 4) | MSN(mod_event[2]));
	fxt = LSN(mod_event[2]);

	if (fxt <= 0x06 || (fxt >= 0x0a && fxt != 0x0e)) {
		event->fxt = fxt;
		event->fxp = mod_event[3];
	}

	libxmp_disable_continue_fx(event);
}

void libxmp_decode_protracker_event(struct xmp_event *event, uint8_t *mod_event)
{
	int fxt = LSN(mod_event[2]);

	memset(event, 0, sizeof (struct xmp_event));
	event->note = libxmp_period_to_note((LSN(mod_event[0]) << 8) + mod_event[1]);
	event->ins = ((MSN(mod_event[0]) << 4) | MSN(mod_event[2]));

	if (fxt != 0x08) {
		event->fxt = fxt;
		event->fxp = mod_event[3];
	}

	libxmp_disable_continue_fx(event);
}

void libxmp_disable_continue_fx(struct xmp_event *event)
{
	if (event->fxp == 0) {
		switch (event->fxt) {
		case 0x05:
			event->fxt = 0x03;
			break;
		case 0x06:
			event->fxt = 0x04;
			break;
		case 0x01:
		case 0x02:
		case 0x0a:
			event->fxt = 0x00;
		}
	} else if (event->fxt == 0x0e) {
		if (event->fxp == 0xa0 || event->fxp == 0xb0) {
			event->fxt = event->fxp = 0;
		}
	}
}

void libxmp_set_type(struct module_data *m, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	vsnprintf(m->mod.type, XMP_NAME_SIZE, fmt, ap);
	va_end(ap);
}
