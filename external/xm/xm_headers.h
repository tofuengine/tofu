/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __XM_HEADERS_H__
#define __XM_HEADERS_H__

#include "xm_internal.h"

#include <stdint.h>

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
#pragma pack(push, 1)
typedef struct _xm_info_t {
	char id[XM_MODULE_ID_LENGTH];
	char module_name[XM_MODULE_NAME_LENGTH];
	uint8_t magic;
	char tracker_name[XM_TRACKER_NAME_LENGTH];
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
	char name[XM_INSTRUMENT_NAME_LENGTH];
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
	uint8_t sample_number[XM_NOTES_AMOUNT];
	xm_instrument_header_evenlope_point_t volume_points[XM_MAX_ENVELOPE_POINTS];
	xm_instrument_header_evenlope_point_t panning_points[XM_MAX_ENVELOPE_POINTS];
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
	uint32_t length; // in bytes, not relative to samples.
	uint32_t loop_start; // ditto.
	uint32_t loop_end; // ditto.
	uint8_t volume;
	uint8_t finetune;
	uint8_t flags;
	uint8_t panning;
	uint8_t relative_note;
	uint8_t __reserved;
	char name[XM_SAMPLE_NAME_LENGTH];
} xm_sample_header_t;
#pragma pack(pop)

#endif	/* __XM_HEADERS_H__ */
