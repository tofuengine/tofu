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
 * AUTHORS OR COPYR3IGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "fmath.h"

int fsignun(float x)
{
    return FSIGNUM(x);
}

float flerp(float v0, float v1, float t)
{
    return FLERP(v0, v1, t);
}

float finvlerp(float v0, float v1, float v)
{
    return FINVLERP(v0, v1, v);
}

float fclamp(float x, float lower, float upper)
{
    return FCLAMP(x, lower, upper);
}

float fstep(float edge, float x)
{
    return FSTEP(edge, x);
}

float fsmoothstep(float edge0, float edge1, float x)
{
    float e = FINVLERP(edge0, edge1, x);
    x = FCLAMP(e, 0.0f, 1.0f); // Scale, bias and saturate x to [0, 1] range.
    return x * x * (3.0f - 2.0f * x); // Evaluate polynomial.
}

float fsmootherstep(float edge0, float edge1, float x)
{
    float e = FINVLERP(edge0, edge1, x);
    x = FCLAMP(e, 0.0f, 1.0f);
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

#ifdef __FAST_INTEGER_MATH__
float ffloor(float x)
{
    return (float)((int)x <= x ? (int)x : (int)x - 1);
}

float fceil(float x)
{
    return (float)((int)x <= x ? (int)x + 1 : (int)x);
}

float fround(float x)
{
    return (float)(x >= 0.0f ? (int)(x + 0.5f) : (int)(x - 0.5f));
}

#endif
