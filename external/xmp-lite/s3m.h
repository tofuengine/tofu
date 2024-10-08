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

#ifndef LIBXMP_LOADERS_S3M_H
#define LIBXMP_LOADERS_S3M_H

/* S3M packed pattern macros */
#define S3M_EOR		0	/* End of row */
#define S3M_CH_MASK	0x1f	/* Channel */
#define S3M_NI_FOLLOW	0x20	/* Note and instrument follow */
#define S3M_VOL_FOLLOWS	0x40	/* Volume follows */
#define S3M_FX_FOLLOWS	0x80	/* Effect and parameter follow */

/* S3M mix volume macros */
#define S3M_MV_VOLUME	0x7f	/* Module mix volume, typically 16 to 127 */
#define S3M_MV_STEREO	0x80	/* Module is stereo if set, otherwise mono */

/* S3M channel info macros */
#define S3M_CH_ON	0x80	/* Psi says it's bit 8, I'll assume bit 7 */
#define S3M_CH_OFF	0xff
#define S3M_CH_NUMBER	0x1f
#define S3M_CH_RIGHT	0x08
#define S3M_CH_ADLIB	0x10

/* S3M channel pan macros */
#define S3M_PAN_SET	0x20
#define S3M_PAN_MASK	0x0f

/* S3M flags */
#define S3M_ST2_VIB	0x01	/* Not recognized */
#define S3M_ST2_TEMPO	0x02	/* Not recognized */
#define S3M_AMIGA_SLIDE	0x04	/* Not recognized */
#define S3M_VOL_OPT	0x08	/* Not recognized */
#define S3M_AMIGA_RANGE	0x10
#define S3M_SB_FILTER	0x20	/* Not recognized */
#define S3M_ST300_VOLS	0x40
#define S3M_CUSTOM_DATA	0x80	/* Not recognized */

/* S3M Adlib instrument types */
#define S3M_INST_SAMPLE	0x01
#define S3M_INST_AMEL	0x02
#define S3M_INST_ABD	0x03
#define S3M_INST_ASNARE	0x04
#define S3M_INST_ATOM	0x05
#define S3M_INST_ACYM	0x06
#define S3M_INST_AHIHAT	0x07

struct s3m_file_header {
	uint8_t name[28];	/* Song name */
	uint8_t doseof;	/* 0x1a */
	uint8_t type;		/* File type */
	uint8_t rsvd1[2];	/* Reserved */
	uint16_t ordnum;	/* Number of orders (must be even) */
	uint16_t insnum;	/* Number of instruments */
	uint16_t patnum;	/* Number of patterns */
	uint16_t flags;	/* Flags */
	uint16_t version;	/* Tracker ID and version */
	uint16_t ffi;		/* File format information */
	uint32_t magic;	/* 'SCRM' */
	uint8_t gv;		/* Global volume */
	uint8_t is;		/* Initial speed */
	uint8_t it;		/* Initial tempo */
	uint8_t mv;		/* Master volume */
	uint8_t uc;		/* Ultra click removal */
	uint8_t dp;		/* Default pan positions if 0xfc */
	uint8_t rsvd2[8];	/* Reserved */
	uint16_t special;	/* Ptr to special custom data */
	uint8_t chset[32];	/* Channel settings */
};

struct s3m_instrument_header {
	uint8_t dosname[12];	/* DOS file name */
	uint8_t memseg_hi;	/* High byte of sample pointer */
	uint16_t memseg;	/* Pointer to sample data */
	uint32_t length;	/* Length */
	uint32_t loopbeg;	/* Loop begin */
	uint32_t loopend;	/* Loop end */
	uint8_t vol;		/* Volume */
	uint8_t rsvd1;		/* Reserved */
	uint8_t pack;		/* Packing type (not used) */
	uint8_t flags;		/* Loop/stereo/16bit samples flags */
	uint16_t c2spd;	/* C 4 speed */
	uint16_t rsvd2;	/* Reserved */
	uint8_t rsvd3[4];	/* Reserved */
	uint16_t int_gp;	/* Internal - GUS pointer */
	uint16_t int_512;	/* Internal - SB pointer */
	uint32_t int_last;	/* Internal - SB index */
	uint8_t name[28];	/* Instrument name */
	uint32_t magic;	/* 'SCRS' */
};

#endif  /* LIBXMP_LOADERS_S3M_H */
