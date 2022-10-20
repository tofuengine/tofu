/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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

#ifndef __LIBS_FMATH_H__
#define __LIBS_FMATH_H__

#include <math.h>

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

#endif  /* __LIBS_FMATH_H__ */
