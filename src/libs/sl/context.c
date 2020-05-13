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

#include "context.h"

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

// The balance law differs from the panning law in the fact that when on center the channels are 0dB.
static inline SL_Mix_t _precompute_mix(float balance, float gain)
{
#if __SL_BALANCE_LAW__ == BALANCE_LAW_LINEAR
    if (balance < 0.0f) {
        return (SL_Mix_t){ .left = gain, .right = (1.0f + balance) * gain };
    } else
    if (balance > 0.0f) {
        return (SL_Mix_t){ .left = (1.0f - balance) * gain, .right = gain };
    } else {
        return (SL_Mix_t){ .left = gain, .right = gain };
    }
#elif __SL_BALANCE_LAW__ == BALANCE_LAW_SINCOS
    if (balance < 0.0f) {
        return (SL_Mix_t){ .left = gain, .right = sinf((1.0f + balance) * M_PI_2) * gain };
    } else
    if (balance > 0.0f) {
        return (SL_Mix_t){ .left = sinf((1.0f - balance) * M_PI_2) * gain, .right = gain };
    } else {
        return (SL_Mix_t){ .left = gain, .right = gain };
    }
#elif __SL_BALANCE_LAW__ == BALANCE_LAW_SQRT
    if (balance < 0.0f) {
        return (SL_Mix_t){ .left = gain, .right = sqrtf(1.0f + balance) * gain };
    } else
    if (balance > 0.0f) {
        return (SL_Mix_t){ .left = sqrtf(1.0f - balance) * gain, .right = gain };
    } else {
        return (SL_Mix_t){ .left = gain, .right = gain };
    }
#endif
}

SL_Context_t *SL_context_create(void)
{
    SL_Context_t *context = malloc(sizeof(SL_Context_t));
    if (!context) {
        return NULL;
    }

    *context = (SL_Context_t){
            .streams = NULL
        };

    for (size_t i = 0; i < SL_GROUPS_AMOUNT; ++i) {
        context->groups[i] = _precompute_mix(0.0f, 1.0f);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context created");
    return context;
}

void SL_context_destroy(SL_Context_t *context)
{
    if (!context) {
        return;
    }

    arrfree(context->streams);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context groups freed");

    free(context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context freed");
}

void SL_context_update(SL_Context_t *context, float delta_time)
{
    SL_Stream_t **current = context->streams;
    for (int count = arrlen(context->streams); count; --count) {
        SL_Stream_t *stream = *(current++);
        SL_stream_update(stream, delta_time);
    }
}

void SL_context_mix(SL_Context_t *context, void *output, size_t frames_requested)
{
    const SL_Mix_t *groups = context->groups;

    SL_Stream_t **current = context->streams;
    for (int count = arrlen(context->streams); count; --count) {
        SL_Stream_t *stream = *(current++);
        SL_stream_mix(stream, output, frames_requested, groups); // FIXME: pass the dereferences mix? API violation?
    }
}

void SL_context_tweak(SL_Context_t *context, size_t group, float balance, float gain)
{
    context->groups[group] = _precompute_mix(balance, gain);
}

void SL_context_track(SL_Context_t *context, SL_Stream_t *stream)
{
    size_t count = arrlen(context->streams);
    for (size_t i = 0; i < count; ++i) {
        if (context->streams[i] == stream) {
            return;
        }
    }
    arrpush(context->streams, stream);
}

void SL_context_untrack(SL_Context_t *context, SL_Stream_t *stream)
{
    size_t count = arrlen(context->streams);
    for (size_t i = 0; i < count; ++i) {
        if (context->streams[i] == stream) {
            arrdel(context->streams, i);
            break;
        }
    }
}

void SL_context_stop(SL_Context_t *context)
{
    SL_Stream_t **current = context->streams;
    for (int count = arrlen(context->streams); count; --count) {
        SL_Stream_t *stream = *(current++);
        SL_stream_stop(stream);
    }
}