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

#ifndef __SL_MUSIC_H__
#define __SL_MUSIC_H__

#include <miniaudio/miniaudio.h>

#include "common.h"

#include <stdbool.h>
#include <stddef.h>

typedef size_t (*SL_Music_Read_Callback_t)(void *user_data, void *output, size_t frames_requested);
typedef void (*SL_Music_Seek_Callback_t)(void *user_data, size_t frame_offset);

typedef enum _SL_Music_States_t {
    SL_MUSIC_STATE_STOPPED,
    SL_MUSIC_STATE_PLAYING,
    SL_MUSIC_STATE_FINISHING,
    SL_Music_States_t_CountOf
} SL_Music_States_t;

typedef struct _SL_Music_t {
    SL_Music_Read_Callback_t on_read;
    SL_Music_Seek_Callback_t on_seek;
    void *user_data;

    ma_pcm_rb buffer;

    ma_data_converter converter;

    size_t group;
    bool looped;
    float gain;
    float pan;
    float speed;

    double time; // ???
    volatile SL_Music_States_t state;
    SL_Mix_t mix;
} SL_Music_t;

extern SL_Music_t *SL_music_create(SL_Music_Read_Callback_t on_read, SL_Music_Seek_Callback_t on_seek, void *user_data, ma_format format, ma_uint32 sample_rate, ma_uint32 channels);
extern void SL_music_destroy(SL_Music_t *music);

extern void SL_music_group(SL_Music_t *music, size_t group);
extern void SL_music_looped(SL_Music_t *music, bool looped);
extern void SL_music_gain(SL_Music_t *music, float gain);
extern void SL_music_pan(SL_Music_t *music, float pan);
extern void SL_music_speed(SL_Music_t *music, float speed);

extern void SL_music_play(SL_Music_t *music);
extern void SL_music_stop(SL_Music_t *music);
extern void SL_music_rewind(SL_Music_t *music);

extern void SL_music_update(SL_Music_t *music, float delta_time);
extern void SL_music_mix(SL_Music_t *music, void *output, size_t frames_requested, const SL_Mix_t *groups);

#endif  /* __SL_MUSIC_H__ */