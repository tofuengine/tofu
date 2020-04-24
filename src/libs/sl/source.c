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

#include "source.h"

#include <config.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <math.h>

#ifndef M_PI
  #define M_PI      3.14159265358979323846f
#endif
#ifndef M_PI_2
  #define M_PI_2    1.57079632679489661923f
#endif

#define LOG_CONTEXT "sl"

static inline SL_Mix_t _0db_linear_mix(float balance, float gain)
{
#if 0
    if (balance < 0.0f) {
        return (SL_Mix_t){ .left = gain, .right = (1.0f + balance) * gain };
    } else
    if (balance > 0.0f) {
        return (SL_Mix_t){ .left = (1.0f - balance) * gain, .right = gain };
    } else {
        return (SL_Mix_t){ .left = gain, .right = gain };
    }
#else
    const float theta = (balance + 1.0f) * 0.5f * M_PI_2; // [-1, 1] -> [0 , 1] -> [0, pi/2]
    return (SL_Mix_t){ .left = cosf(theta) * gain, .right = sinf(theta) * gain };
#endif
}

SL_Source_t *SL_source_create(SL_Source_Read_Callback_t on_read, SL_Source_Seek_Callback_t on_seek, void *user_data)
{
    SL_Source_t *source = malloc(sizeof(SL_Source_t));
    if (!source) {
        return NULL;
    }

    *source = (SL_Source_t){
            .on_read = on_read,
            .on_seek = on_seek,
            .user_data = user_data,
            .looped = false,
            .delay = 0.0f,
            .gain = 1.0,
            .pan = 0.0f,
            .speed = 1.0f,
            .time = 0.0f,
            .state = SL_SOURCE_STATE_STOPPED,
            .mix = _0db_linear_mix(1.0f, 0.0f)
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source created");
    return source;
}

void SL_source_destroy(SL_Source_t *source)
{
    if (!source) {
        return;
    }

    free(source);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source freed");
}

void SL_source_looped(SL_Source_t *source, bool looped)
{
    source->looped = looped;
}

void SL_source_delay(SL_Source_t *source, float delay)
{
    source->delay = delay;
}

void SL_source_gain(SL_Source_t *source, float gain)
{
    source->gain = gain;
    source->mix = _0db_linear_mix(source->gain, source->pan);
}

void SL_source_pan(SL_Source_t *source, float pan)
{
    source->pan = pan;
    source->mix = _0db_linear_mix(source->gain, source->pan);
}

void SL_source_speed(SL_Source_t *source, float speed)
{
    source->speed = speed;
}

// TODO: implement I/O

void SL_source_pause(SL_Source_t *source)
{
    source->state = SL_SOURCE_STATE_STOPPED;
}

void SL_source_resume(SL_Source_t *source)
{
    source->state = SL_SOURCE_STATE_PLAYING;
}

void SL_source_stop(SL_Source_t *source)
{
    source->state = SL_SOURCE_STATE_STOPPED;
}

void SL_source_update(SL_Source_t *source, float delta_time)
{
    source->time = delta_time;
}

size_t SL_source_process(SL_Source_t *source, float *output, size_t frames_requested)
{
    // FIXME: fill the output as much as possible, eventually looping
    return source->on_read(source->user_data, output, frames_requested);
}
