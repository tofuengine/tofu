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

#ifndef __SL_SOURCE_H__
#define __SL_SOURCE_H__

#include <miniaudio/miniaudio.h>

#include "common.h"

#include <stdbool.h>
#include <stddef.h>

// TODO: implement both from memory and from file.
typedef size_t (*SL_Source_Read_Callback_t)(void *user_data, void *output, size_t frames_requested);
typedef void (*SL_Source_Seek_Callback_t)(void *user_data, size_t frame_offset);

typedef enum _SL_Source_States_t {
    SL_SOURCE_STATE_STOPPED,
    SL_SOURCE_STATE_PLAYING,
    SL_SOURCE_STATE_COMPLETED,
    SL_Source_States_t_CountOf
} SL_Source_States_t;

typedef struct _SL_Source_t {
    SL_Source_Read_Callback_t on_read;
    SL_Source_Seek_Callback_t on_seek;
    void *user_data;

    ma_data_converter converter;

    bool looped;
    float delay; // ???
    float gain;
    float pan;
    float speed;

    double time; // ???
    SL_Source_States_t state;
    SL_Mix_t mix;
} SL_Source_t;

extern SL_Source_t *SL_source_create(SL_Source_Read_Callback_t on_read, SL_Source_Seek_Callback_t on_seek, void *user_data, ma_format format, ma_uint32 sample_rate, ma_uint32 channels);
extern void SL_source_destroy(SL_Source_t *source);

extern void SL_source_looped(SL_Source_t *source, bool looped);
extern void SL_source_delay(SL_Source_t *source, float delay);
extern void SL_source_gain(SL_Source_t *source, float gain);
extern void SL_source_pan(SL_Source_t *source, float pan);
extern void SL_source_speed(SL_Source_t *source, float speed);

extern void SL_source_play(SL_Source_t *source);
extern void SL_source_stop(SL_Source_t *source);
extern void SL_source_rewind(SL_Source_t *source);

extern void SL_source_update(SL_Source_t *source, float delta_time);
extern void SL_source_process(SL_Source_t *source, float *output, size_t frames_requested, SL_Mix_t mix);

#endif  /* __SL_SOURCE_H__ */