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
//
//  Copyright (c) 2011, Auerhaus Development, LLC
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What The Fuck You Want
//  To Public License, Version 2, as published by Sam Hocevar. See
//  http://sam.zoy.org/wtfpl/COPYING for more details.
//

// https://github.com/warrenm/AHEasing/blob/master/AHEasing/easing.c

#include <math.h>

// Modeled after the line y = x
static inline float _linear(float p)
{
    return p;
}

// Modeled after the parabola y = x^2
static inline float _quadratic_in(float p)
{
    return p * p;
}

// Modeled after the parabola y = -x^2 + 2x
static inline float _quadratic_out(float p)
{
    return -(p * (p - 2.0f));
}

// Modeled after the piecewise quadratic
// y = (1/2)((2x)^2)             ; [0, 0.5)
// y = -(1/2)((2x-1)*(2x-3) - 1) ; [0.5, 1]
static inline float _quadratic_in_out(float p)
{
    if (p < 0.5) {
        return 2.0f * p * p;
    } else {
        return (-2.0f * p * p) + (4.0f * p) - 1.0f;
    }
}

// Modeled after the cubic y = x^3
static inline float _cubic_in(float p)
{
    return p * p * p;
}

// Modeled after the cubic y = (x - 1)^3 + 1
static inline float _cubic_out(float p)
{
    float f = (p - 1.0f);
    return f * f * f + 1.0f;
}

// Modeled after the piecewise cubic
// y = (1/2)((2x)^3)       ; [0, 0.5)
// y = (1/2)((2x-2)^3 + 2) ; [0.5, 1]
static inline float _cubic_in_out(float p)
{
    if (p < 0.5f) {
        return 4.0f * p * p * p;
    } else {
        float f = ((2.0f * p) - 2.0f);
        return 0.5f * f * f * f + 1.0f;
    }
}

// Modeled after the quartic x^4
static inline float _quartic_in(float p)
{
    return p * p * p * p;
}

// Modeled after the quartic y = 1 - (x - 1)^4
static inline float _quartic_out(float p)
{
    float f = (p - 1.0f);
    return f * f * f * (1.0f - p) + 1.0f;
}

// Modeled after the piecewise quartic
// y = (1/2)((2x)^4)        ; [0, 0.5)
// y = -(1/2)((2x-2)^4 - 2) ; [0.5, 1]
static inline float _quartic_in_out(float p)
{
    if (p < 0.5f) {
        return 8.0f * p * p * p * p;
    } else {
        float f = (p - 1.0f);
        return -8.0f * f * f * f * f + 1.0f;
    }
}

// Modeled after the quintic y = x^5
static inline float _quintic_in(float p)
{
    return p * p * p * p * p;
}

// Modeled after the quintic y = (x - 1)^5 + 1
static inline float _quintic_out(float p)
{
    float f = (p - 1.0f);
    return f * f * f * f * f + 1.0f;
}

// Modeled after the piecewise quintic
// y = (1/2)((2x)^5)       ; [0, 0.5)
// y = (1/2)((2x-2)^5 + 2) ; [0.5, 1]
static inline float _quintic_in_out(float p)
{
    if (p < 0.5f) {
        return 16.0f * p * p * p * p * p;
    } else {
        float f = ((2.0f * p) - 2.0f);
        return  0.5f * f * f * f * f * f + 1.0f;
    }
}

// Modeled after quarter-cycle of sine wave
static inline float _sine_in(float p)
{
    return sinf((p - 1.0f) * (float)M_PI_2) + 1.0f;
}

// Modeled after quarter-cycle of sine wave (different phase)
static inline float _sine_out(float p)
{
    return sinf(p * (float)M_PI_2);
}

// Modeled after half sine wave
static inline float _sine_in_out(float p)
{
    return 0.5f * (1.0f - cosf(p * (float)M_PI));
}

// Modeled after shifted quadrant IV of unit circle
static inline float _circular_in(float p)
{
    return 1.0f - sqrtf(1.0f - (p * p));
}

// Modeled after shifted quadrant II of unit circle
static inline float _circular_out(float p)
{
    return sqrtf((2.0f - p) * p);
}

// Modeled after the piecewise circular function
// y = (1/2)(1 - sqrt(1 - 4x^2))           ; [0, 0.5)
// y = (1/2)(sqrt(-(2x - 3)*(2x - 1)) + 1) ; [0.5, 1]
static inline float _circular_in_out(float p)
{
    if (p < 0.5f) {
        return 0.5f * (1.0f - sqrtf(1.0f - 4.0f * (p * p)));
    } else {
        return 0.5f * (sqrtf(-((2.0f * p) - 3.0f) * ((2.0f * p) - 1.0f)) + 1.0f);
    }
}

// Modeled after the exponential function y = 2^(10(x - 1))
static inline float _exponential_in(float p)
{
    return (p == 0.0f) ? p : powf(2.0f, 10.0f * (p - 1.0f));
}

// Modeled after the exponential function y = -2^(-10x) + 1
static inline float _exponential_out(float p)
{
    return (p == 1.0f) ? p : 1.0f - powf(2.0f, -10.0f * p);
}

// Modeled after the piecewise exponential
// y = (1/2)2^(10(2x - 1))         ; [0,0.5)
// y = -(1/2)*2^(-10(2x - 1))) + 1 ; [0.5,1]
static inline float _exponential_in_out(float p)
{
    if (p == 0.0f || p == 1.0f) {
        return p;
    } else
    if (p < 0.5f) {
        return 0.5f * powf(2.0f, (20.0f * p) - 10.0f);
    } else {
        return -0.5f * powf(2.0f, (-20.0f * p) + 10.0f) + 1.0f;
    }
}

// Modeled after the damped sine wave y = sin(13pi/2*x)*pow(2, 10 * (x - 1))
static inline float _elastic_in(float p)
{
    return sinf(13.0f * (float)M_PI_2 * p) * powf(2.0f, 10.0f * (p - 1.0f));
}

// Modeled after the damped sine wave y = sin(-13pi/2*(x + 1))*pow(2, -10x) + 1
static inline float _elastic_out(float p)
{
    return sinf(-13.0f * (float)M_PI_2 * (p + 1.0f)) * powf(2.0f, -10.0f * p) + 1.0f;
}

// Modeled after the piecewise exponentially-damped sine wave:
// y = (1/2)*sin(13pi/2*(2*x))*pow(2, 10 * ((2*x) - 1))      ; [0,0.5)
// y = (1/2)*(sin(-13pi/2*((2x-1)+1))*pow(2,-10(2*x-1)) + 2) ; [0.5, 1]
static inline float _elastic_in_out(float p)
{
    if (p < 0.5f) {
        return 0.5f * sinf(13.0f * (float)M_PI_2 * (2.0f * p)) * powf(2.0f, 10.0f * ((2.0f * p) - 1.0f));
    } else {
        return 0.5f * (sinf(-13.0f * (float)M_PI_2 * ((2.0f * p - 1.0f) + 1.0f)) * powf(2.0f, -10.0f * (2.0f * p - 1.0f)) + 2.0f);
    }
}

// Modeled after the overshooting cubic y = x^3-x*sin(x*pi)
static inline float _back_in(float p)
{
    return p * p * p - p * sinf(p * (float)M_PI);
}

// Modeled after overshooting cubic y = 1-((1-x)^3-(1-x)*sin((1-x)*pi))
static inline float _back_out(float p)
{
    float f = (1.0f - p);
    return 1.0f - (f * f * f - f * sinf(f * (float)M_PI));
}

// Modeled after the piecewise overshooting cubic function:
// y = (1/2)*((2x)^3-(2x)*sin(2*x*pi))           ; [0, 0.5)
// y = (1/2)*(1-((1-x)^3-(1-x)*sin((1-x)*pi))+1) ; [0.5, 1]
static inline float _back_in_out(float p)
{
    if (p < 0.5f) {
        float f = 2.0f * p;
        return 0.5f * (f * f * f - f * sinf(f * (float)M_PI));
    } else {
        float f = (1.0f - (2.0f * p - 1.0f));
        return 0.5f * (1.0f - (f * f * f - f * sinf(f * (float)M_PI))) + 0.5f;
    }
}

static inline float _bounce_out(float p)
{
    if (p < 4.0f / 11.0f) {
        return (121.0f * p * p) / 16.0;
    } else
    if (p < 8.0f / 11.0f) {
        return (363.0f / 40.0f * p * p) - (99.0f / 10.0f * p) + 17.0f / 5.0f;
    } else
    if (p < 9.0f / 10.0f) {
        return (4356.0f / 361.0f * p * p) - (35442.0f / 1805.0f * p) + 16061.0f / 1805.0f;
    } else {
        return (54.0f / 5.0f * p * p) - (513.0f / 25.0f * p) + 268.0f / 25.0f;
    }
}

static inline float _bounce_in(float p)
{
    return 1.0f - _bounce_out(1.0f - p);
}

static inline float _bounce_in_out(float p)
{
    if (p < 0.5f) {
        return 0.5f * _bounce_in(p * 2.0f);
    } else {
        return 0.5f * _bounce_out(p * 2.0f - 1.0f) + 0.5f;
    }
}
