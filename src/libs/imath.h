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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TOFU_LIBS_IMATH_H
#define TOFU_LIBS_IMATH_H

#include <math.h>

#define IABS(v)         ((v) > 0 ? (v) : -(v))
#define IMOD(a, b)      ((((a) % (b)) + (b)) % (b))
#define IMIN(a, b)      ((a) < (b) ? (a) : (b))
#define IMAX(a, b)      ((a) > (b) ? (a) : (b))

#define ISIGNUM(x)      (((x) > 0) - ((x) < 0))

#define ICLAMP(x, l, u) ((x) < (l) ? (l) : ((x) > (u) ? (u) : (x)))
#define IMIRROR(x)      ((x) >= 0 ? (x) : -(1 + (x)))

#define ITRUNC(x)       ((int)(x))
#define INEAREST(x)     ((int)((x) + 0.5f))

#ifdef __FAST_INTEGER_MATH__
#define IFLOORF(x)      (ifloor((x)))
#define ICEILF(x)       (iceil((x)))
#define IROUNDF(x)      (ifloor((x) + 0.5f))
#else
#define IFLOORF(x)      ((int)floorf((x)))
#define ICEILF(x)       ((int)ceilf((x)))
#define IROUNDF(x)      ((int)roundf((x)))
#endif

extern int iabs(int v);
extern int imod(int a, int b);
extern int imin(int a, int b);
extern int imax(int a, int b);

#ifdef __FAST_INTEGER_MATH__
extern int ifloor(float x);
extern int iceil(float x);
#endif

#endif  /* TOFU_LIBS_IMATH_H */
