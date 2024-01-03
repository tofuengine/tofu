/*
 * MIT License
 *
 * Copyright (c) 2019-2023 Marco Lizza
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

#include "internal.h"
#include "mix.h"

#define _LOG_TAG "sl"
#include <libs/log.h>
#include <libs/stb.h>

#include <math.h>

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
        context->groups[i] = (SL_Group_t){
                .mix = mix_balance(0.0f), // Groups are stereo by definition, so we are balancing as a default.
                .gain = 1.0f
            };
    }

    LOG_D("context created");
    return context;
}

void SL_context_destroy(SL_Context_t *context)
{
    arrfree(context->sources);
    LOG_D("context sources freed");

    free(context);
    LOG_D("context freed");
}

static void _fire_on_group_changed(const SL_Context_t *context, size_t group_id)
{
    LOG_T("context group #%d changed, firing event", group_id);

    SL_Source_t **current = context->sources;
    for (size_t count = arrlenu(context->sources); count; --count) {
        SL_Source_t *source = *(current++);

        SL_source_on_group_changed(source, group_id);
    }
}

void SL_context_set_mix(SL_Context_t *context, size_t group_id, SL_Mix_t mix)
{
    SL_Group_t *group = &context->groups[group_id];

    group->mix = mix;
    _fire_on_group_changed(context, group_id);
}

void SL_context_set_pan(SL_Context_t *context, size_t group_id, float pan)
{
    SL_Group_t *group = &context->groups[group_id];

    group->mix = mix_pan(fmaxf(-1.0f, fminf(pan, 1.0f)));
    _fire_on_group_changed(context, group_id);
}

void SL_context_set_balance(SL_Context_t *context, size_t group_id, float balance)
{
    SL_Group_t *group = &context->groups[group_id];

    group->mix = mix_balance(fmaxf(-1.0f, fminf(balance, 1.0f)));
    _fire_on_group_changed(context, group_id);
}

void SL_context_set_gain(SL_Context_t *context, size_t group_id, float gain)
{
    SL_Group_t *group = &context->groups[group_id];

    group->gain = fmaxf(0.0f, gain);
    _fire_on_group_changed(context, group_id);
}

const SL_Group_t *SL_context_get_group(const SL_Context_t *context, size_t group_id)
{
    return &context->groups[group_id];
}

void SL_context_track(SL_Context_t *context, SL_Source_t *source)
{
    size_t count = arrlenu(context->sources);
    for (size_t i = 0; i < count; ++i) {
        if (context->sources[i] == source) {
            LOG_W("source %p already tracked for context %p", source, context);
            return;
        }
    }
    arrpush(context->sources, source);
    LOG_D("source %p tracked for context %p", source, context);
    SL_source_on_group_changed(source, SL_ANY_GROUP); // Propagate, to the attached source, to precompute the mix matrix.
}

void SL_context_untrack(SL_Context_t *context, SL_Source_t *source)
{
    size_t count = arrlenu(context->sources);
    for (size_t i = 0; i < count; ++i) {
        if (context->sources[i] == source) {
            arrdelswap(context->sources, i);

            LOG_D("source %p untracked from context %p", source, context);
            return;
        }
    }
}

bool SL_context_is_tracked(const SL_Context_t *context, const SL_Source_t *source)
{
    size_t count = arrlenu(context->sources);
    for (size_t i = 0; i < count; ++i) {
        if (context->sources[i] == source) {
            return true;
        }
    }
    return false;
}

size_t SL_context_count_tracked(const SL_Context_t *context)
{
    return arrlenu(context->sources);
}

void SL_context_halt(SL_Context_t *context)
{
    arrfree(context->sources);
}

// TODO: should we constantly generate into a buffer during the `update()` call and pull data later?
bool SL_context_update(SL_Context_t *context, float delta_time)
{
    SL_Source_t **current = context->sources;
    for (size_t count = arrlenu(context->sources); count; --count) {
        SL_Source_t *source = *(current++);
        bool result = source->vtable.update(source, delta_time);
        if (!result) {
            return false;
        }
    }
    return true;
}

void SL_context_generate(SL_Context_t *context, void *output, size_t frames_requested)
{
    // Backward scan, to properly implement the SWAP-AND-POP(tm) idiom along the whole array
    // when removing the to-be-released sources.
    for (int index = arrlen(context->sources) - 1; index >= 0; --index) {
        SL_Source_t *source = context->sources[index];
        bool still_running = source->vtable.generate(source, output, frames_requested);
        if (still_running) {
            continue;
        }

        arrdelswap(context->sources, index); // Obliterate the source!
    }
}
