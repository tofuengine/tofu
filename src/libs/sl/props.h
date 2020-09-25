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

#ifndef __SL_PROPS_H__
#define __SL_PROPS_H__

#include <miniaudio/miniaudio.h>

#include "common.h"
#include "context.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct _SL_Props_t {
    const SL_Context_t *context;

    size_t group_id;
    bool looping; // TODO: rename to looped?
    SL_Mix_t mix;
    float gain;
    float speed;

    // TODO: Add M/S processing: https://github.com/dfilaretti/stereowidth-demo
    ma_data_converter converter;
    SL_Mix_t precomputed_mix;
} SL_Props_t;

extern bool SL_props_init(SL_Props_t *props, const SL_Context_t *context, ma_format format, ma_uint32 sample_rate, ma_uint32 channels_in, ma_uint32 channels_out);
extern void SL_props_deinit(SL_Props_t *props);

extern void SL_props_group(SL_Props_t *props, size_t group_id);
extern void SL_props_looping(SL_Props_t *props, bool looping);
extern void SL_props_mix(SL_Props_t *props, SL_Mix_t mix);
extern void SL_props_pan(SL_Props_t *props, float pan);
extern void SL_props_twin_pan(SL_Props_t *props, float left_pan, float right_pan);
extern void SL_props_balance(SL_Props_t *props, float pan);
extern void SL_props_gain(SL_Props_t *props, float gain);
extern void SL_props_speed(SL_Props_t *props, float speed);

extern void SL_props_on_group_changed(SL_Props_t *props, size_t group_id);

#endif  /* __SL_PROPS_H__ */