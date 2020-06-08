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

#include "internals.h"
#include "mix.h"

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
            .sources = NULL
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

    arrfree(context->sources);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context sources freed");

    free(context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context freed");
}

void SL_context_tweak(SL_Context_t *context, size_t group, float balance, float gain)
{
    context->groups[group] = mix_precompute_balance(balance, gain);
}

size_t SL_context_track(SL_Context_t *context, SL_Source_t *source)
{
    size_t count = arrlen(context->sources);
    for (size_t i = 0; i < count; ++i) {
        if (context->sources[i] == source) {
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "source %p already tracked for context %p", source, context);
            return count;
        }
    }
    arrpush(context->sources, source);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p tracked for context %p", source, context);
    return count + 1;
}

size_t SL_context_untrack(SL_Context_t *context, SL_Source_t *source)
{
    size_t count = arrlen(context->sources);
    for (size_t i = 0; i < count; ++i) {
        if (context->sources[i] == source) {
            arrdel(context->sources, i);
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p untracked for context %p", source, context);
            return count - 1;
        }
    }
    return count;
}

bool SL_context_is_tracking(SL_Context_t *context, SL_Source_t *source)
{
    size_t count = arrlen(context->sources);
    for (size_t i = 0; i < count; ++i) {
        if (context->sources[i] == source) {
            return true;
        }
    }
    return false;
}

void SL_context_update(SL_Context_t *context, float delta_time)
{
    SL_Source_t **current = context->sources;
    for (int count = arrlen(context->sources); count; --count) {
        SL_Source_t *source = *(current++);
        ((Source_t *)source)->vtable.update(source, delta_time);
    }
}

size_t SL_context_mix(SL_Context_t *context, void *output, size_t frames_requested)
{
    const SL_Mix_t *groups = context->groups;

    // Backward scan, to remove to-be-untracked sources.
    for (int i = arrlen(context->sources) - 1; i >= 0; --i) {
        SL_Source_t *source = context->sources[i];
        bool untrack = ((Source_t *)source)->vtable.mix(source, output, frames_requested, groups); // FIXME: pass the dereferences mix? API violation?
        if (untrack) {
            arrdel(context->sources, i);
        }
    }
    return arrlen(context->sources);
}

size_t SL_context_halt(SL_Context_t *context)
{
    arrfree(context->sources);
    return 0;
}
