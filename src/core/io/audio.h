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

typedef struct _Audio_Configuration_t {
    size_t channels;
    size_t sample_rate;
    size_t voices;
} Audio_Configuration_t;

typedef struct _Audio_Stream_t {
    void *data;
    size_t data_size;
    size_t index;
} Audio_Stream_t;

typedef struct _Audio_t {
    Audio_Configuration_t configuration;

    ma_context_config context_config;
    ma_context context;
    ma_device_config device_config;
    ma_device device;
    ma_decoder decoder;
    ma_mutex lock;

    double time;

    float master_volume;
    Audio_Stream_t *streams;
} Audio_t;

extern bool Audio_initialize(Audio_t *audio, const Audio_Configuration_t *configuration);
extern void Audio_terminate(Audio_t *audio);

extern void Audio_update(Audio_t *audio, float delta_time);

#endif  /* __AUDIO_H__ */