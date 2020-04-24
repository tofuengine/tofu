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

#include "group.h"

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

SL_Group_t *SL_group_create(void)
{
    SL_Group_t *group = malloc(sizeof(SL_Group_t));
    if (!group) {
        return NULL;
    }

    *group = (SL_Group_t){
            .gain = 1.0f,
            .pan = 0.0f,
            .sources = NULL,
            .mix = _0db_linear_mix(1.0f, 0.0f)
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group created");
    return group;
}

void SL_group_destroy(SL_Group_t *group)
{
    if (!group) {
        return;
    }

    arrfree(group->sources);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group sources freed");

    free(group);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group freed");
}

void SL_group_update(SL_Group_t *group, float delta_time)
{
    size_t count = arrlen(group->sources);
    for (int i = count - 1; i >= 0; --i) {
        SL_source_update(group->sources[i], delta_time);
    }
}

void SL_group_process(SL_Group_t *group, float *output, size_t frames_requested)
{
#define SL_DEVICE_CHANNELS  2
    float buffer[frames_requested * SL_DEVICE_CHANNELS];

    size_t count = arrlen(group->sources);
    for (int i = count - 1; i >= 0; --i) {
        size_t frames_processed = SL_source_process(group->sources[i], buffer, frames_requested);
        for (size_t j = 0; j < frames_processed; ++j) {
            output[j] += buffer[j];
        }
    }
}

void SL_group_reset(SL_Group_t *group)
{
/*
    size_t count = arrlen(group->sources);
    for (int i = count - 1; i >= 0; --i) {
        SL_source_reset(group->sources[i]);
    }
*/
}

void SL_group_gain(SL_Group_t *group, float gain)
{
    group->gain = gain;
    group->mix = _0db_linear_mix(group->gain, group->pan);
}

void SL_group_pan(SL_Group_t *group, float pan)
{
    group->pan = pan;
    group->mix = _0db_linear_mix(group->gain, group->pan);
}

void SL_group_track(SL_Group_t *group, SL_Source_t *source)
{
    size_t count = arrlen(group->sources);
    for (int i = count - 1; i >= 0; --i) {
        if (group->sources[i] == source) {
            return;
        }
    }
    arrpush(group->sources, source);
}

void SL_group_untrack(SL_Group_t *group, SL_Source_t *source)
{
    size_t count = arrlen(group->sources);
    for (int i = count - 1; i >= 0; --i) {
        if (group->sources[i] == source) {
            arrdel(group->sources, i);
            break;
        }
    }
}
