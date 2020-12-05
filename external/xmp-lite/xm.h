#ifndef LIBXMP_LOADERS_XM_H
#define LIBXMP_LOADERS_XM_H

#define XM_EVENT_PACKING 0x80
#define XM_EVENT_PACK_MASK 0x7f
#define XM_EVENT_NOTE_FOLLOWS 0x01
#define XM_EVENT_INSTRUMENT_FOLLOWS 0x02
#define XM_EVENT_VOLUME_FOLLOWS 0x04
#define XM_EVENT_FXTYPE_FOLLOWS 0x08
#define XM_EVENT_FXPARM_FOLLOWS 0x10
#define XM_LINEAR_FREQ 0x01
#define XM_LOOP_MASK 0x03
#define XM_LOOP_NONE 0
#define XM_LOOP_FORWARD 1
#define XM_LOOP_PINGPONG 2
#define XM_SAMPLE_16BIT 0x10
#define XM_ENVELOPE_ON 0x01
#define XM_ENVELOPE_SUSTAIN 0x02
#define XM_ENVELOPE_LOOP 0x04
#define XM_LINEAR_PERIOD_MODE 0x01


struct xm_file_header {
	uint8_t id[17];	/* ID text: "Extended module: " */
	uint8_t name[20];	/* Module name, padded with zeroes */
	uint8_t doseof;	/* 0x1a */
	uint8_t tracker[20];	/* Tracker name */
	uint16_t version;	/* Version number, minor-major */
	uint32_t headersz;	/* Header size */
	uint16_t songlen;	/* Song length (in patten order table) */
	uint16_t restart;	/* Restart position */
	uint16_t channels;	/* Number of channels (2,4,6,8,10,...,32) */
	uint16_t patterns;	/* Number of patterns (max 256) */
	uint16_t instruments;	/* Number of instruments (max 128) */
	uint16_t flags;	/* bit 0: 0=Amiga freq table, 1=Linear */
	uint16_t tempo;	/* Default tempo */
	uint16_t bpm;		/* Default BPM */
	uint8_t order[256];	/* Pattern order table */
};

struct xm_pattern_header {
	uint32_t length;	/* Pattern header length */
	uint8_t packing;	/* Packing type (always 0) */
	uint16_t rows;		/* Number of rows in pattern (1..256) */
	uint16_t datasize;	/* Packed patterndata size */
};

struct xm_instrument_header {
	uint32_t size;		/* Instrument size */
	uint8_t name[22];	/* Instrument name */
	uint8_t type;		/* Instrument type (always 0) */
	uint16_t samples;	/* Number of samples in instrument */
	uint32_t sh_size;	/* Sample header size */
};

struct xm_instrument {
	uint8_t sample[96];	/* Sample number for all notes */
	uint16_t v_env[24];	/* Points for volume envelope */
	uint16_t p_env[24];	/* Points for panning envelope */
	uint8_t v_pts;		/* Number of volume points */
	uint8_t p_pts;		/* Number of panning points */
	uint8_t v_sus;		/* Volume sustain point */
	uint8_t v_start;	/* Volume loop start point */
	uint8_t v_end;		/* Volume loop end point */
	uint8_t p_sus;		/* Panning sustain point */
	uint8_t p_start;	/* Panning loop start point */
	uint8_t p_end;		/* Panning loop end point */
	uint8_t v_type;	/* Bit 0: On; 1: Sustain; 2: Loop */
	uint8_t p_type;	/* Bit 0: On; 1: Sustain; 2: Loop */
	uint8_t y_wave;	/* Vibrato waveform */
	uint8_t y_sweep;	/* Vibrato sweep */
	uint8_t y_depth;	/* Vibrato depth */
	uint8_t y_rate;	/* Vibrato rate */
	uint16_t v_fade;	/* Volume fadeout */
#if 0
	uint8_t reserved[22];	/* Reserved; 2 bytes in specs, 22 in 1.04 */
#endif
};

struct xm_sample_header {
	uint32_t length;	/* Sample length */
	uint32_t loop_start;	/* Sample loop start */
	uint32_t loop_length;	/* Sample loop length */
	uint8_t volume;	/* Volume */
	int8_t finetune;	/* Finetune (signed byte -128..+127) */
	uint8_t type;		/* 0=No loop,1=Fwd loop,2=Ping-pong,16-bit */
	uint8_t pan;		/* Panning (0-255) */
	int8_t relnote;	/* Relative note number (signed byte) */
	uint8_t reserved;	/* Reserved */
	uint8_t name[22];	/* Sample_name */
};

struct xm_event {
	uint8_t note;		/* Note (0-71, 0 = C-0) */
	uint8_t instrument;	/* Instrument (0-128) */
	uint8_t volume;	/* Volume column byte */
	uint8_t fx_type;	/* Effect type */
	uint8_t fx_parm;	/* Effect parameter */
};

#endif
