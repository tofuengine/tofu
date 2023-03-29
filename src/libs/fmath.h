/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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

#ifndef TOFU_LIBS_FMATH_H
#define TOFU_LIBS_FMATH_H

#include <core/config.h>

#include <math.h>

#define F_E         2.7182818284590452354f
#define F_LOG2E     1.4426950408889634074f
#define F_LOG10E    0.43429448190325182765f
#define F_LN2       0.69314718055994530942f
#define F_LN10      2.30258509299404568402f
#define F_2PI       6.28318530717958647693f
#define F_PI        3.14159265358979323846f
#define F_PI_2      1.57079632679489661923f
#define F_PI_4      0.78539816339744830962f
#define F_1_PI      0.31830988618379067154f
#define F_2_PI      0.63661977236758134308f
#define F_2_SQRTPI  1.12837916709551257390f
#define F_SQRT2     1.41421356237309504880f
#define F_1_SQRT2   0.70710678118654752440f

#define FABS(v)             ((v) > 0.0f ? (v) : -(v))
//#define IMOD(a, b)          ((((a) % (b)) + (b)) % (b))
#define FMIN(a, b)          ((a) < (b) ? (a) : (b))
#define FMAX(a, b)          ((a) > (b) ? (a) : (b))

//#define FSIGNUM(x)          ((x) > 0.0f ? 1 : ((x) < 0.0f ? -1 : 0))
#define FSIGNUM(x)          (((x) > 0.0f) - ((x) < 0.0f))

// Arguments order matches Khronos' one.
// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/clamp.xhtml
#define FCLAMP(x, l, u)     ((x) < (l) ? (l) : ((x) > (u) ? (u) : (x)))
#define FMIRROR(x)          ((x) >= 0.0f ? (x) : -(1.0f + (x)))

// More numerical stable than the following one.
//   #define FLERP(v0, v1, t)    (((v1) - (v0)) * (t) + (v0))
// see: https://en.wikipedia.org/wiki/Linear_interpolation
#define FLERP(v0, v1, t)    ((v0) * (1.0f - (t)) + (v1) * (t))
#define FINVLERP(v0, v1, v) (((v) - (v0)) / ((v1) - (v0)))
#define FSTEP(e, x)         ((x) < (e) ? 0.0f : 1.0f)

#ifdef __FAST_FLOAT_MATH__
#define FFLOOR(x)           (ffloor((x)))
#define FCEIL(x )           (fceil((x)))
#define FROUND(x)           (ffloor((x) + 0.5f))
#else
#define FFLOOR(x)           (floorf((x)))
#define FCEIL(x )           (ceilf((x)))
#define FROUND(x)           (roundf((x)))
#endif

extern int fsignun(float x);
extern float flerp(float v0, float v1, float t);
extern float finvlerp(float v0, float v1, float v);
extern float fclamp(float x, float lower, float upper);
extern float fstep(float edge, float x);
extern float fsmoothstep(float edge0, float edge1, float x);
extern float fsmootherstep(float edge0, float edge1, float x);

#ifdef __FAST_INTEGER_MATH__
extern float ffloor(float x);
extern float fceil(float x);
#endif

#endif  /* TOFU_LIBS_FMATH_H */
