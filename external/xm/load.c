/* Author: Romain "Artefact2" Dalmaso <artefact2@gmail.com> */
/* Contributor: Dan Spencer <dan@atomicpotato.net> */

/* This program is free software. It comes without any warranty, to the
 * extent permitted by applicable law. You can redistribute it and/or
 * modify it under the terms of the Do What The Fuck You Want To Public
 * License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */

#include "xm_internal.h"
#include "xm_headers.h"

#include <stdio.h>

/* .xm files are little-endian. (XXX: Are they really?) */

int xm_check_sanity_preload(xm_read_callback_t read, xm_seek_callback_t seek, void* user_data) {
	seek(user_data, 0, SEEK_SET);

	xm_info_t module_info;
	size_t bytes_read = read(user_data, &module_info, sizeof(xm_info_t));

	if (bytes_read < sizeof(xm_info_t)) {
		return 4;
	}

	if (memcmp("Extended Module: ", module_info.id, XM_MODULE_ID_LENGTH) != 0) {
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
	memory_needed += XM_MAX_PATTERN_ROWS * module_header.song_length * sizeof(uint8_t); /* Module length */

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

#define XM_PATTERN_BYTES_PER_ROW	5

#define XM_PATTERN_FLAG_COMPRESSED	0x80
#define XM_PATTERN_FLAG_NOTE		0x01
#define XM_PATTERN_FLAG_INSTRUMENT	0x02
#define XM_PATTERN_FLAG_VOLUME		0x04
#define XM_PATTERN_FLAG_EFFECT		0x08
#define XM_PATTERN_FLAG_PARAMETER	0x10

static void _read_pattern_data(xm_read_callback_t read, void *user_data, xm_pattern_t *pattern, size_t pattern_data_size) {
	uint8_t buffer[XM_PATTERN_BYTES_PER_ROW * XM_MAX_PATTERN_ROWS]; // Worst case, with unpacked data.
	read(user_data, buffer, pattern_data_size);

	uint8_t *cursor = buffer;
	uint8_t *end_of_data = buffer + pattern_data_size;

	for (xm_pattern_slot_t *slot = pattern->slots; cursor < end_of_data; ++slot) {
		uint8_t note = *(cursor++);

		if(note & XM_PATTERN_FLAG_COMPRESSED) {
			slot->note = note & XM_PATTERN_FLAG_NOTE ? *(cursor++) : 0;
			slot->instrument = note & XM_PATTERN_FLAG_INSTRUMENT ? *(cursor++) : 0;
			slot->volume_column = note & XM_PATTERN_FLAG_VOLUME ? *(cursor++) : 0;
			slot->effect_type = note & XM_PATTERN_FLAG_EFFECT ? *(cursor++) : 0;
			slot->effect_param = note & XM_PATTERN_FLAG_PARAMETER ? *(cursor++) : 0;
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

// https://github.com/kode54/dumb/blob/master/src/it/readxm.c

#define XM_MODULE_FLAG_LINEAR_FREQUENCY	0x0001

#define XM_SAMPLE_FLAG_NO_LOOP			0x00
#define XM_SAMPLE_FLAG_FORWARD_LOOP		0x01
#define XM_SAMPLE_FLAG_PINGPONG_LOOP	0x02
#define XM_SAMPLE_FLAG_16BIT			0x10
#define XM_SAMPLE_FLAG_STEREO			0x20	// Unused?

#define XM_ENVELOPE_FLAG_ON			0x01
#define XM_ENVELOPE_FLAG_SUSTAIN	0x02
#define XM_ENVELOPE_FLAG_LOOP		0x04

char* xm_load_module(xm_context_t* ctx, xm_read_callback_t read, xm_seek_callback_t seek, void* user_data, char* mempool) {
	xm_module_t* mod = &(ctx->module);

	/* Read XM header */
	seek(user_data, 0, SEEK_SET);

	xm_info_t module_info;
	read(user_data, &module_info, sizeof(xm_info_t));

	xm_header_t module_header;
	read(user_data, &module_header, sizeof(xm_header_t));

	memcpy(mod->name, module_info.module_name, XM_MODULE_NAME_LENGTH);
	memcpy(mod->trackername, module_info.tracker_name, XM_TRACKER_NAME_LENGTH);

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

		xm_instrument_t *instrument = mod->instruments + i;

		memcpy(instrument->name, instrument_header.name, XM_INSTRUMENT_NAME_LENGTH);

		instrument->num_samples = instrument_header.samples;

		size_t sample_header_size = 0;

		if(instrument->num_samples > 0) {
			/* Read extra header properties */
			xm_instrument_header_ex_t instrument_header_ex;
			read(user_data, &instrument_header_ex, sizeof(xm_instrument_header_ex_t));

			sample_header_size = instrument_header_ex.sample_header_size;

			memcpy(instrument->sample_of_notes, instrument_header_ex.sample_number, XM_NOTES_AMOUNT);

			instrument->volume_envelope.num_points = instrument_header_ex.volume_points_number;
			instrument->panning_envelope.num_points = instrument_header_ex.panning_points_number;

			for(uint8_t j = 0; j < instrument->volume_envelope.num_points; ++j) {
				instrument->volume_envelope.points[j].frame = instrument_header_ex.volume_points[j].frame;
				instrument->volume_envelope.points[j].value = instrument_header_ex.volume_points[j].value;
			}

			for(uint8_t j = 0; j < instrument->panning_envelope.num_points; ++j) {
				instrument->panning_envelope.points[j].frame = instrument_header_ex.panning_points[j].frame;
				instrument->panning_envelope.points[j].value = instrument_header_ex.panning_points[j].value;
			}

			instrument->volume_envelope.sustain_point = instrument_header_ex.volume_sustain_point;
			instrument->volume_envelope.loop_start_point = instrument_header_ex.volume_loop_start_point;
			instrument->volume_envelope.loop_end_point = instrument_header_ex.volume_loop_end_point;

			instrument->panning_envelope.sustain_point = instrument_header_ex.panning_sustain_point;
			instrument->panning_envelope.loop_start_point = instrument_header_ex.panning_loop_start_point;
			instrument->panning_envelope.loop_end_point = instrument_header_ex.panning_loop_end_point;

			instrument->volume_envelope.enabled = instrument_header_ex.volume_type & XM_ENVELOPE_FLAG_ON;
			instrument->volume_envelope.sustain_enabled = instrument_header_ex.volume_type & XM_ENVELOPE_FLAG_SUSTAIN;
			instrument->volume_envelope.loop_enabled = instrument_header_ex.volume_type & XM_ENVELOPE_FLAG_LOOP;

			instrument->panning_envelope.enabled = instrument_header_ex.panning_type & XM_ENVELOPE_FLAG_ON;
			instrument->panning_envelope.sustain_enabled = instrument_header_ex.panning_type & XM_ENVELOPE_FLAG_SUSTAIN;
			instrument->panning_envelope.loop_enabled = instrument_header_ex.panning_type & XM_ENVELOPE_FLAG_LOOP;

			instrument->vibrato_type = instrument_header_ex.vibrato_type;
			instrument->vibrato_sweep = instrument_header_ex.vibrato_sweep;
			instrument->vibrato_depth = instrument_header_ex.vibrato_depth;
			instrument->vibrato_rate = instrument_header_ex.vibrato_rate;
			instrument->volume_fadeout = instrument_header_ex.volume_fadeout;

			instrument->samples = (xm_sample_t*)mempool;
			mempool += instrument->num_samples * sizeof(xm_sample_t);
		} else {
			instrument->samples = NULL;
		}

		/* Instrument header size */
		int offset = instrument_header.header_size - sizeof(xm_instrument_header_t);
		if (instrument_header.samples > 0) {
			offset -= sizeof(xm_instrument_header_ex_t);
		}
		seek(user_data, offset, SEEK_CUR);

		for (uint16_t j = 0; j < instrument->num_samples; ++j) {
			/* Read sample header */
			xm_sample_header_t sample_header;
			read(user_data, &sample_header, sizeof(xm_sample_header_t));

			xm_sample_t* sample = instrument->samples + j;

			memcpy(sample->name, sample_header.name, XM_SAMPLE_NAME_LENGTH);

			sample->bytes_per_sample = (sample_header.flags & XM_SAMPLE_FLAG_16BIT) ? 2 : 1;

			sample->length = sample_header.length / sample->bytes_per_sample; // Convert from bytes to samples.
			sample->loop_start = sample_header.loop_start / sample->bytes_per_sample;
			sample->loop_length = sample_header.loop_end / sample->bytes_per_sample;
			sample->loop_end = sample->loop_start + sample->loop_length;
			sample->volume = (float)sample_header.volume / (float)0x40;
			sample->finetune = sample_header.finetune;

			if (sample_header.flags & XM_SAMPLE_FLAG_PINGPONG_LOOP) { // bit #2 is `1` only for ping-pong loop.
				sample->loop_type = XM_PING_PONG_LOOP;
			} else if (sample_header.flags & XM_SAMPLE_FLAG_FORWARD_LOOP) { // since bit #2 is `0` there's only that loop left.
				sample->loop_type = XM_FORWARD_LOOP;
			} else {
				sample->loop_type = XM_NO_LOOP;
			}

			sample->panning = (float)sample_header.panning / (float)0xFF;
			sample->relative_note = sample_header.relative_note;

			sample->data = (void*)mempool;
			mempool += sample->length << 1;

			seek(user_data, sample_header_size - sizeof(xm_sample_header_t), SEEK_CUR);
		}

		for (uint16_t j = 0; j < instrument->num_samples; ++j) { /* Read sample data */
			xm_sample_t *sample = instrument->samples + j;

			_delta_decode(read, user_data, sample->data, sample->length, sample->bytes_per_sample);
		}
	}

	return mempool;
}
