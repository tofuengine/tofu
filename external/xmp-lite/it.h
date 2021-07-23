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

#ifndef LIBXMP_LOADERS_IT_H
#define LIBXMP_LOADERS_IT_H

/* IT flags */
#define IT_STEREO	0x01
#define IT_VOL_OPT	0x02	/* Not recognized */
#define IT_USE_INST	0x04
#define IT_LINEAR_FREQ	0x08
#define IT_OLD_FX	0x10
#define IT_LINK_GXX	0x20

/* IT special */
#define IT_HAS_MSG	0x01

/* IT instrument flags */
#define IT_INST_SAMPLE	0x01
#define IT_INST_16BIT	0x02
#define IT_INST_STEREO	0x04
#define IT_INST_LOOP	0x10
#define IT_INST_SLOOP	0x20
#define IT_INST_BLOOP	0x40
#define IT_INST_BSLOOP	0x80

/* IT sample flags */
#define IT_SMP_SAMPLE	0x01
#define IT_SMP_16BIT	0x02
#define IT_SMP_STEREO	0x04	/* unsupported */
#define IT_SMP_COMP	0x08	/* unsupported */
#define IT_SMP_LOOP	0x10
#define IT_SMP_SLOOP	0x20
#define IT_SMP_BLOOP	0x40
#define IT_SMP_BSLOOP	0x80

/* IT sample conversion flags */
#define IT_CVT_SIGNED	0x01
#define IT_CVT_BIGEND	0x02	/* 'safe to ignore' according to ittech.txt */
#define IT_CVT_DIFF	0x04	/* Compressed sample flag */
#define IT_CVT_BYTEDIFF	0x08	/* 'safe to ignore' according to ittech.txt */
#define IT_CVT_12BIT	0x10	/* 'safe to ignore' according to ittech.txt */

/* IT envelope flags */
#define IT_ENV_ON	0x01
#define IT_ENV_LOOP	0x02
#define IT_ENV_SLOOP	0x04
#define IT_ENV_CARRY	0x08
#define IT_ENV_FILTER	0x80


struct it_file_header {
	uint32_t magic;	/* 'IMPM' */
	uint8_t name[26];	/* ASCIIZ Song name */
	uint8_t hilite_min;	/* Pattern editor highlight */
	uint8_t hilite_maj;	/* Pattern editor highlight */
	uint16_t ordnum;	/* Number of orders (must be even) */
	uint16_t insnum;	/* Number of instruments */
	uint16_t smpnum;	/* Number of samples */
	uint16_t patnum;	/* Number of patterns */
	uint16_t cwt;		/* Tracker ID and version */
	uint16_t cmwt;		/* Format version */
	uint16_t flags;	/* Flags */
	uint16_t special;	/* More flags */
	uint8_t gv;		/* Global volume */
	uint8_t mv;		/* Master volume */
	uint8_t is;		/* Initial speed */
	uint8_t it;		/* Initial tempo */
	uint8_t sep;		/* Panning separation */
	uint8_t pwd;		/* Pitch wheel depth */
	uint16_t msglen;	/* Message length */
	uint32_t msgofs;	/* Message offset */
	uint32_t rsvd;		/* Reserved */
	uint8_t chpan[64];	/* Channel pan settings */
	uint8_t chvol[64];	/* Channel volume settings */
};

struct it_instrument1_header {
	uint32_t magic;	/* 'IMPI' */
	uint8_t dosname[12];	/* DOS filename */
	uint8_t zero;		/* Always zero */
	uint8_t flags;		/* Instrument flags */
	uint8_t vls;		/* Volume loop start */
	uint8_t vle;		/* Volume loop end */
	uint8_t sls;		/* Sustain loop start */
	uint8_t sle;		/* Sustain loop end */
	uint16_t rsvd1;	/* Reserved */
	uint16_t fadeout;	/* Fadeout (release) */
	uint8_t nna;		/* New note action */
	uint8_t dnc;		/* Duplicate note check */
	uint16_t trkvers;	/* Tracker version */
	uint8_t nos;		/* Number of samples */
	uint8_t rsvd2;		/* Reserved */
	uint8_t name[26];	/* ASCIIZ Instrument name */
	uint8_t rsvd3[6];	/* Reserved */
	uint8_t keys[240];
	uint8_t epoint[200];
	uint8_t enode[50];
};

struct it_instrument2_header {
	uint32_t magic;	/* 'IMPI' */
	uint8_t dosname[12];	/* DOS filename */
	uint8_t zero;		/* Always zero */
	uint8_t nna;		/* New Note Action */
	uint8_t dct;		/* Duplicate Check Type */
	uint8_t dca;		/* Duplicate Check Action */
	uint16_t fadeout;
	uint8_t pps;		/* Pitch-Pan Separation */
	uint8_t ppc;		/* Pitch-Pan Center */
	uint8_t gbv;		/* Global Volume */
	uint8_t dfp;		/* Default pan */
	uint8_t rv;		/* Random volume variation */
	uint8_t rp;		/* Random pan variation */
	uint16_t trkvers;	/* Not used: tracked version */
	uint8_t nos;		/* Not used: number of samples */
	uint8_t rsvd1;		/* Reserved */
	uint8_t name[26];	/* ASCIIZ Instrument name */
	uint8_t ifc;		/* Initial filter cutoff */
	uint8_t ifr;		/* Initial filter resonance */
	uint8_t mch;		/* MIDI channel */
	uint8_t mpr;		/* MIDI program */
	uint16_t mbnk;		/* MIDI bank */
	uint8_t keys[240];
};

struct it_envelope_node {
	int8_t y;
	uint16_t x;
};

struct it_envelope {
	uint8_t flg;		/* Flags */
	uint8_t num;		/* Number of node points */
	uint8_t lpb;		/* Loop beginning */
	uint8_t lpe;		/* Loop end */
	uint8_t slb;		/* Sustain loop beginning */
	uint8_t sle;		/* Sustain loop end */
	struct it_envelope_node node[25];
	uint8_t unused;
};

struct it_sample_header {
	uint32_t magic;	/* 'IMPS' */
	uint8_t dosname[12];	/* DOS filename */
	uint8_t zero;		/* Always zero */
	uint8_t gvl;		/* Global volume for instrument */
	uint8_t flags;		/* Sample flags */
	uint8_t vol;		/* Volume */
	uint8_t name[26];	/* ASCIIZ sample name */
	uint8_t convert;	/* Sample flags */
	uint8_t dfp;		/* Default pan */
	uint32_t length;	/* Length */
	uint32_t loopbeg;	/* Loop begin */
	uint32_t loopend;	/* Loop end */
	uint32_t c5spd;	/* C 5 speed */
	uint32_t sloopbeg;	/* SusLoop begin */
	uint32_t sloopend;	/* SusLoop end */
	uint32_t sample_ptr;	/* Sample pointer */
	uint8_t vis;		/* Vibrato speed */
	uint8_t vid;		/* Vibrato depth */
	uint8_t vir;		/* Vibrato rate */
	uint8_t vit;		/* Vibrato waveform */
};

extern const struct format_loader libxmp_loader_it;

#endif /* LIBXMP_LOADERS_IT_H */
