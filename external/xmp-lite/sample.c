/* Extended Module Player
 * Copyright (C) 1996-2025 Claudio Matsuoka and Hipolito Carraro Jr
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
static void convert_delta(uint8_t *p, int frames, int is_16bit, int channels)
{
	uint16_t *w = (uint16_t *)p;
	uint16_t absval;
	int chn, i;

	if (is_16bit) {
		for (chn = 0; chn < channels; chn++) {
			absval = 0;
			for (i = 0; i < frames; i++) {
				absval = *w + absval;
				*w++ = absval;
			}
		}
	} else {
		for (chn = 0; chn < channels; chn++) {
			absval = 0;
			for (i = 0; i < frames; i++) {
				absval = *p + absval;
				*p++ = (uint8_t) absval;
			}
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
			*p += (uint8_t)0x80;
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

/* Convert non-interleaved stereo to interleaved stereo.
 * Due to tracker quirks this should be done after delta decoding, etc. */
static void convert_stereo_interleaved(void * LIBXMP_RESTRICT _out,
 const void *in, int frames, int is_16bit)
{
	int i;

	if (is_16bit) {
		const int16_t *in_l = (const int16_t *)in;
		const int16_t *in_r = in_l + frames;
		int16_t *out = (int16_t *)_out;

		for (i = 0; i < frames; i++) {
			*(out++) = *(in_l++);
			*(out++) = *(in_r++);
		}
	} else {
		const uint8 *in_l = (const uint8 *)in;
		const uint8 *in_r = in_l + frames;
		uint8 *out = (uint8 *)_out;

		for (i = 0; i < frames; i++) {
			*(out++) = *(in_l++);
			*(out++) = *(in_r++);
		}
	}
}


int libxmp_load_sample(struct module_data *m, HIO_HANDLE *f, int flags, struct xmp_sample *xxs, const void *buffer)
{
	unsigned char *tmp = NULL;
	unsigned char *dest;
	int channels = 1;
	int framelen;
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

	/* Patches with samples
	 * Allocate extra sample for interpolation.
	 */
	bytelen = xxs->len;
	framelen = 1;
	extralen = 4;

	if (xxs->flg & XMP_SAMPLE_16BIT) {
		bytelen *= 2;
		extralen *= 2;
		framelen *= 2;
	}
	if (xxs->flg & XMP_SAMPLE_STEREO) {
		bytelen *= 2;
		extralen *= 2;
		framelen *= 2;
		channels = 2;
	}

	/* If this sample starts at or after EOF, skip it entirely.
	 */
	if (~flags & SAMPLE_FLAG_NOLOAD) {
		long file_pos, file_len;
		long remaining = 0;
		long over = 0;
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
		remaining = file_len - file_pos;

		if (bytelen > remaining) {
			over = bytelen - remaining;
			bytelen = remaining;
		}

		if (over) {
			D_(D_WARN "sample would extend %ld bytes past EOF; truncating to %ld",
				over, remaining);

			/* Trim extra bytes non-aligned to sample frame. */
			bytelen -= bytelen & (framelen - 1);

			xxs->len = bytelen;
			if (xxs->flg & XMP_SAMPLE_16BIT)
				xxs->len >>= 1;
			if (xxs->flg & XMP_SAMPLE_STEREO)
				xxs->len >>= 1;
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

	/* Disable bidirectional loop flag if sample is not looped
	 */
	if (xxs->flg & XMP_SAMPLE_LOOP_BIDIR) {
		if (~xxs->flg & XMP_SAMPLE_LOOP)
			xxs->flg &= ~XMP_SAMPLE_LOOP_BIDIR;
	}
	if (xxs->flg & XMP_SAMPLE_SLOOP_BIDIR) {
		if (~xxs->flg & XMP_SAMPLE_SLOOP)
			xxs->flg &= ~XMP_SAMPLE_SLOOP_BIDIR;
	}

	/* add guard bytes before the buffer for higher order interpolation */
	xxs->data = (unsigned char *) malloc(bytelen + extralen + 4);
	if (xxs->data == NULL) {
		goto err;
	}

	*(uint32_t *)xxs->data = 0;
	xxs->data += 4;
	dest = xxs->data;

	/* If this is a non-interleaved stereo sample, most conversions need
	 * to occur in an intermediate buffer prior to interleaving. Most
	 * formats supporting stereo samples use non-interleaved stereo.
	 */
	if ((xxs->flg & XMP_SAMPLE_STEREO) && (~flags & SAMPLE_FLAG_INTERLEAVED)) {
		tmp = (unsigned char *) malloc(bytelen);
		if (!tmp)
			goto err2;

		dest = tmp;
	}

	if (flags & SAMPLE_FLAG_NOLOAD) {
		memcpy(dest, buffer, bytelen);
	} else {
		int bytes_read = (int)hio_read(dest, 1, bytelen, f);
		if (bytes_read != bytelen) {
			D_(D_WARN "short read (%d) in sample load", bytes_read - bytelen);
			memset(dest + bytes_read, 0, bytelen - bytes_read);
		}
	}

	/* Fix endianism if needed */
	if (xxs->flg & XMP_SAMPLE_16BIT) {
#ifdef WORDS_BIGENDIAN
		if (~flags & SAMPLE_FLAG_BIGEND)
			convert_endian(dest, xxs->len * channels);
#else
		if (flags & SAMPLE_FLAG_BIGEND)
			convert_endian(dest, xxs->len * channels);
#endif
	}

	/* Convert delta samples */
	if (flags & SAMPLE_FLAG_DIFF) {
		convert_delta(dest, xxs->len, xxs->flg & XMP_SAMPLE_16BIT, channels);
	} else if (flags & SAMPLE_FLAG_8BDIFF) {
		int len = xxs->len;
		if (xxs->flg & XMP_SAMPLE_16BIT) {
			len *= 2;
		}
		convert_delta(dest, len, 0, channels);
	}

	/* Convert samples to signed */
	if (flags & SAMPLE_FLAG_UNS) {
		convert_signal(dest, xxs->len * channels,
				xxs->flg & XMP_SAMPLE_16BIT);
	}

	/* Done converting individual samples; convert to interleaved. */
	if ((xxs->flg & XMP_SAMPLE_STEREO) && (~flags & SAMPLE_FLAG_INTERLEAVED)) {
		convert_stereo_interleaved(xxs->data, dest, xxs->len,
					   xxs->flg & XMP_SAMPLE_16BIT);
	}

	/* Check for full loop samples */
	if (flags & SAMPLE_FLAG_FULLREP) {
	    if (xxs->lps == 0 && xxs->len > xxs->lpe)
		xxs->flg |= XMP_SAMPLE_LOOP_FULL;
	}

	/* Add extra samples at end */
	for (i = 0; i < extralen; i++) {
		xxs->data[bytelen + i] = xxs->data[bytelen - framelen + i];
	}

	/* Add extra samples at start */
	for (i = -1; i >= -4; i--) {
		xxs->data[i] = xxs->data[framelen + i];
	}

	free(tmp);
	return 0;

    err2:
	libxmp_free_sample(xxs);
	free(tmp);
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
