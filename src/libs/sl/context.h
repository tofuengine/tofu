/*
 * MIT License
 *
 * Copyright (c) 2019-2024 Marco Lizza
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

#ifndef TOFU_LIBS_SL_CONTEXT_H
#define TOFU_LIBS_SL_CONTEXT_H

#include "common.h"
#include "source.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct SL_Context_s {
    SL_Group_t groups[SL_GROUPS_AMOUNT];
    SL_Source_t **sources;
} SL_Context_t;

extern SL_Context_t *SL_context_create(void);
extern void SL_context_destroy(SL_Context_t *context);

extern void SL_context_set_mix(SL_Context_t *context, size_t group_id, SL_Mix_t mix);
extern void SL_context_set_pan(SL_Context_t *context, size_t group_id, float pan);
extern void SL_context_set_balance(SL_Context_t *context, size_t group_id, float balance);
extern void SL_context_set_gain(SL_Context_t *context, size_t group_id, float gain);

extern const SL_Group_t *SL_context_get_group(const SL_Context_t *context, size_t group_id);

extern void SL_context_track(SL_Context_t *context, SL_Source_t *source);
extern void SL_context_untrack(SL_Context_t *context, SL_Source_t *source);
extern bool SL_context_is_tracked(const SL_Context_t *context, const SL_Source_t *source);
extern size_t SL_context_count_tracked(const SL_Context_t *context);
extern void SL_context_halt(SL_Context_t *context);

extern bool SL_context_update(SL_Context_t *context, float delta_time);
extern void SL_context_generate(SL_Context_t *context, void *output, size_t frames_requested);

#endif  /* TOFU_LIBS_SL_CONTEXT_H */
