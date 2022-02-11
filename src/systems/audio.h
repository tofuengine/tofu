/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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

#ifndef __SYSTEMS_AUDIO_H__
#define __SYSTEMS_AUDIO_H__

#include <config.h>
#include <libs/sl/sl.h>

#include <miniaudio/miniaudio.h>

#include <stdbool.h>

typedef struct Audio_Configuration_s {
    int device_index;
    float master_volume;
} Audio_Configuration_t;

typedef struct Audio_s {
    Audio_Configuration_t configuration;

    struct {
        ma_log log;
        ma_context context;
        ma_device device;
        ma_mutex lock;
    } driver;

    // TODO: should the audio voices be limited?

    SL_Context_t *context;

#ifdef __AUDIO_START_AND_STOP__
    double grace;
#endif  /* __AUDIO_START_AND_STOP__ */
} Audio_t;

// TODO: rename as lowercase!!!
extern Audio_t *Audio_create(const Audio_Configuration_t *configuration);
extern void Audio_destroy(Audio_t *audio);

extern void Audio_halt(Audio_t *audio);

extern void Audio_set_volume(Audio_t *audio, float volume);
extern void Audio_set_mix(Audio_t *audio, size_t group_id, SL_Mix_t mix);
extern void Audio_set_pan(Audio_t *audio, size_t group_id, float pan);
extern void Audio_set_balance(Audio_t *audio, size_t group_id, float balance);
extern void Audio_set_gain(Audio_t *audio, size_t group_id, float gain);

extern float Audio_get_volume(const Audio_t *audio);
extern SL_Mix_t Audio_get_mix(const Audio_t *audio, size_t group_id);
extern float Audio_get_gain(const Audio_t *audio, size_t group_id);

extern void Audio_track(Audio_t *audio, SL_Source_t *source, bool reset);
extern void Audio_untrack(Audio_t *audio, SL_Source_t *source);
extern bool Audio_is_tracked(const Audio_t *audio, SL_Source_t *source);

extern bool Audio_update(Audio_t *audio, float delta_time);

#endif  /* __SYSTEMS_AUDIO_H__ */
