/*
 * MIT License
 *
 * Copyright (c) 2019-2021 Marco Lizza
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

#ifndef __SL_MIX_H__
#define __SL_MIX_H__

#include "common.h"

#include <stddef.h>

extern SL_Mix_t mix_twin_pan(float left_pan, float right_pan);
extern SL_Mix_t mix_pan(float pan);
extern SL_Mix_t mix_balance(float balance);

extern void mix_2on2_additive(void *output, const void *input, size_t frames, const SL_Mix_t mix);
extern void mix_1on2_additive(void *output, const void *input, size_t frames, const SL_Mix_t mix);

#endif  /* __SL_MIX_H__ */
