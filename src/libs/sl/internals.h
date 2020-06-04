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

#include "source.h"
#include "props.h"

typedef struct _Source_VTable_t {
    void (*dtor)(SL_Source_t *source);
    void (*play)(SL_Source_t *source);
    void (*stop)(SL_Source_t *source);
    void (*rewind)(SL_Source_t *source);
    void (*update)(SL_Source_t *source, float delta_time);
    void (*mix)(SL_Source_t *source, void *output, size_t frames_requested, const SL_Mix_t *groups);
} Source_VTable_t;

typedef enum _Source_States_t {
    SOURCE_STATE_STOPPED,
    SOURCE_STATE_PLAYING,
    SOURCE_STATE_FINISHING, // Used for streaming. There's buffered audio to play before stopping.
    Source_States_t_CountOf
} Source_States_t;

typedef struct _Source_t {
    Source_VTable_t vtable;
    SL_Props_t props;
    volatile Source_States_t state;
} Source_t;

#endif  /* __SL_INTERNALS_H__ */