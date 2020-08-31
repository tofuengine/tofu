/* Author: Romain "Artefact2" Dalmaso <artefact2@gmail.com> */

/* This program is free software. It comes without any warranty, to the
 * extent permitted by applicable law. You can redistribute it and/or
 * modify it under the terms of the Do What The Fuck You Want To Public
 * License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */
#pragma once

#include "xm.h"
#include "xm_config.h"
#include <stdbool.h>
#include <math.h>
#include <string.h>

#ifdef XM_DEBUG
#include <stdio.h>
#define XM_DEBUG_OUT(fmt, ...) do {										\
		fprintf(stderr, "%s(): " fmt "\n", __func__, __VA_ARGS__);	\
		fflush(stderr);												\
	} while(0)
#else
#define XM_DEBUG_OUT(...)
#endif

#ifdef XM_BIG_ENDIAN
#error "Big endian platforms are not yet supported, sorry"
/* Make sure the compiler stops, even if #error is ignored */
extern int __fail[-1];
#endif

/* ----- XM constants ----- */

#define XM_MODULE_ID_LENGTH	17

#define XM_SAMPLE_NAME_LENGTH 22
#define XM_INSTRUMENT_NAME_LENGTH 22
#define XM_MODULE_NAME_LENGTH 20
#define XM_TRACKER_NAME_LENGTH 20

#define PATTERN_ORDER_TABLE_LENGTH 256
#define XM_NOTES_AMOUNT	96
#define XM_MAX_ENVELOPE_POINTS 12
#define XM_MAX_PATTERN_ROWS 256

/* ----- Data types ----- */

enum xm_waveform_type_e {
	XM_SINE_WAVEFORM = 0,
	XM_SQUARE_WAVEFORM = 1,
	XM_RAMP_DOWN_WAVEFORM = 2,
	XM_RAMP_UP_WAVEFORM = 3,
	XM_RANDOM_WAVEFORM = 4
};
typedef enum xm_waveform_type_e xm_waveform_type_t;

enum xm_loop_type_e {
	XM_NO_LOOP,
	XM_FORWARD_LOOP,
	XM_PING_PONG_LOOP,
};
typedef enum xm_loop_type_e xm_loop_type_t;

enum xm_frequency_type_e {
	XM_LINEAR_FREQUENCIES,
	XM_AMIGA_FREQUENCIES,
};
typedef enum xm_frequency_type_e xm_frequency_type_t;

struct xm_envelope_point_s {
	uint16_t frame;
	uint16_t value;
};
typedef struct xm_envelope_point_s xm_envelope_point_t;

struct xm_envelope_s {
	xm_envelope_point_t points[XM_MAX_ENVELOPE_POINTS];
	uint8_t num_points;
	uint8_t sustain_point;
	uint8_t loop_start_point;
	uint8_t loop_end_point;
	bool enabled;
	bool sustain_enabled;
	bool loop_enabled;
};
typedef struct xm_envelope_s xm_envelope_t;

struct xm_sample_s {
	char name[XM_SAMPLE_NAME_LENGTH + 1];

	uint8_t bytes_per_sample;

	uint32_t length;
	uint32_t loop_start;
	uint32_t loop_length;
	uint32_t loop_end;
	float volume;
	int8_t finetune;
	xm_loop_type_t loop_type;
	float panning;
	int8_t relative_note;
	uint64_t latest_trigger;

	int16_t* data; // Sample data is always internally stored as s16.
};
typedef struct xm_sample_s xm_sample_t;

struct xm_instrument_s {
	char name[XM_INSTRUMENT_NAME_LENGTH + 1];

	uint16_t num_samples;
	uint8_t sample_of_notes[XM_NOTES_AMOUNT];
	xm_envelope_t volume_envelope;
	xm_envelope_t panning_envelope;
	xm_waveform_type_t vibrato_type;
	uint8_t vibrato_sweep;
	uint8_t vibrato_depth;
	uint8_t vibrato_rate;
	uint16_t volume_fadeout;
	uint64_t latest_trigger;
	bool muted;

	xm_sample_t* samples;
};
typedef struct xm_instrument_s xm_instrument_t;

struct xm_pattern_slot_s {
	uint8_t note; /* 1-96, 97 = Key Off note */
	uint8_t instrument; /* 1-128 */
	uint8_t volume_column;
	uint8_t effect_type;
	uint8_t effect_param;
};
typedef struct xm_pattern_slot_s xm_pattern_slot_t;

struct xm_pattern_s {
	uint16_t num_rows;
	xm_pattern_slot_t* slots; /* Array of size num_rows * num_channels */
};
typedef struct xm_pattern_s xm_pattern_t;

struct xm_module_s {
	char name[XM_MODULE_NAME_LENGTH + 1];
	char trackername[XM_TRACKER_NAME_LENGTH + 1];

	uint16_t length;
	uint16_t restart_position;
	uint16_t num_channels;
	uint16_t num_patterns;
	uint16_t num_instruments;
	xm_frequency_type_t frequency_type;
	uint8_t pattern_table[PATTERN_ORDER_TABLE_LENGTH];

	xm_pattern_t* patterns;
	xm_instrument_t* instruments; /* Instrument 1 has index 0,
								   * instrument 2 has index 1, etc. */
};
typedef struct xm_module_s xm_module_t;

struct xm_channel_context_s {
	float note;
	float orig_note; /* The original note before effect modifications, as read in the pattern. */
	xm_instrument_t* instrument; /* Could be NULL */
	xm_sample_t* sample; /* Could be NULL */
	xm_pattern_slot_t* current;

	float sample_position;
	float period;
	float frequency;
	float step;
	bool ping; /* For ping-pong samples: true is -->, false is <-- */

	float volume; /* Ideally between 0 (muted) and 1 (loudest) */
	float panning; /* Between 0 (left) and 1 (right); 0.5 is centered */

	uint16_t autovibrato_ticks;

	bool sustained;
	float fadeout_volume;
	float volume_envelope_volume;
	float panning_envelope_panning;
	uint16_t volume_envelope_frame_count;
	uint16_t panning_envelope_frame_count;

	float autovibrato_note_offset;

	bool arp_in_progress;
	uint8_t arp_note_offset;
	uint8_t volume_slide_param;
	uint8_t fine_volume_slide_param;
	uint8_t global_volume_slide_param;
	uint8_t panning_slide_param;
	uint8_t portamento_up_param;
	uint8_t portamento_down_param;
	uint8_t fine_portamento_up_param;
	uint8_t fine_portamento_down_param;
	uint8_t extra_fine_portamento_up_param;
	uint8_t extra_fine_portamento_down_param;
	uint8_t tone_portamento_param;
	float tone_portamento_target_period; // FIXME: use long-integer type?
	uint8_t multi_retrig_param;
	uint8_t note_delay_param;
	uint8_t pattern_loop_origin; /* Where to restart a E6y loop */
	uint8_t pattern_loop_count; /* How many loop passes have been done */
	bool vibrato_in_progress;
	xm_waveform_type_t vibrato_waveform;
	bool vibrato_waveform_retrigger; /* True if a new note retriggers the waveform */
	uint8_t vibrato_param;
	uint16_t vibrato_ticks; /* Position in the waveform */
	float vibrato_note_offset;
	xm_waveform_type_t tremolo_waveform;
	bool tremolo_waveform_retrigger;
	uint8_t tremolo_param;
	uint8_t tremolo_ticks;
	float tremolo_volume;
	uint8_t tremor_param;
	bool tremor_on;

	uint64_t latest_trigger;
	bool muted;

	float actual_panning;
	float actual_volume;
};
typedef struct xm_channel_context_s xm_channel_context_t;

struct xm_context_s {
	xm_module_t module;

	uint32_t rate;

	uint16_t tempo;
	uint16_t bpm;
	float global_volume;

	uint8_t current_table_index;
	uint8_t current_row;
	uint16_t current_tick; /* Can go below 255, with high tempo and a pattern delay */
	float remaining_samples_in_tick;
	uint64_t generated_samples;

	bool position_jump;
	bool pattern_break;
	uint8_t jump_dest;
	uint8_t jump_row;

	/* Extra ticks to be played before going to the next row -
	 * Used for EEy effect */
	uint16_t extra_ticks;

	uint8_t* row_loop_count; /* Array of size XM_MAX_PATTERN_ROWS * module_length */
	uint8_t loop_count;
	uint8_t max_loop_count;

	xm_channel_context_t* channels;
};

/* ----- Internal API ----- */

/** Check the module data for errors/inconsistencies.
 *
 * @returns 0 if everything looks OK. Module should be safe to load.
 */
extern int xm_check_sanity_preload(xm_read_callback_t, xm_seek_callback_t, void*);

/** Check a loaded module for errors/inconsistencies.
 *
 * @returns 0 if everything looks OK.
 */
extern int xm_check_sanity_postload(xm_context_t*);

/** Get the number of bytes needed to store the module data in a
 * dynamically allocated blank context.
 *
 * Things that are dynamically allocated:
 * - sample data
 * - sample structures in instruments
 * - pattern data
 * - row loop count arrays
 * - pattern structures in module
 * - instrument structures in module
 * - channel contexts
 * - context structure itself

 * @returns 0 if everything looks OK.
 */
extern size_t xm_get_memory_needed_for_context(xm_read_callback_t, xm_seek_callback_t, void*);

/** Populate the context from module data.
 *
 * @returns pointer to the memory pool
 */
extern char* xm_load_module(xm_context_t*, xm_read_callback_t, xm_seek_callback_t, void*, char*);
