/* Extended Module Player
 * Copyright (C) 1996-2023 Claudio Matsuoka and Hipolito Carraro Jr
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

#include "common.h"
#include "loader.h"

/* Convert differential to absolute sample data */
static void convert_delta(uint8_t *p, int l, int r)
{
	uint16_t *w = (uint16_t *)p;
	uint16_t absval = 0;

	if (r) {
		for (; l--;) {
			absval = *w + absval;
			*w++ = absval;
		}
	} else {
		for (; l--;) {
			absval = *p + absval;
			*p++ = (uint8_t) absval;
		}
	}
}

/* Convert signed to unsigned sample data */
static void convert_signal(uint8_t *p, int l, int r)
{
	uint16_t *w = (uint16_t *)p;

	if (r) {
		for (; l--; w++)
			*w += 0x8000;
	} else {
		for (; l--; p++)
			*p += (char)0x80;	/* cast needed by MSVC++ */
	}
}

/* Convert little-endian 16 bit samples to big-endian */
static void convert_endian(uint8_t *p, int l)
{
	uint8_t b;
	int i;

	for (i = 0; i < l; i++) {
		b = p[0];
		p[0] = p[1];
		p[1] = b;
		p += 2;
	}
}

#ifdef LIBXMP_DOWNMIX_STEREO_TO_MONO
/* Downmix stereo samples to mono */
static void convert_stereo_to_mono(uint8_t *p, int l, int r)
{
	int16_t *b = (int16_t *)p;
	int i;

	if (r) {
		l /= 2;
		for (i = 0; i < l; i++)
			b[i] = (b[i * 2] + b[i * 2 + 1]) / 2;
	} else {
		for (i = 0; i < l; i++)
			p[i] = (p[i * 2] + p[i * 2 + 1]) / 2;
	}
}
#endif


int libxmp_load_sample(struct module_data *m, HIO_HANDLE *f, int flags, struct xmp_sample *xxs, const void *buffer)
{
	int bytelen, extralen, i;

	/* Empty or invalid samples
	 */
	if (xxs->len <= 0) {
		return 0;
	}

	/* Skip sample loading
	 * FIXME: fails for ADPCM samples
	 *
	 * + Sanity check: skip huge samples (likely corrupt module)
	 */
	if (xxs->len > MAX_SAMPLE_SIZE || (m && m->smpctl & XMP_SMPCTL_SKIP)) {
		if (~flags & SAMPLE_FLAG_NOLOAD) {
			/* coverity[check_return] */
			hio_seek(f, xxs->len, SEEK_CUR);
		}
		return 0;
	}

	/* If this sample starts at or after EOF, skip it entirely.
	 */
	if (~flags & SAMPLE_FLAG_NOLOAD) {
		long file_pos, file_len;
		if (!f) {
			return 0;
		}
		file_pos = hio_tell(f);
		file_len = hio_size(f);
		if (file_pos >= file_len) {
			D_(D_WARN "ignoring sample at EOF");
			return 0;
		}
		/* If this sample goes past EOF, truncate it. */
		if (file_pos + xxs->len > file_len && (~flags & SAMPLE_FLAG_ADPCM)) {
			D_(D_WARN "sample would extend %ld bytes past EOF; truncating to %ld",
				file_pos + xxs->len - file_len, file_len - file_pos);
			xxs->len = file_len - file_pos;
		}
	}

	/* Loop parameters sanity check
	 */
	if (xxs->lps < 0) {
		xxs->lps = 0;
	}
	if (xxs->lpe > xxs->len) {
		xxs->lpe = xxs->len;
	}
	if (xxs->lps >= xxs->len || xxs->lps >= xxs->lpe) {
		xxs->lps = xxs->lpe = 0;
		xxs->flg &= ~(XMP_SAMPLE_LOOP | XMP_SAMPLE_LOOP_BIDIR);
	}

	/* Patches with samples
	 * Allocate extra sample for interpolation.
	 */
	bytelen = xxs->len;
	extralen = 4;

	/* Disable birectional loop flag if sample is not looped
	 */
	if (xxs->flg & XMP_SAMPLE_LOOP_BIDIR) {
		if (~xxs->flg & XMP_SAMPLE_LOOP)
			xxs->flg &= ~XMP_SAMPLE_LOOP_BIDIR;
	}
	if (xxs->flg & XMP_SAMPLE_SLOOP_BIDIR) {
		if (~xxs->flg & XMP_SAMPLE_SLOOP)
			xxs->flg &= ~XMP_SAMPLE_SLOOP_BIDIR;
	}

	if (xxs->flg & XMP_SAMPLE_16BIT) {
		bytelen *= 2;
		extralen *= 2;
	}

	/* add guard bytes before the buffer for higher order interpolation */
	xxs->data = (unsigned char *) malloc(bytelen + extralen + 4);
	if (xxs->data == NULL) {
		goto err;
	}

	*(uint32_t *)xxs->data = 0;
	xxs->data += 4;

	if (flags & SAMPLE_FLAG_NOLOAD) {
		memcpy(xxs->data, buffer, bytelen);
	} else {
		if (!hio_readn(xxs->data, bytelen, f)) {
			D_(D_WARN "short read in sample load");
		}
	}

	/* Fix endianism if needed */
	if (xxs->flg & XMP_SAMPLE_16BIT) {
#ifdef WORDS_BIGENDIAN
		if (~flags & SAMPLE_FLAG_BIGEND)
			convert_endian(xxs->data, xxs->len);
#else
		if (flags & SAMPLE_FLAG_BIGEND)
			convert_endian(xxs->data, xxs->len);
#endif
	}

	/* Convert delta samples */
	if (flags & SAMPLE_FLAG_DIFF) {
		convert_delta(xxs->data, xxs->len, xxs->flg & XMP_SAMPLE_16BIT);
	} else if (flags & SAMPLE_FLAG_8BDIFF) {
		int len = xxs->len;
		if (xxs->flg & XMP_SAMPLE_16BIT) {
			len *= 2;
		}
		convert_delta(xxs->data, len, 0);
	}

	/* Convert samples to signed */
	if (flags & SAMPLE_FLAG_UNS) {
		convert_signal(xxs->data, xxs->len,
				xxs->flg & XMP_SAMPLE_16BIT);
	}

#ifdef LIBXMP_DOWNMIX_STEREO_TO_MONO
	/* Downmix stereo samples */
	if (flags & SAMPLE_FLAG_STEREO) {
		convert_stereo_to_mono(xxs->data, xxs->len,
					xxs->flg & XMP_SAMPLE_16BIT);
		xxs->len /= 2;
	}
#endif

	/* Check for full loop samples */
	if (flags & SAMPLE_FLAG_FULLREP) {
	    if (xxs->lps == 0 && xxs->len > xxs->lpe)
		xxs->flg |= XMP_SAMPLE_LOOP_FULL;
	}

	/* Add extra samples at end */
	if (xxs->flg & XMP_SAMPLE_16BIT) {
		for (i = 0; i < 8; i++) {
			xxs->data[bytelen + i] = xxs->data[bytelen - 2 + i];
		}
	} else {
		for (i = 0; i < 4; i++) {
			xxs->data[bytelen + i] = xxs->data[bytelen - 1 + i];
		}
	}

	/* Add extra samples at start */
	if (xxs->flg & XMP_SAMPLE_16BIT) {
		xxs->data[-2] = xxs->data[0];
		xxs->data[-1] = xxs->data[1];
	} else {
		xxs->data[-1] = xxs->data[0];
	}

	return 0;

    err:
	return -1;
}

void libxmp_free_sample(struct xmp_sample *s)
{
	if (s->data) {
		free(s->data - 4);
		s->data = NULL;		/* prevent double free in PCM load error */
	}
}
