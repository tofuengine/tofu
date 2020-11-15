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

#include "common.h"
#include "loader.h"

/* Convert differential to absolute sample data */
static void convert_delta(uint8_t *p, int l, int r)
{
	uint16_t *w = (uint16_t *)p;
	uint16_t abs = 0;

	if (r) {
		for (; l--;) {
			abs = *w + abs;
			*w++ = abs;
		}
	} else {
		for (; l--;) {
			abs = *p + abs;
			*p++ = (uint8_t) abs;
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

static void unroll_loop(struct xmp_sample *xxs)
{
	int8_t *s8;
	int16_t *s16;
	int start, loop_size;
	int i;

	s16 = (int16_t *)xxs->data;
	s8 = (int8_t *)xxs->data;

	if (xxs->len > xxs->lpe) {
		start = xxs->lpe;
	} else {
		start = xxs->len;
	}

	loop_size = xxs->lpe - xxs->lps;

	if (xxs->flg & XMP_SAMPLE_16BIT) {
		s16 += start;
		for (i = 0; i < loop_size; i++) {
			*(s16 + i) = *(s16 - i - 1);	
		}
	} else {
		s8 += start;
		for (i = 0; i < loop_size; i++) {
			*(s8 + i) = *(s8 - i - 1);	
		}
	}
}


int libxmp_load_sample(struct module_data *m, HIO_HANDLE *f, int flags, struct xmp_sample *xxs, const void *buffer)
{
	int bytelen, extralen, unroll_extralen, i;

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
	unroll_extralen = 0;

	/* Disable birectional loop flag if sample is not looped
	 */
	if (xxs->flg & XMP_SAMPLE_LOOP_BIDIR) {
		if (~xxs->flg & XMP_SAMPLE_LOOP)
			xxs->flg &= ~XMP_SAMPLE_LOOP_BIDIR;
	}
	/* Unroll bidirectional loops
	 */
	if (xxs->flg & XMP_SAMPLE_LOOP_BIDIR) {
		unroll_extralen = (xxs->lpe - xxs->lps) -
				(xxs->len - xxs->lpe);

		if (unroll_extralen < 0) {
			unroll_extralen = 0;
		}
	}

	if (xxs->flg & XMP_SAMPLE_16BIT) {
		bytelen *= 2;
		extralen *= 2;
		unroll_extralen *= 2;
	}

	/* add guard bytes before the buffer for higher order interpolation */
	xxs->data = malloc(bytelen + extralen + unroll_extralen + 4);
	if (xxs->data == NULL) {
		goto err;
	}

	*(uint32_t *)xxs->data = 0;
	xxs->data += 4;

	if (flags & SAMPLE_FLAG_NOLOAD) {
		memcpy(xxs->data, buffer, bytelen);
	} else {
		int x = hio_read(xxs->data, 1, bytelen, f);
		if (x != bytelen) {
			D_(D_WARN "short read (%d) in sample load", x - bytelen);
			memset(xxs->data + x, 0, bytelen - x);
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

	/* Unroll bidirectional loops */
	if (xxs->flg & XMP_SAMPLE_LOOP_BIDIR) {
		unroll_loop(xxs);
		bytelen += unroll_extralen;
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

	/* Fix sample at loop */
	if (xxs->flg & XMP_SAMPLE_LOOP) {
		int lpe = xxs->lpe;
		int lps = xxs->lps;

		if (xxs->flg & XMP_SAMPLE_LOOP_BIDIR) {
			lpe += lpe - lps;
		}

		if (xxs->flg & XMP_SAMPLE_16BIT) {
			lpe <<= 1;
			lps <<= 1;
			for (i = 0; i < 8; i++) {
				xxs->data[lpe + i] = xxs->data[lps + i];
			}
		} else {
			for (i = 0; i < 4; i++) {
				xxs->data[lpe + i] = xxs->data[lps + i];
			}
		}
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
