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

#include "mix.h"
#include <config.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <math.h>

#define LOG_CONTEXT "sl"

SL_Context_t *SL_context_create(void)
{
    SL_Context_t *context = malloc(sizeof(SL_Context_t));
    if (!context) {
        return NULL;
    }

    *context = (SL_Context_t){
            .voices = NULL
        };

    for (size_t i = 0; i < SL_GROUPS_AMOUNT; ++i) {
        context->groups[i] = mix_precompute_balance(0.0f, 1.0f);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context created");
    return context;
}

void SL_context_destroy(SL_Context_t *context)
{
    if (!context) {
        return;
    }

    arrfree(context->voices);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context voices freed");

    free(context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context freed");
}

void SL_context_tweak(SL_Context_t *context, size_t group, float balance, float gain)
{
    context->groups[group] = mix_precompute_balance(balance, gain);
}

void SL_context_track(SL_Context_t *context, SL_Source_t *source, SL_Source_Types_t type)
{
    size_t count = arrlen(context->voices);
    for (size_t i = 0; i < count; ++i) {
        if (context->voices[i].source == source) {
            return;
        }
    }
    arrpush(context->voices, (SL_Voice_t){ .type = type, .source = source });
}

void SL_context_untrack(SL_Context_t *context, SL_Source_t *source)
{
    size_t count = arrlen(context->voices);
    for (size_t i = 0; i < count; ++i) {
        if (context->voices[i].source == source) {
            arrdel(context->voices, i);
            break;
        }
    }
}

void SL_context_update(SL_Context_t *context, float delta_time)
{
    SL_Voice_t **current = context->voices;
    for (int count = arrlen(context->voices); count; --count) {
        SL_Voice_t *voice = *(current++);
        SL_music_update(stream, delta_time);
    }
}

void SL_context_mix(SL_Context_t *context, void *output, size_t frames_requested)
{
    const SL_Mix_t *groups = context->groups;

    SL_Voice_t **current = context->voices;
    for (int count = arrlen(context->voices); count; --count) {
        SL_Voice_t *voice = *(current++);
        SL_music_mix(stream, output, frames_requested, groups); // FIXME: pass the dereferences mix? API violation?
    }
}

void SL_context_stop(SL_Context_t *context)
{
    SL_Voice_t **current = context->voices;
    for (int count = arrlen(context->voices); count; --count) {
        SL_Voice_t *voice = *(current++);
        SL_music_stop(stream);
    }
}