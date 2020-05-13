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

#ifndef __SL_stream_H__
#define __SL_stream_H__

#include <miniaudio/miniaudio.h>

#include "common.h"

#include <stdbool.h>
#include <stddef.h>

typedef size_t (*SL_Stream_Read_Callback_t)(void *user_data, void *output, size_t frames_requested);
typedef void (*SL_Stream_Seek_Callback_t)(void *user_data, size_t frame_offset);

typedef enum _SL_Stream_States_t {
    SL_STREAM_STATE_STOPPED,
    SL_STREAM_STATE_PLAYING,
    SL_STREAM_STATE_FINISHING,
    SL_STREAM_STATE_COMPLETED,
    SL_Stream_States_t_CountOf
} SL_Stream_States_t;

typedef struct _SL_stream_t {
    SL_Stream_Read_Callback_t on_read;
    SL_Stream_Seek_Callback_t on_seek;
    void *user_data;

    ma_pcm_rb buffer;

    ma_data_converter converter;

    size_t group;
    bool looped;
    float gain;
    float pan;
    float speed;

    double time; // ???
    SL_Stream_States_t state;
    SL_Mix_t mix;
} SL_Stream_t;

extern SL_Stream_t *SL_stream_create(SL_Stream_Read_Callback_t on_read, SL_Stream_Seek_Callback_t on_seek, void *user_data, ma_format format, ma_uint32 sample_rate, ma_uint32 channels);
extern void SL_stream_destroy(SL_Stream_t *stream);

extern void SL_stream_group(SL_Stream_t *stream, size_t group);
extern void SL_stream_looped(SL_Stream_t *stream, bool looped);
extern void SL_stream_gain(SL_Stream_t *stream, float gain);
extern void SL_stream_pan(SL_Stream_t *stream, float pan);
extern void SL_stream_speed(SL_Stream_t *stream, float speed);

extern void SL_stream_play(SL_Stream_t *stream);
extern void SL_stream_stop(SL_Stream_t *stream);
extern void SL_stream_rewind(SL_Stream_t *stream);

extern void SL_stream_update(SL_Stream_t *stream, float delta_time);
extern void SL_stream_mix(SL_Stream_t *stream, void *output, size_t frames_requested, const SL_Mix_t *groups);

#endif  /* __SL_stream_H__ */