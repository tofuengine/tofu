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

#include "mix.h"

#include <config.h>

#include <math.h>
#include <stdint.h>

#ifndef M_PI
  #define M_PI      3.14159265358979323846f
#endif
#ifndef M_PI_2
  #define M_PI_2    1.57079632679489661923f
#endif

extern void mix_additive(void *output, void *input, size_t frames, const SL_Mix_t mix)
{
    const float left = mix.left; // Apply panning and gain to the data.
    const float right = mix.right;

#if SL_BYTES_PER_FRAME == 2
    int16_t *sptr = input;
    int16_t *dptr = output;

    for (size_t i = frames; i; --i) {
        *(dptr++) += (int16_t)((float)*(sptr++) * left);
        *(dptr++) += (int16_t)((float)*(sptr++) * right);
    }
#elif SL_BYTES_PER_FRAME == 4
    float *sptr = input;
    float *dptr = output;

    for (size_t i = frames; i; --i) {
        *(dptr++) += *(sptr++) * left;
        *(dptr++) += *(sptr++) * right;
    }
#else
  #error Wrong internal format.
#endif
}

SL_Mix_t mix_precompute_pan(float pan, float gain)
{
#if __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_GAIN
    const float theta = (pan + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    return (SL_Mix_t){ .left = (1.0f - theta) * gain, .right = theta * gain }; // powf(theta, 1)
#elif __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_POWER_SINCOS
    const float theta = (pan + 1.0f) * 0.5f * M_PI_2; // [-1, 1] -> [0 , 1] -> [0, pi/2]
    return (SL_Mix_t){ .left = cosf(theta) * gain, .right = sinf(theta) * gain };
#elif __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_POWER_SQRT
    const float theta = (pan + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    return (SL_Mix_t){ .left = sqrtf(1.0f - theta) * gain, .right = sqrtf(theta) * gain }; // powf(theta, 0.5)
#endif
}

// The balance law differs from the panning law in the fact that when on center the channels are 0dB.
SL_Mix_t mix_precompute_balance(float balance, float gain)
{
#if __SL_BALANCE_LAW__ == BALANCE_LAW_LINEAR
    if (balance < 0.0f) {
        return (SL_Mix_t){ .left = gain, .right = (1.0f + balance) * gain };
    } else
    if (balance > 0.0f) {
        return (SL_Mix_t){ .left = (1.0f - balance) * gain, .right = gain };
    } else {
        return (SL_Mix_t){ .left = gain, .right = gain };
    }
#elif __SL_BALANCE_LAW__ == BALANCE_LAW_SINCOS
    if (balance < 0.0f) {
        return (SL_Mix_t){ .left = gain, .right = sinf((1.0f + balance) * M_PI_2) * gain };
    } else
    if (balance > 0.0f) {
        return (SL_Mix_t){ .left = sinf((1.0f - balance) * M_PI_2) * gain, .right = gain };
    } else {
        return (SL_Mix_t){ .left = gain, .right = gain };
    }
#elif __SL_BALANCE_LAW__ == BALANCE_LAW_SQRT
    if (balance < 0.0f) {
        return (SL_Mix_t){ .left = gain, .right = sqrtf(1.0f + balance) * gain };
    } else
    if (balance > 0.0f) {
        return (SL_Mix_t){ .left = sqrtf(1.0f - balance) * gain, .right = gain };
    } else {
        return (SL_Mix_t){ .left = gain, .right = gain };
    }
#endif
}

