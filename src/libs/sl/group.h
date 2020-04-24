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

#ifndef __SL_GROUP_H__
#define __SL_GROUP_H__

#include "common.h"
#include "source.h"

typedef struct _SL_Group_t {
    float gain;
    float pan;

    SL_Source_t **sources;

    SL_Mix_t mix;
} SL_Group_t;

extern SL_Group_t *SL_group_create(void);
extern void SL_group_destroy(SL_Group_t *group);

extern void SL_group_update(SL_Group_t *group, float delta_time);
extern void SL_group_process(SL_Group_t *group, float *output, size_t frames_requested);
extern void SL_group_reset(SL_Group_t *group);

extern void SL_group_gain(SL_Group_t *group, float gain);
extern void SL_group_pan(SL_Group_t *group, float pan);

extern void SL_group_track(SL_Group_t *group, SL_Source_t *source);
extern void SL_group_untrack(SL_Group_t *group, SL_Source_t *source);

#endif  /* __SL_GROUP_H__ */