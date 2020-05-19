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

#ifndef __SL_SAMPLE_H__
#define __SL_SAMPLE_H__

#include <miniaudio/miniaudio.h>

#include "common.h"
#include "buffer.h"

#include <stdbool.h>
#include <stddef.h>

typedef size_t (*SL_Sample_Read_Callback_t)(void *user_data, void *output, size_t frames_requested);

typedef enum _SL_Sample_States_t {
    SL_SAMPLE_STATE_STOPPED,
    SL_SAMPLE_STATE_PLAYING,
    SL_SAMPLE_STATE_COMPLETED,
    SL_Sample_States_t_CountOf
} SL_Sample_States_t;

typedef struct _SL_Sample_t {
    Buffer_t buffer;

    ma_data_converter converter;

    size_t group;
    bool looped;
    float gain;
    float pan;
    float speed;

    double time; // ???
    SL_Sample_States_t state;
    SL_Mix_t mix;
} SL_Sample_t;

extern SL_Sample_t *SL_sample_create(SL_Sample_Read_Callback_t on_read, void *user_data, size_t length_in_frames, ma_format format, ma_uint32 sample_rate, ma_uint32 channels);
extern void SL_sample_destroy(SL_Sample_t *sample);

extern void SL_sample_group(SL_Sample_t *sample, size_t group);
extern void SL_sample_looped(SL_Sample_t *sample, bool looped);
extern void SL_sample_gain(SL_Sample_t *sample, float gain);
extern void SL_sample_pan(SL_Sample_t *sample, float pan);
extern void SL_sample_speed(SL_Sample_t *sample, float speed);

extern void SL_sample_play(SL_Sample_t *sample);
extern void SL_sample_stop(SL_Sample_t *sample);
extern void SL_sample_rewind(SL_Sample_t *sample);

extern void SL_sample_update(SL_Sample_t *sample, float delta_time);
extern void SL_sample_mix(SL_Sample_t *sample, void *output, size_t frames_requested, const SL_Mix_t *groups);

#endif  /* __SL_SAMPLE_H__ */