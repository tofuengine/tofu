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

#ifndef __AL_GROUP_H__
#define __AL_GROUP_H__

#include "source.h"

typedef struct _AL_Group_t {
    float gain;
    float pan;

    AL_Source_t *sources;

    float mix[2];
} AL_Group_t;

extern AL_Group_t *AL_group_create();
extern void AL_group_destroy(AL_Group_t *group);

extern void AL_group_reset(AL_Group_t *group); // TODO: call on software failure.

extern void AL_group_gain(AL_Group_t *group, float gain);
extern void AL_group_pan(AL_Group_t *group, float pan);

extern void AL_group_track(AL_Group_t *group, AL_Source_t *source);
extern void AL_group_untrack(AL_Group_t *group, AL_Source_t *source);

extern void AL_group_update(AL_Group_t *group, float delta_time);

#endif  /* __AL_GROUP_H__ */