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

#include "wave.h"

#include <libs/fmath.h>

float wave_sine(float t)
{
    float value = sinf(t * F_2PI);
    return value;
}

float wave_square(float t)
{
    float value = 2.0f * (2.0f * floorf(t) - floorf(2.0f * t)) + 1.0f;
    return value;
}

float wave_triangle(float t)
{
    float value = 2.0f * fabsf(2.0f * (t + 0.25f - floorf(t + 0.75f))) - 1.0f;
    return value;
}

float wave_sawtooth(float t)
{
    float value = 2.0f * (t - floorf(0.5f + t));
    return value;
}
