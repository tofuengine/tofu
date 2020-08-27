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

/* Bounded reader macros.
 * If we attempt to read the buffer out-of-bounds, pretend that the buffer is
 * infinitely padded with zeroes.
 */
#define READ_U8(offset) (((offset) < moddata_length) ? (*(uint8_t*)(moddata + (offset))) : 0)
#define READ_U16(offset) ((uint16_t)READ_U8(offset) | ((uint16_t)READ_U8((offset) + 1) << 8))
#define READ_U32(offset) ((uint32_t)READ_U16(offset) | ((uint32_t)READ_U16((offset) + 2) << 16))
#define READ_MEMCPY(ptr, offset, length) memcpy_pad(ptr, length, moddata, moddata_length, offset)

static inline void memcpy_pad(void* dst, size_t dst_len, const void* src, size_t src_len, size_t offset) {
	uint8_t* dst_c = dst;
	const uint8_t* src_c = src;

	/* how many bytes can be copied without overrunning `src` */
	size_t copy_bytes = (src_len >= offset) ? (src_len - offset) : 0;
	copy_bytes = copy_bytes > dst_len ? dst_len : copy_bytes;

	memcpy(dst_c, src_c + offset, copy_bytes);
	/* padded bytes */
	memset(dst_c + copy_bytes, 0, dst_len - copy_bytes);
}

int xm_check_sanity_preload(const char* module, size_t module_length) {
	if(module_length < 60) {
		return 4;
	}

	if(memcmp("Extended Module: ", module, 17) != 0) {
		return 1;
	}

	if(module[37] != 0x1A) {
		return 2;
	}

	if(module[59] != 0x01 || module[58] != 0x04) {
		/* Not XM 1.04 */
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

size_t xm_get_memory_needed_for_context(const char* moddata, size_t moddata_length) {
	size_t memory_needed = 0;
	size_t offset = 60; /* Skip the first header */
	uint16_t num_channels;
	uint16_t num_patterns;
	uint16_t num_instruments;

	/* Read the module header */
	num_channels = READ_U16(offset + 8);

	num_patterns = READ_U16(offset + 10);
	memory_needed += num_patterns * sizeof(xm_pattern_t);

	num_instruments = READ_U16(offset + 12);
	memory_needed += num_instruments * sizeof(xm_instrument_t);

	memory_needed += MAX_NUM_ROWS * READ_U16(offset + 4) * sizeof(uint8_t); /* Module length */

	/* Header size */
	offset += READ_U32(offset);

	/* Read pattern headers */
	for(uint16_t i = 0; i < num_patterns; ++i) {
		uint16_t num_rows;

		num_rows = READ_U16(offset + 5);
		memory_needed += num_rows * num_channels * sizeof(xm_pattern_slot_t);

		/* Pattern header length + packed pattern data size */
		offset += READ_U32(offset) + READ_U16(offset + 7);
	}

	/* Read instrument headers */
	for(uint16_t i = 0; i < num_instruments; ++i) {
		uint16_t num_samples;
		uint32_t sample_header_size = 0;
		uint32_t sample_size_aggregate = 0;

		num_samples = READ_U16(offset + 27);
		memory_needed += num_samples * sizeof(xm_sample_t);

		if(num_samples > 0) {
			sample_header_size = READ_U32(offset + 29);
		}

		/* Instrument header size */
		offset += READ_U32(offset);

		for(uint16_t j = 0; j < num_samples; ++j) {
			uint32_t sample_size;

			sample_size = READ_U32(offset);
			sample_size_aggregate += sample_size;
			memory_needed += sample_size;
			offset += sample_header_size;
		}

		offset += sample_size_aggregate;
	}

	memory_needed += num_channels * sizeof(xm_channel_context_t);
	memory_needed += sizeof(xm_context_t);

	return memory_needed;
}

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
	uint8_t __fixed;
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

size_t xm_get_memory_needed_for_context_cb(xm_read_callback_t read, xm_seek_callback_t seek, void* user_data) {
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

			memory_needed += sample_header.length;

			seek(user_data, sample_header_size - sizeof(xm_sample_header_t), SEEK_CUR);

			instrument_samples_data_size += sample_header.length;
		}

		seek(user_data, instrument_samples_data_size, SEEK_CUR); // Skip sample data, located after the samples headers.
	}

	memory_needed += module_header.channels * sizeof(xm_channel_context_t);
	memory_needed += sizeof(xm_context_t);

	return memory_needed;
}

char* xm_load_module(xm_context_t* ctx, const char* moddata, size_t moddata_length, char* mempool) {
	size_t offset = 0;
	xm_module_t* mod = &(ctx->module);

	/* Read XM header */
#ifdef XM_STRINGS
	READ_MEMCPY(mod->name, offset + 17, MODULE_NAME_LENGTH);
	READ_MEMCPY(mod->trackername, offset + 38, TRACKER_NAME_LENGTH);
#endif
	offset += 60;

	/* Read module header */
	uint32_t header_size = READ_U32(offset);

	mod->length = READ_U16(offset + 4);
	mod->restart_position = READ_U16(offset + 6);
	mod->num_channels = READ_U16(offset + 8);
	mod->num_patterns = READ_U16(offset + 10);
	mod->num_instruments = READ_U16(offset + 12);

	mod->patterns = (xm_pattern_t*)mempool;
	mempool += mod->num_patterns * sizeof(xm_pattern_t);

	mod->instruments = (xm_instrument_t*)mempool;
	mempool += mod->num_instruments * sizeof(xm_instrument_t);

	uint16_t flags = READ_U32(offset + 14);
	mod->frequency_type = (flags & (1 << 0)) ? XM_LINEAR_FREQUENCIES : XM_AMIGA_FREQUENCIES;

	ctx->tempo = READ_U16(offset + 16);
	ctx->bpm = READ_U16(offset + 18);

	READ_MEMCPY(mod->pattern_table, offset + 20, PATTERN_ORDER_TABLE_LENGTH);
	offset += header_size;

	/* Read patterns */
	for(uint16_t i = 0; i < mod->num_patterns; ++i) {
		uint16_t packed_patterndata_size = READ_U16(offset + 7);
		xm_pattern_t* pat = mod->patterns + i;

		pat->num_rows = READ_U16(offset + 5);

		pat->slots = (xm_pattern_slot_t*)mempool;
		mempool += mod->num_channels * pat->num_rows * sizeof(xm_pattern_slot_t);

		/* Pattern header length */
		offset += READ_U32(offset);

		if(packed_patterndata_size == 0) {
			/* No pattern data is present */
			memset(pat->slots, 0, sizeof(xm_pattern_slot_t) * pat->num_rows * mod->num_channels);
		} else {
			/* This isn't your typical for loop */
			for(uint16_t j = 0, k = 0; j < packed_patterndata_size; ++k) {
				uint8_t note = READ_U8(offset + j);
				xm_pattern_slot_t* slot = pat->slots + k;

				if(note & (1 << 7)) {
					/* MSB is set, this is a compressed packet */
					++j;

					if(note & (1 << 0)) {
						/* Note follows */
						slot->note = READ_U8(offset + j);
						++j;
					} else {
						slot->note = 0;
					}

					if(note & (1 << 1)) {
						/* Instrument follows */
						slot->instrument = READ_U8(offset + j);
						++j;
					} else {
						slot->instrument = 0;
					}

					if(note & (1 << 2)) {
						/* Volume column follows */
						slot->volume_column = READ_U8(offset + j);
						++j;
					} else {
						slot->volume_column = 0;
					}

					if(note & (1 << 3)) {
						/* Effect follows */
						slot->effect_type = READ_U8(offset + j);
						++j;
					} else {
						slot->effect_type = 0;
					}

					if(note & (1 << 4)) {
						/* Effect parameter follows */
						slot->effect_param = READ_U8(offset + j);
						++j;
					} else {
						slot->effect_param = 0;
					}
				} else {
					/* Uncompressed packet */
					slot->note = note;
					slot->instrument = READ_U8(offset + j + 1);
					slot->volume_column = READ_U8(offset + j + 2);
					slot->effect_type = READ_U8(offset + j + 3);
					slot->effect_param = READ_U8(offset + j + 4);
					j += 5;
				}
			}
		}

		offset += packed_patterndata_size;
	}

	/* Read instruments */
	for(uint16_t i = 0; i < ctx->module.num_instruments; ++i) {
		uint32_t sample_header_size = 0;
		xm_instrument_t* instr = mod->instruments + i;

#ifdef XM_STRINGS
		READ_MEMCPY(instr->name, offset + 4, INSTRUMENT_NAME_LENGTH);
#endif
	    instr->num_samples = READ_U16(offset + 27);

		if(instr->num_samples > 0) {
			/* Read extra header properties */
			sample_header_size = READ_U32(offset + 29);
			READ_MEMCPY(instr->sample_of_notes, offset + 33, NUM_NOTES);

			instr->volume_envelope.num_points = READ_U8(offset + 225);
			instr->panning_envelope.num_points = READ_U8(offset + 226);

			for(uint8_t j = 0; j < instr->volume_envelope.num_points; ++j) {
				instr->volume_envelope.points[j].frame = READ_U16(offset + 129 + 4 * j);
				instr->volume_envelope.points[j].value = READ_U16(offset + 129 + 4 * j + 2);
			}

			for(uint8_t j = 0; j < instr->panning_envelope.num_points; ++j) {
				instr->panning_envelope.points[j].frame = READ_U16(offset + 177 + 4 * j);
				instr->panning_envelope.points[j].value = READ_U16(offset + 177 + 4 * j + 2);
			}

			instr->volume_envelope.sustain_point = READ_U8(offset + 227);
			instr->volume_envelope.loop_start_point = READ_U8(offset + 228);
			instr->volume_envelope.loop_end_point = READ_U8(offset + 229);

			instr->panning_envelope.sustain_point = READ_U8(offset + 230);
			instr->panning_envelope.loop_start_point = READ_U8(offset + 231);
			instr->panning_envelope.loop_end_point = READ_U8(offset + 232);

			flags = READ_U8(offset + 233);
			instr->volume_envelope.enabled = flags & (1 << 0);
			instr->volume_envelope.sustain_enabled = flags & (1 << 1);
			instr->volume_envelope.loop_enabled = flags & (1 << 2);

			flags = READ_U8(offset + 234);
			instr->panning_envelope.enabled = flags & (1 << 0);
			instr->panning_envelope.sustain_enabled = flags & (1 << 1);
			instr->panning_envelope.loop_enabled = flags & (1 << 2);

			instr->vibrato_type = READ_U8(offset + 235);
			if(instr->vibrato_type == 2) {
				instr->vibrato_type = 1;
			} else if(instr->vibrato_type == 1) {
				instr->vibrato_type = 2;
			}
			instr->vibrato_sweep = READ_U8(offset + 236);
			instr->vibrato_depth = READ_U8(offset + 237);
			instr->vibrato_rate = READ_U8(offset + 238);
			instr->volume_fadeout = READ_U16(offset + 239);

			instr->samples = (xm_sample_t*)mempool;
			mempool += instr->num_samples * sizeof(xm_sample_t);
		} else {
			instr->samples = NULL;
		}

		/* Instrument header size */
		offset += READ_U32(offset);

		for(uint16_t j = 0; j < instr->num_samples; ++j) {
			/* Read sample header */
			xm_sample_t* sample = instr->samples + j;

			sample->length = READ_U32(offset);
			sample->loop_start = READ_U32(offset + 4);
			sample->loop_length = READ_U32(offset + 8);
			sample->loop_end = sample->loop_start + sample->loop_length;
			sample->volume = (float)READ_U8(offset + 12) / (float)0x40;
			sample->finetune = (int8_t)READ_U8(offset + 13);

			flags = READ_U8(offset + 14);
			if((flags & 3) == 0) {
				sample->loop_type = XM_NO_LOOP;
			} else if((flags & 3) == 1) {
				sample->loop_type = XM_FORWARD_LOOP;
			} else {
				sample->loop_type = XM_PING_PONG_LOOP;
			}

			sample->bits = (flags & (1 << 4)) ? 16 : 8;

			sample->panning = (float)READ_U8(offset + 15) / (float)0xFF;
			sample->relative_note = (int8_t)READ_U8(offset + 16);
#ifdef XM_STRINGS
			READ_MEMCPY(sample->name, 18, SAMPLE_NAME_LENGTH);
#endif
			sample->data.as8 = (int8_t*)mempool;
			mempool += sample->length;

			if(sample->bits == 16) {
				sample->loop_start >>= 1;
				sample->loop_length >>= 1;
				sample->loop_end >>= 1;
				sample->length >>= 1;
			}

			offset += sample_header_size;
		}

		for(uint16_t j = 0; j < instr->num_samples; ++j) {
			/* Read sample data */
			xm_sample_t* sample = instr->samples + j;
			uint32_t length = sample->length;

			if(sample->bits == 16) {
				int16_t v = 0;
				for(uint32_t k = 0; k < length; ++k) {
					v = v + (int16_t)READ_U16(offset + (k << 1));
					sample->data.as16[k] = v;
				}
				offset += sample->length << 1;
			} else {
				int8_t v = 0;
				for(uint32_t k = 0; k < length; ++k) {
					v = v + (int8_t)READ_U8(offset + k);
					sample->data.as8[k] = v;
				}
				offset += sample->length;
			}
		}
	}

	return mempool;
}

char* xm_load_module_cb(xm_context_t* ctx, xm_read_callback_t read, xm_seek_callback_t seek, void* user_data, char* mempool) {
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

	mod->frequency_type = (module_header.flags & (1 << 0)) ? XM_LINEAR_FREQUENCIES : XM_AMIGA_FREQUENCIES;

	ctx->tempo = module_header.tempo;
	ctx->bpm = module_header.bpm;

	memcpy(mod->pattern_table, module_header.pattern_table, sizeof(uint8_t) * PATTERN_ORDER_TABLE_LENGTH);

	seek(user_data, module_header.header_size - sizeof(xm_header_t), SEEK_CUR);

	/* Read patterns */
	for(uint16_t i = 0; i < mod->num_patterns; ++i) {
		xm_pattern_header_t pattern_header;
		read(user_data, &pattern_header, sizeof(xm_pattern_header_t));

		uint16_t patterndata_size = pattern_header.data_size;
		xm_pattern_t* pat = mod->patterns + i;

		pat->num_rows = pattern_header.rows;

		pat->slots = (xm_pattern_slot_t*)mempool;
		mempool += mod->num_channels * pat->num_rows * sizeof(xm_pattern_slot_t);

		/* Pattern header length */
		seek(user_data, pattern_header.header_size - sizeof(xm_pattern_header_t), SEEK_CUR);

		/* This isn't your typical for loop */
		for (uint16_t j = 0; patterndata_size; ++j) {
			uint8_t note;
			read(user_data, &note, sizeof(uint8_t));

			xm_pattern_slot_t* slot = pat->slots + j;

			if(note & (1 << 7)) { /* MSB is set, this is a compressed packet */
				patterndata_size--;

				if(note & (1 << 0)) { /* Note follows */
					read(user_data, &slot->note, sizeof(uint8_t));
					patterndata_size--;
				} else {
					slot->note = 0;
				}

				if(note & (1 << 1)) { /* Instrument follows */
					read(user_data, &slot->instrument, sizeof(uint8_t));
					patterndata_size--;
				} else {
					slot->instrument = 0;
				}

				if(note & (1 << 2)) { /* Volume column follows */
					read(user_data, &slot->volume_column, sizeof(uint8_t));
					patterndata_size--;
				} else {
					slot->volume_column = 0;
				}

				if(note & (1 << 3)) { /* Effect follows */
					read(user_data, &slot->effect_type, sizeof(uint8_t));
					patterndata_size--;
				} else {
					slot->effect_type = 0;
				}

				if(note & (1 << 4)) { /* Effect parameter follows */
					read(user_data, &slot->effect_param, sizeof(uint8_t));
					patterndata_size--;
				} else {
					slot->effect_param = 0;
				}
			} else { /* Uncompressed packet */
				slot->note = note;
				read(user_data, &slot->instrument, sizeof(uint8_t));
				read(user_data, &slot->volume_column, sizeof(uint8_t));
				read(user_data, &slot->effect_type, sizeof(uint8_t));
				read(user_data, &slot->effect_param, sizeof(uint8_t));
				patterndata_size -= 5;
			}
		}
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

			sample->bits = (sample_header.flags & (1 << 4)) ? 16 : 8;

			sample->panning = (float)sample_header.panning / (float)0xFF;
			sample->relative_note = sample_header.relative_note;
#ifdef XM_STRINGS
			memcpy(sample->name, sample_header.name, SAMPLE_NAME_LENGTH);
#endif
			sample->data.as8 = (int8_t*)mempool;
			mempool += sample->length;

			if(sample->bits == 16) {
				sample->loop_start >>= 1;
				sample->loop_length >>= 1;
				sample->loop_end >>= 1;
				sample->length >>= 1;
			}

			seek(user_data, sample_header_size - sizeof(xm_sample_header_t), SEEK_CUR);
		}

		for(uint16_t j = 0; j < instr->num_samples; ++j) {
			/* Read sample data */
			xm_sample_t* sample = instr->samples + j;
			uint32_t length = sample->length;

			if(sample->bits == 16) {
				int16_t v = 0;
				for(uint32_t k = 0; k < length; ++k) {
					int16_t value;
					read(user_data, &value, sizeof(int16_t));

					v = v + value;
					sample->data.as16[k] = v;
				}
			} else {
				int8_t v = 0;
				for(uint32_t k = 0; k < length; ++k) {
					int8_t value;
					read(user_data, &value, sizeof(int8_t));

					v = v + value;
					sample->data.as8[k] = v;
				}
			}
		}
	}

	return mempool;
}
