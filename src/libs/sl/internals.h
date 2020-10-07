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

#if SL_BYTES_PER_SAMPLE == 2
  #define INTERNAL_FORMAT   ma_format_s16
#elif SL_BYTES_PER_SAMPLE == 4
  #define INTERNAL_FORMAT   ma_format_f32
#else
  #error Wrong internal format.
#endif

typedef struct _Source_VTable_t {
    void (*dtor)(SL_Source_t *source);
    bool (*reset)(SL_Source_t *source);
    bool (*update)(SL_Source_t *source, float delta_time);
    bool (*generate)(SL_Source_t *source, void *output, size_t frames_requested); // Returns `false` when end-of-data.
} Source_VTable_t;

typedef struct _Source_t {
    Source_VTable_t vtable;

    SL_Props_t *props;
} Source_t;

#endif  /* __SL_INTERNALS_H__ */
