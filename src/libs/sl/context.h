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

#ifndef __SL_CONTEXT_H__
#define __SL_CONTEXT_H__

#include "group.h"

typedef struct _SL_Context_t {
    SL_Group_t **groups;
} SL_Context_t;

extern SL_Context_t *SL_context_create(void);
extern void SL_context_destroy(SL_Context_t *context);

extern void SL_context_update(SL_Context_t *context, float delta_time);
extern void SL_context_process(SL_Context_t *context, float *output, size_t frames_requested);
extern void SL_context_reset(SL_Context_t *context); // TODO: call on software failure.

extern void SL_context_track(SL_Context_t *context, SL_Group_t *group);
extern void SL_context_untrack(SL_Context_t *context, SL_Group_t *group);

#endif  /* __SL_CONTEXT_H__ */