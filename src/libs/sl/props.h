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

#ifndef TOFU_LIBS_SL_PROPS_H
#define TOFU_LIBS_SL_PROPS_H

#include "common.h"
#include "context.h"

#include <libs/dr_libs.h>

#include <stdbool.h>
#include <stddef.h>

typedef struct SL_Props_s {
    const SL_Context_t *context;
    size_t channels;

    size_t group_id;
    bool looped;
    SL_Mix_t mix;
    float gain;
    float speed;

    // TODO: Add M/S processing: https://github.com/dfilaretti/stereowidth-demo
    // TODO: Add reverb: https://medium.com/the-seekers-project/coding-a-basic-reverb-algorithm-an-introduction-to-audio-programming-d5d90ad58bde
    //                   https://github.com/fabiensanglard/chocolate_duke3D/blob/master/Game/src/audiolib/mvreverb.c
    ma_data_converter converter;
    SL_Mix_t precomputed_mix;
} SL_Props_t;

extern SL_Props_t *SL_props_create(const SL_Context_t *context, ma_format format, ma_uint32 sample_rate, ma_uint32 channels_in, ma_uint32 channels_out);
extern void SL_props_destroy(SL_Props_t *props);

extern void SL_props_set_group(SL_Props_t *props, size_t group_id);
extern void SL_props_set_looped(SL_Props_t *props, bool looped);
extern void SL_props_set_mix(SL_Props_t *props, SL_Mix_t mix);
extern void SL_props_set_pan(SL_Props_t *props, float pan);
extern void SL_props_set_twin_pan(SL_Props_t *props, float left_pan, float right_pan);
extern void SL_props_set_balance(SL_Props_t *props, float pan);
extern void SL_props_set_gain(SL_Props_t *props, float gain);
extern void SL_props_set_speed(SL_Props_t *props, float speed);

extern void SL_props_on_group_changed(SL_Props_t *props, size_t group_id);

#endif  /* TOFU_LIBS_SL_PROPS_H */
