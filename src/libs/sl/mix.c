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

#ifndef M_PI_2
  #define M_PI_2    1.57079632679489661923
#endif

// Add to the `accumulator` the `sample` scaled by `gain`.
//
// Note that, due to scaling, the intermediate `sample * gain` value can exceed the sample maximum/minimum value.
// We are clamping after the accumulation to save one operation.
//
// Also note that we are safe using a `float`, over a (for example) fixed-point 24:8 value. We are not going to
// loose resolution during the computation.
#if SL_BYTES_PER_SAMPLE == 2
static inline int16_t _accumulate_s16(int16_t accumulator, int16_t left_sample, float left_gain, int16_t right_sample, float right_gain)
{
    const int32_t result = (int32_t)((float)accumulator + (float)left_sample * left_gain + (float)right_sample * right_gain);
    if (result >= INT16_MAX) {
        return INT16_MAX;
    }
    if (result <= INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)result;
}
#elif SL_BYTES_PER_SAMPLE == 4
static inline float _accumulate_f32(float accumulator, float left_sample, float left_gain, float right_sample, float right_gain)
{
    const float result = accumulator + left_sample * left_gain + right_sample * right_gain;
    if (result >= 1.0f) {
        return 1.0f;
    }
    if (result <= -1.0f) {
        return -1.0f;
    }
    return result;
}
#endif

//
// | L/L R/L |   | L |
// |         | * |   | = | L/L * L + R/L * R, L/R * L + R/R * R |
// | L/R R/R |   | R |
//
void mix_2on2_additive(void *output, const void *input, size_t frames, const SL_Mix_t mix)
{
    const float left_to_left = mix.left_to_left;
    const float left_to_right = mix.left_to_right;
    const float right_to_left = mix.right_to_left;
    const float right_to_right = mix.right_to_right;

#if SL_BYTES_PER_SAMPLE == 2
    const int16_t *sptr = input;
    int16_t *dptr = output;
    for (size_t i = frames; i--; dptr += 2, sptr += 2) {
        const int16_t left = sptr[0];
        const int16_t right = sptr[1];
        dptr[0] = _accumulate_s16(dptr[0], left, left_to_left, right, right_to_left);
        dptr[1] = _accumulate_s16(dptr[1], left, left_to_right, right, right_to_right);
    }
#elif SL_BYTES_PER_SAMPLE == 4
    const float *sptr = input;
    float *dptr = output;
    for (size_t i = frames; i--; dptr += 2, sptr += 2) {
        const float left = sptr[0];
        const float right = sptr[1];
        dptr[0] = _accumulate_f32(dptr[0], left, left_to_left, right, right_to_left);
        dptr[1] = _accumulate_f32(dptr[1], left, left_to_right, right, right_to_right);
    }
#else
  #error Wrong internal format.
#endif
}

void mix_1on2_additive(void *output, const void *input, size_t frames, const SL_Mix_t mix)
{
    const float left_to_left = mix.left_to_left;
    const float left_to_right = mix.left_to_right;
    const float right_to_left = mix.right_to_left;
    const float right_to_right = mix.right_to_right;

#if SL_BYTES_PER_SAMPLE == 2
    const int16_t *sptr = input;
    int16_t *dptr = output;
    for (size_t i = frames; i--; dptr += 2, sptr += 1) {
        const int16_t left = sptr[0];
        const int16_t right = sptr[0];
        dptr[0] = _accumulate_s16(dptr[0], left, left_to_left, right, right_to_left);
        dptr[1] = _accumulate_s16(dptr[1], left, left_to_right, right, right_to_right);
    }
#elif SL_BYTES_PER_SAMPLE == 4
    const float *sptr = input;
    float *dptr = output;
    for (size_t i = frames; i--; dptr += 2, sptr += 1) {
        const float left = sptr[0];
        const float right = sptr[0];
        dptr[0] = _accumulate_f32(dptr[0], left, left_to_left, right, right_to_left);
        dptr[1] = _accumulate_f32(dptr[1], left, left_to_right, right, right_to_right);
    }
#else
  #error Wrong internal format.
#endif
}

// Thread the stereo source as two seperate mono channels and pan the individually.
SL_Mix_t mix_twin_pan(float left_pan, float right_pan)
{
#if __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_GAIN
    const float left_theta = (left_pan + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    const float right_theta = (right_pan + 1.0f) * 0.5f;
    return (SL_Mix_t){ // powf(theta, 1)
            .left_to_left = 1.0f - left_theta, .left_to_right = left_theta
            .right_to_left = 1.0f - right_theta, .right_to_right = right_theta
        };
#elif __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_POWER_SINCOS
    const float left_theta = (left_pan + 1.0f) * 0.5f * (float)M_PI_2; // [-1, 1] -> [0 , 1] -> [0, pi/2]
    const float right_theta = (right_pan + 1.0f) * 0.5f * (float)M_PI_2;
    return (SL_Mix_t){
            .left_to_left = cosf(left_theta), .left_to_right = sinf(left_theta),
            .right_to_left = cosf(right_theta), .right_to_right = sinf(right_theta),
        };
#elif __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_POWER_SQRT
    const float left_theta = (left_pan + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    const float right_theta = (right_pan + 1.0f) * 0.5f;
    return (SL_Mix_t){ // powf(theta, 0.5)
            .left_to_left = sqrtf(1.0f - left_theta), .left_to_right = sqrtf(left_theta)
            .right_to_left = sqrtf(1.0f - right_theta), .right_to_right = sqrtf(right_theta)
        };
#endif
}

SL_Mix_t mix_pan(float pan)
{
#if __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_GAIN
    const float theta = (pan + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    return (SL_Mix_t){ .left_to_left = 1.0f - theta, .right_to_right = theta }; // powf(theta, 1)
#elif __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_POWER_SINCOS
    const float theta = (pan + 1.0f) * 0.5f * (float)M_PI_2; // [-1, 1] -> [0 , 1] -> [0, pi/2]
    return (SL_Mix_t){ .left_to_left = cosf(theta), .right_to_right = sinf(theta) };
#elif __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_POWER_SQRT
    const float theta = (pan + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    return (SL_Mix_t){ .left_to_left = sqrtf(1.0f - theta), .right_to_right = sqrtf(theta) }; // powf(theta, 0.5)
#endif
}

// The balance law differs from the panning law in the fact that when on center the channels are 0dB.
SL_Mix_t mix_balance(float balance)
{
#if __SL_BALANCE_LAW__ == BALANCE_LAW_LINEAR
    if (balance < 0.0f) {
        return (SL_Mix_t){ .left_to_left = 1.0f, .right_to_right = 1.0f + balance };
    } else
    if (balance > 0.0f) {
        return (SL_Mix_t){ .left_to_left = 1.0f - balance, .right_to_right = 1.0f };
    } else {
        return (SL_Mix_t){ .left_to_left = 1.0f, .right_to_right = 1.0f };
    }
#elif __SL_BALANCE_LAW__ == BALANCE_LAW_SINCOS
    if (balance < 0.0f) {
        return (SL_Mix_t){ .left_to_left = 1.0f, .right_to_right = sinf((1.0f + balance) * (float)M_PI_2) };
    } else
    if (balance > 0.0f) {
        return (SL_Mix_t){ .left_to_left = sinf((1.0f - balance) * (float)M_PI_2), .right_to_right = 1.0f };
    } else {
        return (SL_Mix_t){ .left_to_left = 1.0f, .right_to_right = 1.0f };
    }
#elif __SL_BALANCE_LAW__ == BALANCE_LAW_SQRT
    if (balance < 0.0f) {
        return (SL_Mix_t){ .left_to_left = 1.0f, .right_to_right = sqrtf(1.0f + balance) };
    } else
    if (balance > 0.0f) {
        return (SL_Mix_t){ .left_to_left = sqrtf(1.0f - balance), .right_to_right = 1.0f };
    } else {
        return (SL_Mix_t){ .left_to_left = 1.0f, .right_to_right = 1.0f };
    }
#endif
}

