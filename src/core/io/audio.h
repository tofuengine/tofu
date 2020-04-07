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

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "config.h"

#include <miniaudio/miniaudio.h>

#include <stdbool.h>

// https://www.cs.cmu.edu/~music/icm-online/readings/panlaws/
typedef enum _Audio_Panning_Laws_t {
    AUDIO_PANNING_LAW_LINEAR, // L(p) + R(p) = 1
    AUDIO_PANNING_LAW_CONSTANT_POWER, // L(p)^2 + R(p)^2 = 1
    AUDIO_PANNING_LAW_45_DB, // 
    Audio_Panning_Laws_t_CountOf
} Audio_Panning_Laws_t;

typedef struct _Audio_Configuration_t {
    float master_volume;
    Audio_Panning_Laws_t panning_law;
} Audio_Configuration_t;

typedef enum _Audio_Source_States_t {
    AUDIO_SOURCE_STATE_STOPPED,
    AUDIO_SOURCE_STATE_PLAYING,
    AUDIO_SOURCE_STATE_COMPLETED,
    Audio_Source_States_t_CountOf
} Audio_Source_States_t;

// Don't distinguish between music and sounds.

// TODO: implement both from memory and from file.
typedef size_t (*Audio_Source_Read_Callback_t)(void *user_data, void *data, size_t data_size);
typedef void (*Audio_Source_Seek_Callback_t)(void *user_data, long position, int whence);

typedef union _Audio_Mix_t {
#ifdef __AUDIO_FULL_MIX__
    float left_to_left, left_to_right;
    float right_to_left, right_to_right;
#else
    float left, right;
#endif
} Audio_Mix_t;

typedef struct _Audio_Source_t {
//    int channels;
    ma_data_converter converter;
//    ma_decoder decoder;

    void *user_data;
    Audio_Source_Read_Callback_t reader;
    Audio_Source_Seek_Callback_t seeker;

    Audio_Source_States_t state;

    bool looping;
    float volume;
    float panning;
//    float balance; // TODO: use only panning an MONO sources are converted to STEREO on the fly?
    float pitch;

    Audio_Mix_t mix;
} Audio_Source_t;

typedef struct _Audio_t {
    Audio_Configuration_t configuration;

    ma_context_config context_config;
    ma_context context;
    ma_device_config device_config;
    ma_device device;
    ma_mutex lock;

    double time;

    float volume;
    float panning;
//    float balance; // TODO: use only panning an MONO sources are converted to STEREO on the fly?
    float pitch;

    Audio_Source_t **sources;
    Audio_Panning_Laws_t panning_law;
    Audio_Mix_t mix;
} Audio_t;

extern bool Audio_initialize(Audio_t *audio, const Audio_Configuration_t *configuration);
extern void Audio_terminate(Audio_t *audio);
extern void Audio_set_master_volume(Audio_t *audio, float volume);
extern float Audio_get_master_volume(Audio_t *audio);
extern float Audio_get_balance(Audio_Source_t *source);
extern void Audio_set_balance(Audio_Source_t *source, float balance);

extern Audio_Source_t *Audio_source_create(Audio_t *audio, Audio_Source_Read_Callback_t reader, Audio_Source_Seek_Callback_t seeker, void *user_data);
extern void Audio_source_destroy(Audio_t *audio, Audio_Source_t *source);
extern void Audio_source_pause(Audio_Source_t *source);
extern void Audio_source_resume(Audio_Source_t *source);
extern void Audio_source_stop(Audio_Source_t *source);
//extern void Audio_source_set_volume(Audio_Source_t *source, float volume);
//extern void Audio_source_set_panning(Audio_Source_t *source, float panning);
//extern void Audio_source_set_dual_panning(Audio_Source_t *source, float left, float right);

extern void Audio_update(Audio_t *audio, float delta_time);

#endif  /* __AUDIO_H__ */