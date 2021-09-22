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

#include "easing.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
  #define M_PI      3.14159265358979323846264f
#endif
#ifndef M_PI_2
  #define M_PI_2    1.57079632679489661923132f
#endif

static const Easing_t _entries[] = {
    { "linear", easing_linear },
    { "quadratic-in", easing_quadratic_in },
    { "quadratic-out", easing_quadratic_out },
    { "quadratic-in-out", easing_quadratic_in_out },
    { "cubic-in", easing_cubic_in },
    { "cubic-out", easing_cubic_out },
    { "cubic-in-out", easing_cubic_in_out },
    { "quartic-in", easing_quartic_in },
    { "quartic-out", easing_quartic_out },
    { "quartic-in-out", easing_quartic_in_out },
    { "quintic-in", easing_quintic_in },
    { "quintic-out", easing_quintic_out },
    { "quintic-in-out", easing_quintic_in_out },
    { "sine-in", easing_sine_in },
    { "sine-out", easing_sine_out },
    { "sine-in-out", easing_sine_in_out },
    { "circular-in", easing_circular_in },
    { "circular-out", easing_circular_out },
    { "circular-in-out", easing_circular_in_out },
    { "exponential-in", easing_exponential_in },
    { "exponential-out", easing_exponential_out },
    { "exponential-in-out", easing_exponential_in_out },
    { "elastic-in", easing_elastic_in },
    { "elastic-out", easing_elastic_out },
    { "elastic-in-out", easing_elastic_in_out },
    { "back-in", easing_back_in },
    { "back-out", easing_back_out },
    { "back-in-out", easing_back_in_out },
    { "bounce-out", easing_bounce_out },
    { "bounce-in", easing_bounce_in },
    { "bounce-in-out", easing_bounce_in_out },
    { NULL, NULL }
};

const Easing_t *easing_from_id(const char *id)
{
    for (const Easing_t *entry = _entries; entry->id; ++entry) {
        if (strcasecmp(entry->id, id) == 0) {
            return entry;
        }
    }
    return NULL;
}

float easing_linear(float p)
{
    return p;
}

float easing_quadratic_in(float p)
{
    return p * p;
}

float easing_quadratic_out(float p)
{
    return -(p * (p - 2.0f));
}

float easing_quadratic_in_out(float p)
{
    p *= 2.0f;
    if (p < 1) {
        return 0.5f * p * p;
    } else {
        p -= 1.0f;
        return -0.5f * (p * (p - 2.0f) - 1.0f);
    }
}

float easing_cubic_in(float p)
{
    return p * p * p;
}

float easing_cubic_out(float p)
{
    float f = (p - 1.0f);
    return f * f * f + 1.0f;
}

float easing_cubic_in_out(float p)
{
    if (p < 0.5f) {
        return 4.0f * p * p * p;
    } else {
        float f = ((2.0f * p) - 2.0f);
        return 0.5f * f * f * f + 1.0f;
    }
}

float easing_quartic_in(float p)
{
    return p * p * p * p;
}

float easing_quartic_out(float p)
{
    float f = (p - 1.0f);
    return 1.0f - f * f * f * f;
}

float easing_quartic_in_out(float p)
{
    if (p < 0.5f) {
        return 8.0f * p * p * p * p;
    } else {
        float f = (p - 1.0f);
        return -8.0f * f * f * f * f + 1.0f;
    }
}

float easing_quintic_in(float p)
{
    return p * p * p * p * p;
}

float easing_quintic_out(float p)
{
    float f = (p - 1.0f);
    return f * f * f * f * f + 1.0f;
}

float easing_quintic_in_out(float p)
{
    if (p < 0.5f) {
        return 16.0f * p * p * p * p * p;
    } else {
        float f = ((2.0f * p) - 2.0f);
        return  0.5f * f * f * f * f * f + 1.0f;
    }
}

float easing_sine_in(float p)
{
    return 1.0f - cosf(p * (float)M_PI_2);
}

float easing_sine_out(float p)
{
    return sinf(p * (float)M_PI_2);
}

float easing_sine_in_out(float p)
{
    return 0.5f - 0.5f * cosf(p * (float)M_PI);
}

float easing_circular_in(float p)
{
    return 1.0f - sqrtf(1.0f - (p * p));
}

float easing_circular_out(float p)
{
    return sqrtf((2.0f - p) * p);
}

float easing_circular_in_out(float p)
{
    if (p < 0.5f) {
        return 0.5f * (1.0f - sqrtf(1.0f - 4.0f * (p * p)));
    } else {
        return 0.5f * (sqrtf(-((2.0f * p) - 3.0f) * ((2.0f * p) - 1.0f)) + 1.0f);
    }
}

float easing_exponential_in(float p)
{
    return (p == 0.0f) ? 0.0f : powf(2.0f, 10.0f * (p - 1.0f));
}

float easing_exponential_out(float p)
{
    return (p == 1.0f) ? 1.0f : 1.0f - powf(2.0f, -10.0f * p);
}

float easing_exponential_in_out(float p)
{
    if (p == 0.0f) {
        return 0.0f;
    } else
    if (p == 1.0f) {
        return 1.0f;
    } else
    if (p < 0.5f) {
        return 0.5f * powf(2.0f, 20.0f * (p - 0.5f));
    } else {
        return -0.5f * (powf(2.0f, -20.0f * (p - 0.5f)) - 2.0f);
    }
}

float easing_elastic_in(float p)
{
    static const float k = 0.3f;

    if (p == 0.0f) {
        return 0.0f;
    } else
    if (p == 1.0f) {
        return 1.0f;
    } else {
        p -= 1.0f;
        float f = powf(2.0f, 10.0f * p);
        return 0.0f - f * sinf((p / k - 0.25f) * (2.0f * (float)M_PI));
    }
}

float easing_elastic_out(float p)
{
    static const float k = 0.3f;

    if (p == 0.0f) {
        return 0.0f;
    } else
    if (p == 1.0f) {
        return 1.0f;
    } else {
        float f = powf(2.0f, -10.0f * p);
        return 1.0f + f * sinf((p / k - 0.25f) * (2.0f * (float)M_PI));
    }
}

float easing_elastic_in_out(float p)
{
    static const float k = 0.3f * 1.5f;

    if (p == 0.0f) {
        return 0.0f;
    } else
    if (p == 1.0f) {
        return 1.0f;
    } else
    if (p < 0.5f) {
        p -= 0.5f;
        float f = powf(2.0f, 20.0f * p);
        return 0.0f - 0.5f * f * sinf((p / k - 0.25f) * (2.0f * (float)M_PI));
    } else {
        p -= 0.5f;
        float f = powf(2.0f, -20.0f * p);
        return 1.0f + 0.5f * f * sinf((p / k - 0.25f) * (2.0f * (float)M_PI));
    }
}

float easing_back_in(float p)
{
    static const float s = 1.70158f;
    return p * p * ((s + 1.0f) * p - s);
}

float easing_back_out(float p)
{
    static const float s = 1.70158f;
    p -= 1.0f;
    return p * p * ((s + 1.0f) * p + s) + 1.0f;
}

float easing_back_in_out(float p)
{
    static const float s = 1.70158f * 1.525f;
    p *= 2.0f;
    if (p < 1.0f) {
        return 0.5f * (p * p * ((s + 1) * p - s));
    } else {
        p -= 2.0f;
        return 0.5f * (p * p * ((s + 1.0f) * p + s) + 2.0f);
    }
}

float easing_bounce_in(float p)
{
    return 1.0f - easing_bounce_out(1.0f - p);
}

float easing_bounce_out(float p)
{
    if (p < (1.0f / 2.75f)) {
        return (7.5625f * p * p);
    } else if (p < (2.0f / 2.75f)) {
        p -= 1.5f / 2.75f;
        return (7.5625f * p * p + 0.75f);
    } else if (p < (2.5f / 2.75f)) {
        p -= 2.25f / 2.75f;
        return (7.5625f * p * p + 0.9375f);
    } else {
        p -= 2.625f / 2.75f;
        return (7.5625f * p * p + 0.984375f);
    }
}

float easing_bounce_in_out(float p)
{
    if (p < 0.5f) {
        return 0.5f * easing_bounce_in(p * 2.0f);
    } else {
        return 0.5f * easing_bounce_out(p * 2.0f - 1.0f) + 0.5f;
    }
}
