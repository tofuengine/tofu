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

#ifndef __SL_SOURCE_H__
#define __SL_SOURCE_H__

#include "common.h"

#include <stdbool.h>
#include <stddef.h>

typedef void SL_Source_t;

extern void SL_source_destroy(SL_Source_t *source);

extern void SL_source_set_group(SL_Source_t *source, size_t group_id);
extern void SL_source_set_looped(SL_Source_t *source, bool looped);
extern void SL_source_set_mix(SL_Source_t *source, SL_Mix_t mix);
extern void SL_source_set_pan(SL_Source_t *source, float pan);
extern void SL_source_set_twin_pan(SL_Source_t *source, float left_pan, float right_pan);
extern void SL_source_set_balance(SL_Source_t *source, float balance);
extern void SL_source_set_gain(SL_Source_t *source, float gain);
extern void SL_source_set_speed(SL_Source_t *source, float speed);

extern size_t SL_source_get_group(const SL_Source_t *source);
extern bool SL_source_get_looped(const SL_Source_t *source);
extern SL_Mix_t SL_source_get_mix(const SL_Source_t *source);
extern float SL_source_get_gain(const SL_Source_t *source);
extern float SL_source_get_speed(const SL_Source_t *source);

extern bool SL_source_reset(SL_Source_t *source);

extern void SL_source_on_group_changed(SL_Source_t *source, size_t group_id);

#endif  /* __SL_SOURCE_H__ */
