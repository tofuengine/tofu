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

#ifndef __SL_SAMPLE_H__
#define __SL_SAMPLE_H__

#include <miniaudio/miniaudio.h>

#include "source.h"

#include <stddef.h>

typedef size_t (*SL_Sample_Read_Callback_t)(void *user_data, void *output, size_t frames_requested);

extern SL_Source_t *SL_sample_create(SL_Sample_Read_Callback_t on_read, void *user_data, size_t length_in_frames, ma_format format, ma_uint32 sample_rate, ma_uint32 channels);

#endif  /* __SL_SAMPLE_H__ */