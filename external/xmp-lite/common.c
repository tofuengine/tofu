/* Extended Module Player
 * Copyright (C) 1996-2018 Claudio Matsuoka and Hipolito Carraro Jr
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
#include <stdarg.h>
#ifdef __WATCOMC__
#include <direct.h>
#elif !defined(_WIN32)
#include <dirent.h>
#endif

#include "xmp.h"
#include "common.h"
#include "period.h"
#include "loader.h"

int libxmp_init_instrument(struct module_data *m)
{
	struct xmp_module *mod = &m->mod;

	if (mod->ins > 0) {
		mod->xxi = calloc(sizeof (struct xmp_instrument), mod->ins);
		if (mod->xxi == NULL)
			return -1;
	}

	if (mod->smp > 0) {
		int i;

		mod->xxs = calloc(sizeof (struct xmp_sample), mod->smp);
		if (mod->xxs == NULL)
			return -1;
		m->xtra = calloc(sizeof (struct extra_sample_data), mod->smp);
		if (m->xtra == NULL)
			return -1;

		for (i = 0; i < mod->smp; i++) {
			m->xtra[i].c5spd = m->c4rate;
		}
	}

	return 0;
}

int libxmp_alloc_subinstrument(struct xmp_module *mod, int i, int num)
{
	if (num == 0)
		return 0;

	mod->xxi[i].sub = calloc(sizeof (struct xmp_subinstrument), num);
	if (mod->xxi[i].sub == NULL)
		return -1;

	return 0;
}

int libxmp_init_pattern(struct xmp_module *mod)
{
	mod->xxt = calloc(sizeof (struct xmp_track *), mod->trk);
	if (mod->xxt == NULL)
		return -1;

	mod->xxp = calloc(sizeof (struct xmp_pattern *), mod->pat);
	if (mod->xxp == NULL)
		return -1;

	return 0;
}

int libxmp_alloc_pattern(struct xmp_module *mod, int num)
{
	/* Sanity check */
	if (num < 0 || num >= mod->pat || mod->xxp[num] != NULL)
		return -1;

	mod->xxp[num] = calloc(1, sizeof (struct xmp_pattern) +
        				sizeof (int) * (mod->chn - 1));
	if (mod->xxp[num] == NULL)
		return -1;

	return 0;
}

int libxmp_alloc_track(struct xmp_module *mod, int num, int rows)
{
	/* Sanity check */
	if (num < 0 || num >= mod->trk || mod->xxt[num] != NULL || rows <= 0)
		return -1;

	mod->xxt[num] = calloc(sizeof (struct xmp_track) +
			       sizeof (struct xmp_event) * (rows - 1), 1);
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
	if (rows < 0 || rows > 256)
		return -1;

	if (libxmp_alloc_pattern(mod, num) < 0)
		return -1;

	mod->xxp[num]->rows = rows;

	if (libxmp_alloc_tracks_in_pattern(mod, num) < 0)
		return -1;

	return 0;
}

/* Sample number adjustment by Vitamin/CAIG */
struct xmp_sample *libxmp_realloc_samples(struct xmp_sample *buf, int *size, int new_size)
{
	buf = realloc(buf, sizeof (struct xmp_sample) * new_size);
	if (buf == NULL)
		return NULL;
	if (new_size > *size)
		memset(buf + *size, 0, sizeof (struct xmp_sample) * (new_size - *size));
	*size = new_size;

	return buf;
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
		if (!isprint((int)s[i]) || ((uint8_t)s[i] > 127))
			s[i] = '.';
	}

	while (*s && (s[strlen(s) - 1] == ' '))
		s[strlen(s) - 1] = 0;

	return s;
}

void libxmp_read_title(HIO_HANDLE *f, char *t, int s)
{
	uint8_t buf[XMP_NAME_SIZE];

	if (t == NULL)
		return;

	if (s >= XMP_NAME_SIZE)
		s = XMP_NAME_SIZE -1;

	memset(t, 0, s + 1);

	hio_read(buf, 1, s, f);		/* coverity[check_return] */
	buf[s] = 0;
	libxmp_copy_adjust(t, buf, s);
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
