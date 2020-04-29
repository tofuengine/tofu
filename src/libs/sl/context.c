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

#define LOG_CONTEXT "sl"

SL_Context_t *SL_context_create(void)
{
    SL_Context_t *context = malloc(sizeof(SL_Context_t));
    if (!context) {
        return NULL;
    }

    *context = (SL_Context_t){
            .groups = NULL
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context created");
    return context;
}

void SL_context_destroy(SL_Context_t *context)
{
    if (!context) {
        return;
    }

    arrfree(context->groups);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context groups freed");

    free(context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context freed");
}

void SL_context_update(SL_Context_t *context, float delta_time)
{
    size_t count = arrlen(context->groups); // TODO: iterate differently with forward direction?
    for (int i = count - 1; i >= 0; --i) {
        SL_group_update(context->groups[i], delta_time);
    }
}

void SL_context_process(SL_Context_t *context, float *output, size_t frames_requested)
{
    size_t count = arrlen(context->groups);
    for (int i = count - 1; i >= 0; --i) {
        SL_group_process(context->groups[i], output, frames_requested);
    }
}

void SL_context_reset(SL_Context_t *context)
{
    size_t count = arrlen(context->groups);
    for (int i = count - 1; i >= 0; --i) {
        SL_group_reset(context->groups[i]);
    }
}

void SL_context_track(SL_Context_t *context, SL_Group_t *group)
{
    size_t count = arrlen(context->groups);
    for (int i = count - 1; i >= 0; --i) {
        if (context->groups[i] == group) {
            return;
        }
    }
    arrpush(context->groups, group);
}

void SL_context_untrack(SL_Context_t *context, SL_Group_t *group)
{
    size_t count = arrlen(context->groups);
    for (int i = count - 1; i >= 0; --i) {
        if (context->groups[i] == group) {
            arrdel(context->groups, i);
            break;
        }
    }
}

