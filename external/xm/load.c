/* Author: Romain "Artefact2" Dalmaso <artefact2@gmail.com> */
/* Contributor: Dan Spencer <dan@atomicpotato.net> */

/* This program is free software. It comes without any warranty, to the
 * extent permitted by applicable law. You can redistribute it and/or
 * modify it under the terms of the Do What The Fuck You Want To Public
 * License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */

#include "xm_internal.h"

#include <stdio.h>

/* .xm files are little-endian. (XXX: Are they really?) */

/*
HEADER
======

Offset|Length| Type   | Description
------+------+--------+--------------------------------------------
   0  |  17  | (char) | ID text: 'Extended Module: '
      |      |        | (The last character is space, i.e. $20)
  17  |  20  | (char) | Module name, padded with zeros.
  37  |   1  | (char) | Always $1a
  38  |  20  | (char) | Tracker name
  58  |   2  | (word) | Version number, hi-byte major and low-byte minor
      |      |        | The current format is version $0104. Format
      |      |        | versions below $0104 have a LOT of differences.
      |      |        | Remember to check this field! Your loader will
      |      |        | probably crash if you don't!
      |      |        |
  60  |   4  | (dword)| Header size
      |      |        | Calculated FROM THIS OFFSET, NOT from
      |      |        | the beginning of the file!
  +4  |   2  | (word) | Song length (in pattern order table)
  +6  |   2  | (word) | Song restart position
  +8  |   2  | (word) | Number of channels (2, 4, 6, 8, 10, ..., 32)
 +10  |   2  | (word) | Number of patterns (max 256)
      |      |        | NOTICE: This might not include all patterns used!
      |      |        | If empty patterns are used at the end of the song
      |      |        | they are NOT saved to the file!!
 +12  |   2  | (word) | Number of instruments (max 128)
 +14  |   2  | (word) | Flags field: 
      |      |        | bit0: 0 = Amiga frequency table
      |      |        |       1 = Linear frequency table
 +16  |   2  | (word) | Default tempo
 +18  |   2  | (word) | Default BPM
 +20  | 256  | (byte) | Pattern order table
*/
#define MODULE_ID_LENGTH	17

#pragma pack(push, 1)
typedef struct _xm_info_t {
	char id[MODULE_ID_LENGTH];
	char module_name[MODULE_NAME_LENGTH];
	uint8_t magic;
	char tracker_name[TRACKER_NAME_LENGTH];
	uint16_t version_number;
} xm_info_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _xm_header_t {
	uint32_t header_size;
	uint16_t song_length;
	uint16_t song_restart_position;
	uint16_t channels;
	uint16_t patterns;
	uint16_t instruments;
	uint16_t flags;
	uint16_t tempo;
	uint16_t bpm;
	uint8_t pattern_table[PATTERN_ORDER_TABLE_LENGTH];
} xm_header_t;
#pragma pack(pop)

/*
PATTERNS
========

Offset|Length| Type   | Description
------+------+--------+--------------------------------------------
   ?  |   4  | (dword)| Pattern header length
  +4  |   1  | (byte) | Packing type (always 0)
  +5  |   2  | (word) | Number of rows in pattern (1..256)
  +7  |   2  | (word) | Packed patterndata size
      |      |        | << Note! This is zero if the pattern is
      |      |        | completely empty and no pattern data
      |      |        | follows! >>
      |      |        |
   ?  |   ?  |        | Packed pattern data.
*/
#pragma pack(push, 1)
typedef struct _xm_pattern_header_t {
	uint32_t header_size;
	uint8_t packing_type;
	uint16_t rows;
	uint16_t data_size;
} xm_pattern_header_t;
#pragma pack(pop)

/*
INSTRUMENTS
===========

Offset|Length| Type   | Description
------+------+--------+--------------------------------------------
   ?  |   4  | (dword)| Instrument HEADER size (SEE THE NOTICE BELOW)
  +4  |  22  | (char) | Instrument name
 +26  |   1  | (byte) | Instrument type (always 0)
      |      |        | << In reality, this seems pretty random,
      |      |        |    so DON'T assume that it's zero. >>
      |      |        |
 +27  |   2  | (word) | Number of samples in instrument.
PATTERNS
========

Offset|Length| Type   | Description
------+------+--------+--------------------------------------------
   ?  |   4  | (dword)| Pattern header length
  +4  |   1  | (byte) | Packing type (always 0)
  +5  |   2  | (word) | Number of rows in pattern (1..256)
  +7  |   2  | (word) | Packed patterndata size
      |      |        | << Note! This is zero if the pattern is
      |      |        | completely empty and no pattern data
      |      |        | follows! >>
      |      |        |
   ?  |   ?  |        | Packed pattern data.

INSTRUMENTS
===========

Offset|Length| Type   | Description
------+------+--------+--------------------------------------------
   ?  |   4  | (dword)| Instrument HEADER size (SEE THE NOTICE BELOW)
  +4  |  22  | (char) | Instrument name
 +26  |   1  | (byte) | Instrument type (always 0)
      |      |        | << In reality, this seems pretty random,
      |      |        |    so DON'T assume that it's zero. >>
      |      |        |
 +27  |   2  | (word) | Number of samples in instrument.

NOTICE: The "Instrument HEADER Size" field tends to be more than the actual
	size of the structure documented here (it includes also the
        extended instrument sample header below). So remember to check
	it and skip the additional bytes before the first sample header!

If the number of samples is greater than zero, then this will follow:
(if it is zero, nothing will follow!)

Offset|Length| Type   | Description
------+------+--------+--------------------------------------------
  +29 |   4  | (dword)| Sample header size
  +33 |  96  | (byte) | Sample number for all notes
 +129 |  48  | (byte) | Points for volume envelope
 +177 |  48  | (byte) | Points for panning envelope
 +225 |   1  | (byte) | Number of volume points
 +226 |   1  | (byte) | Number of panning points
 +227 |   1  | (byte) | Volume sustain point
 +228 |   1  | (byte) | Volume loop start point
 +229 |   1  | (byte) | Volume loop end point
 +230 |   1  | (byte) | Panning sustain point
 +231 |   1  | (byte) | Panning loop start point
 +232 |   1  | (byte) | Panning loop end point
 +233 |   1  | (byte) | Volume type: bit 0: On; 1: Sustain; 2: Loop
 +234 |   1  | (byte) | Panning type: bit 0: On; 1: Sustain; 2: Loop
 +235 |   1  | (byte) | Vibrato type
 +236 |   1  | (byte) | Vibrato sweep
 +237 |   1  | (byte) | Vibrato depth
 +238 |   1  | (byte) | Vibrato rate
 +239 |   2  | (word) | Volume fadeout
 +241 |   2  | (word) | Reserved
*/
#pragma pack(push, 1)
typedef struct _xm_instrument_header_t {
	uint32_t header_size;
	char name[INSTRUMENT_NAME_LENGTH];
	uint8_t type;
	uint16_t samples;
} xm_instrument_header_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _xm_instrument_header_evenlope_point_t {
	uint16_t frame;
	uint16_t value;
} xm_instrument_header_evenlope_point_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _xm_instrument_header_ex_t {
	uint32_t sample_header_size;
	uint8_t sample_number[NUM_NOTES];
	xm_instrument_header_evenlope_point_t volume_points[NUM_ENVELOPE_POINTS];
	xm_instrument_header_evenlope_point_t panning_points[NUM_ENVELOPE_POINTS];
	uint8_t volume_points_number;
	uint8_t panning_points_number;
	uint8_t volume_sustain_point;
	uint8_t volume_loop_start_point;
	uint8_t volume_loop_end_point;
	uint8_t panning_sustain_point;
	uint8_t panning_loop_start_point;
	uint8_t panning_loop_end_point;
	uint8_t volume_type;
	uint8_t panning_type;
	uint8_t vibrato_type;
	uint8_t vibrato_sweep;
	uint8_t vibrato_depth;
	uint8_t vibrato_rate;
	uint16_t volume_fadeout;
	uint16_t __reserved;
} xm_instrument_header_ex_t;
#pragma pack(pop)

/*
SAMPLE HEADERS
==============
"Number of samples" of these will follow after the instrument header.
See also the XM file general layout in the beginning of this file to
understand better/get the big picture.

Offset|Length| Type   | Description
------+------+--------+--------------------------------------------
   ?  |  4   | (dword)| Sample length
  +4  |  4   | (dword)| Sample loop start
  +8  |  4   | (dword)| Sample loop length
 +12  |  1   | (byte) | Volume
 +13  |  1   | (byte) | Finetune (signed byte -16..+15)
 +14  |  1   | (byte) | Type of sample, bit meanings:
      |      |        | 0-1: 00 = 0 dec = No loop,
      |      |        |      01 = 1 dec = Forward loop,
      |      |        |      11 = 2 dec = Ping-pong loop;
      |      |        |   4: 16-bit sampledata
 +15  |  1   | (byte) | Panning (0-255)
 +16  |  1   | (byte) | Relative note number (signed byte)
 +17  |  1   | (byte) | Reserved
 +18  | 22   | (char) | Sample name

NOTICE: Note! After the instrument header the file contains
	ALL sample headers for the instrument followed by the
        sample data for all samples.

        Also note that it is possible that samples have loops with
        length zero. In that case the loops just have to be skipped.
*/
#pragma pack(push, 1)
typedef struct _xm_sample_header_t {
	uint32_t length;
	uint32_t loop_start;
	uint32_t loop_end;
	uint8_t volume;
	uint8_t finetune;
	uint8_t flags;
	uint8_t panning;
	uint8_t relative_note;
	uint8_t __reserved;
	char name[SAMPLE_NAME_LENGTH];
} xm_sample_header_t;
#pragma pack(pop)

int xm_check_sanity_preload(xm_read_callback_t read, xm_seek_callback_t seek, void* user_data) {
	seek(user_data, 0, SEEK_SET);

	xm_info_t module_info;
	size_t bytes_read = read(user_data, &module_info, sizeof(xm_info_t));

	if (bytes_read < 60) {
		return 4;
	}

	if (memcmp("Extended Module: ", module_info.id, MODULE_ID_LENGTH) != 0) {
		return 1;
	}

	if (module_info.magic != 0x1A) {
		return 2;
	}

	if (module_info.version_number != 0x0104) { /* Not XM 1.04 */
		return 3;
	}

	return 0;
}

int xm_check_sanity_postload(xm_context_t* ctx) {
	/* @todo: plenty of stuff to do here… */

	/* Check the POT */
	for(uint8_t i = 0; i < ctx->module.length; ++i) {
		if(ctx->module.pattern_table[i] >= ctx->module.num_patterns) {
			if(i+1 == ctx->module.length && ctx->module.length > 1) {
				/* Cheap fix */
				--ctx->module.length;
				XM_DEBUG_OUT("trimming invalid POT at pos %X", i);
			} else {
				XM_DEBUG_OUT("module has invalid POT, pos %X references nonexistent pattern %X",
				      i,
				      ctx->module.pattern_table[i]);
				return 1;
			}
		}
	}

	return 0;
}

size_t xm_get_memory_needed_for_context(xm_read_callback_t read, xm_seek_callback_t seek, void* user_data) {
	size_t memory_needed = 0;

	seek(user_data, 0, SEEK_SET);

	xm_info_t module_info;
	read(user_data, &module_info, sizeof(xm_info_t));

	xm_header_t module_header;
	read(user_data, &module_header, sizeof(xm_header_t));

	memory_needed += module_header.patterns * sizeof(xm_pattern_t);
	memory_needed += module_header.instruments * sizeof(xm_instrument_t);
	memory_needed += MAX_NUM_ROWS * module_header.song_length * sizeof(uint8_t); /* Module length */

	seek(user_data, module_header.header_size - sizeof(xm_header_t), SEEK_CUR);

	for (uint16_t i = 0; i < module_header.patterns; ++i) {
		xm_pattern_header_t pattern_header;
		read(user_data, &pattern_header, sizeof(xm_pattern_header_t));

		memory_needed += pattern_header.rows * module_header.channels * sizeof(xm_pattern_slot_t);

		seek(user_data, pattern_header.header_size + pattern_header.data_size - sizeof(xm_pattern_header_t), SEEK_CUR);
	}

	/* Read instrument headers */
	for (uint16_t i = 0; i < module_header.instruments; ++i) {
		xm_instrument_header_t instrument_header;
		read(user_data, &instrument_header, sizeof(xm_instrument_header_t));

		size_t sample_header_size = 0;

		if (instrument_header.samples > 0) {
			xm_instrument_header_ex_t instrument_header_ex;
			read(user_data, &instrument_header_ex, sizeof(xm_instrument_header_ex_t));

			sample_header_size = instrument_header_ex.sample_header_size;
		}

		int offset = instrument_header.header_size - sizeof(xm_instrument_header_t);
		if (instrument_header.samples > 0) {
			offset -= sizeof(xm_instrument_header_ex_t);
		}
		seek(user_data, offset, SEEK_CUR);

		memory_needed += instrument_header.samples * sizeof(xm_sample_t);

		uint32_t instrument_samples_data_size = 0;

		for(uint16_t j = 0; j < instrument_header.samples; ++j) {
			xm_sample_header_t sample_header;
			read(user_data, &sample_header, sizeof(xm_sample_header_t));

			memory_needed += sample_header.length << 1; // Internally stored as 16 bit data.

			seek(user_data, sample_header_size - sizeof(xm_sample_header_t), SEEK_CUR);

			instrument_samples_data_size += sample_header.length;
		}

		seek(user_data, instrument_samples_data_size, SEEK_CUR); // Skip sample data, located after the samples headers.
	}

	memory_needed += module_header.channels * sizeof(xm_channel_context_t);
	memory_needed += sizeof(xm_context_t);

	return memory_needed;
}

#define PATTERN_BYTES_PER_ROW		5

#define XM_PATTERN_FLAG_COMPRESSED	0x80
#define XM_PATTERN_FLAG_NOTE		0x01
#define XM_PATTERN_FLAG_INSTRUMENT	0x02
#define XM_PATTERN_FLAG_VOLUME		0x04
#define XM_PATTERN_FLAG_EFFECT		0x08
#define XM_PATTERN_FLAG_PARAMETER	0x10

static void _read_pattern_data(xm_read_callback_t read, void *user_data, xm_pattern_t *pattern, size_t pattern_data_size) {
	uint8_t buffer[PATTERN_BYTES_PER_ROW * MAX_NUM_ROWS]; // Worst case, with unpacked data.
	read(user_data, buffer, pattern_data_size);

	uint8_t *cursor = buffer;
	uint8_t *end_of_data = buffer + pattern_data_size;

	for (xm_pattern_slot_t *slot = pattern->slots; cursor < end_of_data; ++slot) {
		uint8_t note = *(cursor++);

		if(note & XM_PATTERN_FLAG_COMPRESSED) {
			if(note & XM_PATTERN_FLAG_NOTE) {
				slot->note = *(cursor++);
			} else {
				slot->note = 0;
			}

			if(note & XM_PATTERN_FLAG_INSTRUMENT) {
				slot->instrument = *(cursor++);
			} else {
				slot->instrument = 0;
			}

			if(note & XM_PATTERN_FLAG_VOLUME) {
				slot->volume_column = *(cursor++);
			} else {
				slot->volume_column = 0;
			}

			if(note & XM_PATTERN_FLAG_EFFECT) {
				slot->effect_type = *(cursor++);
			} else {
				slot->effect_type = 0;
			}

			if(note & XM_PATTERN_FLAG_PARAMETER) {
				slot->effect_param = *(cursor++);
			} else {
				slot->effect_param = 0;
			}
		} else {
			slot->note = note;
			slot->instrument = *(cursor++);
			slot->volume_column = *(cursor++);
			slot->effect_type = *(cursor++);
			slot->effect_param = *(cursor++);
		}
	}
}

#define DELTA_BUFFER_SIZE	2048

// https://en.wikipedia.org/wiki/Delta_encoding
static void _delta_decode(xm_read_callback_t read, void *user_data, int16_t *output, size_t length, size_t bytes_per_value) {
	uint8_t buffer[DELTA_BUFFER_SIZE];

	int16_t value = 0;

	size_t remaining = length * bytes_per_value;
	while (remaining > 0) {
		size_t bytes_to_read = remaining > DELTA_BUFFER_SIZE ? DELTA_BUFFER_SIZE : remaining;
		size_t bytes_read = read(user_data, buffer, bytes_to_read);

		for (size_t i = 0; i < bytes_read; i += bytes_per_value) {
			int16_t v;
			if (bytes_per_value == 1) {
				v = *((int8_t *)(buffer + i)) << 8; // Convert the 8 bit sample to 16 bit.
			} else {
				v = *((int16_t *)(buffer + i));
			}
			value += v;
			*(output++) = value;
		}

		remaining -= bytes_read;
	}
}

#define XM_MODULE_FLAG_LINEAR_FREQUENCY	0x0001

#define XM_SAMPLE_MASK_LOOPMODE	0x03
#define XM_SAMPLE_FLAG_IS16BIT	0x10

char* xm_load_module(xm_context_t* ctx, xm_read_callback_t read, xm_seek_callback_t seek, void* user_data, char* mempool) {
	xm_module_t* mod = &(ctx->module);

	/* Read XM header */
	seek(user_data, 0, SEEK_SET);

	xm_info_t module_info;
	read(user_data, &module_info, sizeof(xm_info_t));

	xm_header_t module_header;
	read(user_data, &module_header, sizeof(xm_header_t));

#ifdef XM_STRINGS
	memcpy(mod->name, module_info.module_name, MODULE_NAME_LENGTH);
	memcpy(mod->trackername, module_info.tracker_name, TRACKER_NAME_LENGTH);
#endif

	/* Read module header */
	mod->length = module_header.song_length;
	mod->restart_position = module_header.song_restart_position;
	mod->num_channels = module_header.channels;
	mod->num_patterns = module_header.patterns;
	mod->num_instruments = module_header.instruments;

	mod->patterns = (xm_pattern_t*)mempool;
	mempool += mod->num_patterns * sizeof(xm_pattern_t);

	mod->instruments = (xm_instrument_t*)mempool;
	mempool += mod->num_instruments * sizeof(xm_instrument_t);

	mod->frequency_type = (module_header.flags & XM_MODULE_FLAG_LINEAR_FREQUENCY) ? XM_LINEAR_FREQUENCIES : XM_AMIGA_FREQUENCIES;

	ctx->tempo = module_header.tempo;
	ctx->bpm = module_header.bpm;

	memcpy(mod->pattern_table, module_header.pattern_table, sizeof(uint8_t) * PATTERN_ORDER_TABLE_LENGTH);

	seek(user_data, module_header.header_size - sizeof(xm_header_t), SEEK_CUR);

	/* Read patterns */
	for(uint16_t i = 0; i < mod->num_patterns; ++i) {
		xm_pattern_header_t pattern_header;
		read(user_data, &pattern_header, sizeof(xm_pattern_header_t));

		xm_pattern_t* pat = mod->patterns + i;

		pat->num_rows = pattern_header.rows;

		pat->slots = (xm_pattern_slot_t*)mempool;
		mempool += mod->num_channels * pat->num_rows * sizeof(xm_pattern_slot_t);

		/* Pattern header length */
		seek(user_data, pattern_header.header_size - sizeof(xm_pattern_header_t), SEEK_CUR);

		_read_pattern_data(read, user_data, pat, pattern_header.data_size);
	}

	/* Read instruments */
	for(uint16_t i = 0; i < ctx->module.num_instruments; ++i) {
		xm_instrument_header_t instrument_header;
		read(user_data, &instrument_header, sizeof(xm_instrument_header_t));

		xm_instrument_t* instr = mod->instruments + i;

#ifdef XM_STRINGS
		memcpy(instr->name, instrument_header.name, INSTRUMENT_NAME_LENGTH);
#endif
		instr->num_samples = instrument_header.samples;

		size_t sample_header_size = 0;

		if(instr->num_samples > 0) {
			/* Read extra header properties */
			xm_instrument_header_ex_t instrument_header_ex;
			read(user_data, &instrument_header_ex, sizeof(xm_instrument_header_ex_t));

			sample_header_size = instrument_header_ex.sample_header_size;

			memcpy(instr->sample_of_notes, instrument_header_ex.sample_number, NUM_NOTES);

			instr->volume_envelope.num_points = instrument_header_ex.volume_points_number;
			instr->panning_envelope.num_points = instrument_header_ex.panning_points_number;

			for(uint8_t j = 0; j < instr->volume_envelope.num_points; ++j) {
				instr->volume_envelope.points[j].frame = instrument_header_ex.volume_points[j].frame;
				instr->volume_envelope.points[j].value = instrument_header_ex.volume_points[j].value;
			}

			for(uint8_t j = 0; j < instr->panning_envelope.num_points; ++j) {
				instr->panning_envelope.points[j].frame = instrument_header_ex.panning_points[j].frame;
				instr->panning_envelope.points[j].value = instrument_header_ex.panning_points[j].value;
			}

			instr->volume_envelope.sustain_point = instrument_header_ex.volume_sustain_point;
			instr->volume_envelope.loop_start_point = instrument_header_ex.volume_loop_start_point;
			instr->volume_envelope.loop_end_point = instrument_header_ex.volume_loop_end_point;

			instr->panning_envelope.sustain_point = instrument_header_ex.panning_sustain_point;
			instr->panning_envelope.loop_start_point = instrument_header_ex.panning_loop_start_point;
			instr->panning_envelope.loop_end_point = instrument_header_ex.panning_loop_end_point;

			instr->volume_envelope.enabled = instrument_header_ex.volume_type & (1 << 0);
			instr->volume_envelope.sustain_enabled = instrument_header_ex.volume_type & (1 << 1);
			instr->volume_envelope.loop_enabled = instrument_header_ex.volume_type & (1 << 2);

			instr->panning_envelope.enabled = instrument_header_ex.panning_type & (1 << 0);
			instr->panning_envelope.sustain_enabled = instrument_header_ex.panning_type & (1 << 1);
			instr->panning_envelope.loop_enabled = instrument_header_ex.panning_type & (1 << 2);

			instr->vibrato_type = instrument_header_ex.vibrato_type;
			if(instr->vibrato_type == 2) {
				instr->vibrato_type = 1;
			} else if(instr->vibrato_type == 1) {
				instr->vibrato_type = 2;
			}
			instr->vibrato_sweep = instrument_header_ex.vibrato_sweep;
			instr->vibrato_depth = instrument_header_ex.vibrato_depth;
			instr->vibrato_rate = instrument_header_ex.vibrato_rate;
			instr->volume_fadeout = instrument_header_ex.volume_fadeout;

			instr->samples = (xm_sample_t*)mempool;
			mempool += instr->num_samples * sizeof(xm_sample_t);
		} else {
			instr->samples = NULL;
		}

		/* Instrument header size */
		int offset = instrument_header.header_size - sizeof(xm_instrument_header_t);
		if (instrument_header.samples > 0) {
			offset -= sizeof(xm_instrument_header_ex_t);
		}
		seek(user_data, offset, SEEK_CUR);

		for(uint16_t j = 0; j < instr->num_samples; ++j) {
			/* Read sample header */
			xm_sample_header_t sample_header;
			read(user_data, &sample_header, sizeof(xm_sample_header_t));

			xm_sample_t* sample = instr->samples + j;

			sample->length = sample_header.length;
			sample->loop_start = sample_header.loop_start;
			sample->loop_length = sample_header.loop_end;
			sample->loop_end = sample->loop_start + sample->loop_length;
			sample->volume = (float)sample_header.volume / (float)0x40;
			sample->finetune = sample_header.finetune;

			if((sample_header.flags & 3) == 0) {
				sample->loop_type = XM_NO_LOOP;
			} else if((sample_header.flags & 3) == 1) {
				sample->loop_type = XM_FORWARD_LOOP;
			} else {
				sample->loop_type = XM_PING_PONG_LOOP;
			}

			sample->bytes_per_sample = (sample_header.flags & XM_SAMPLE_FLAG_IS16BIT) ? 2 : 1;

			sample->panning = (float)sample_header.panning / (float)0xFF;
			sample->relative_note = sample_header.relative_note;
#ifdef XM_STRINGS
			memcpy(sample->name, sample_header.name, SAMPLE_NAME_LENGTH);
#endif
			sample->data = (void*)mempool;
			mempool += sample->length << 1;

			if(sample->bytes_per_sample == 2) {
				sample->loop_start >>= 1;
				sample->loop_length >>= 1;
				sample->loop_end >>= 1;
				sample->length >>= 1;
			}

			seek(user_data, sample_header_size - sizeof(xm_sample_header_t), SEEK_CUR);
		}

		for (uint16_t j = 0; j < instr->num_samples; ++j) { /* Read sample data */
			xm_sample_t* sample = instr->samples + j;

			_delta_decode(read, user_data, sample->data, sample->length, sample->bytes_per_sample);
		}
	}

	return mempool;
}
