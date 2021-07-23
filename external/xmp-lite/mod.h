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

#ifndef LIBXMP_LOADERS_MOD_H
#define LIBXMP_LOADERS_MOD_H

struct mod_instrument {
	uint8_t name[22];	/* Instrument name */
	uint16_t size;		/* Sample length in 16-bit words */
	int8_t finetune;	/* Finetune (signed nibble) */
	int8_t volume;		/* Linear playback volume */
	uint16_t loop_start;	/* Loop start in 16-bit words */
	uint16_t loop_size;	/* Loop length in 16-bit words */
};

struct mod_header {
	uint8_t name[20];
	struct mod_instrument ins[31];
	uint8_t len;
	uint8_t restart;	/* Number of patterns in Soundtracker,
				 * Restart in Noisetracker/Startrekker,
				 * 0x7F in Protracker
				 */
	uint8_t order[128];
	uint8_t magic[4];
};

extern const struct format_loader libxmp_loader_mod;

#endif  /* LIBXMP_LOADERS_MOD_H */
