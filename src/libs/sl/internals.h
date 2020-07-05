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

#ifndef __SL_INTERNALS_H__
#define __SL_INTERNALS_H__

#include "common.h"
#include "source.h"
#include "props.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct _Source_VTable_t {
    void (*dtor)(SL_Source_t *source);
    bool (*reset)(SL_Source_t *source);
    bool (*update)(SL_Source_t *source, float delta_time);
    bool (*mix)(SL_Source_t *source, void *output, size_t frames_requested, const SL_Group_t *groups); // Returns `false` when end-of-data.
} Source_VTable_t;

typedef struct _Source_t {
    Source_VTable_t vtable;

    SL_Props_t props;

    SL_Callbacks_t callbacks;
    void *user_data;

    size_t length_in_frames;
} Source_t;

// https://english.stackexchange.com/questions/457305/the-difference-between-state-and-status
typedef enum _Source_States_t {
    SOURCE_STATE_PLAYING,
    SOURCE_STATE_STALLING,
    SOURCE_STATE_EOD,
} Source_States_t;

#endif  /* __SL_INTERNALS_H__ */