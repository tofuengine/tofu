/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2026 Marco Lizza
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

#ifndef TOFU_LIBS_FPMATH_H
#define TOFU_LIBS_FPMATH_H

#define FPMATH_H_INCLUDED

#include <stdint.h>

/*
 * The library refers to the actual type as `fixed32_t` as internally it is
 * just an integer.
 * If not specified otherwise, the fixed-point numbers are represented as 16.16
 * (i.e. 16 bits for the integer part and 16 bits for the fractional part).
 */

#if !defined(FIXED32_FRACTIONAL_BITS)
    #define FIXED32_FRACTIONAL_BITS 16
    #define FIXED32_FRACTIONAL_MASK 0x0000FFFF
    #define FIXED32_INTEGER_MASK 0xFFFF0000
#endif  /* !defined(FIXED32_FRACTIONAL_BITS) */

#define FIXED32_ONE (1 << FIXED32_FRACTIONAL_BITS)
#define FIXED32_ONE_HALF (FIXED32_ONE >> 1)

//typedef int32_t fp32_16_t;
typedef int32_t fixed32_t;

// All the functions are defined as `static inline` to allow the compiler to
// optimize them as much as possible, and to avoid potential issues with
// multiple definitions when included in different translation units.
static inline fixed32_t fixed32_from_int(int v)
{
    return (fixed32_t)(v << FIXED32_FRACTIONAL_BITS);
}

static inline fixed32_t fixed32_from_float(float v)
{
    return (fixed32_t)(v * (float)FIXED32_ONE);
}

static inline fixed32_t fixed32_from_float_round(float v)
{
    return (fixed32_t)(v * (float)FIXED32_ONE + (v >= 0 ? 0.5f : -0.5f));
}

static inline fixed32_t fixed32_from_double(double v)
{
    return (fixed32_t)(v * (double)FIXED32_ONE);
}

static inline fixed32_t fixed32_from_double_round(double v)
{
    return (fixed32_t)(v * (double)FIXED32_ONE + (v >= 0 ? 0.5 : -0.5));
}

static inline int fixed32_to_int(fixed32_t v)
{
    return v >> FIXED32_FRACTIONAL_BITS;
}

static inline int fixed32_to_int_round(fixed32_t v)
{
    if (v >= 0) {
        return (v + FIXED32_ONE_HALF) >> FIXED32_FRACTIONAL_BITS;
    }
    return (v - FIXED32_ONE_HALF) >> FIXED32_FRACTIONAL_BITS;
}

static inline float fixed32_to_float(fixed32_t v)
{
    return (float)v / (float)FIXED32_ONE;
}

static inline float fixed32_to_float_round(fixed32_t v)
{
    if (v >= 0) {
        return (float)(v + FIXED32_ONE_HALF) / (float)FIXED32_ONE;
    }
    return (float)(v - FIXED32_ONE_HALF) / (float)FIXED32_ONE;
}

static inline double fixed32_to_double(fixed32_t v)
{
    return (double)v / (double)FIXED32_ONE;
}

static inline double fixed32_to_double_round(fixed32_t v)
{
    if (v >= 0) {
        return (double)(v + FIXED32_ONE_HALF) / (double)FIXED32_ONE;
    }
    return (double)(v - FIXED32_ONE_HALF) / (double)FIXED32_ONE;
}

static inline fixed32_t fixed32_floor(fixed32_t v)
{
    return v & FIXED32_INTEGER_MASK;
}

static inline fixed32_t fixed32_ceil(fixed32_t v)
{
    if (v >= 0) {
        return (v & FIXED32_INTEGER_MASK) + ((v & FIXED32_FRACTIONAL_MASK) ? FIXED32_ONE : 0);
    }
    return (v & FIXED32_INTEGER_MASK) - ((v & FIXED32_FRACTIONAL_MASK) ? FIXED32_ONE : 0);
}

static inline fixed32_t fixed32_round(fixed32_t v)
{
    if (v >= 0) {
        return (v + FIXED32_ONE_HALF) & FIXED32_INTEGER_MASK;
    }
    return (v - FIXED32_ONE_HALF) & FIXED32_INTEGER_MASK;
}

// Helper macros. The user can still call the functions directly if they want
// to, but these macros provide a more convenient interface. Also, they allow
// to switch beetween rounding and non-rounding versions of the functions by
// defining the `FIXED32_NO_ROUNDING` macro.
#define FIXED32_FROM_INT(v) fixed32_from_int(v)
#if !defined(FIXED32_NO_ROUNDING)
    #define FIXED32_FROM_FLOAT(v) fixed32_from_float_round(v)
    #define FIXED32_FROM_DOUBLE(v) fixed32_from_double_round(v)
    #define FIXED32_TO_INT(v) fixed32_to_int_round(v)
    #define FIXED32_TO_FLOAT(v) fixed32_to_float_round(v)
    #define FIXED32_TO_DOUBLE(v) fixed32_to_double_round(v)
#else   /* !defined(FIXED32_NO_ROUNDING) */
    #define FIXED32_FROM_FLOAT(v) fixed32_from_float(v)
    #define FIXED32_FROM_DOUBLE(v) fixed32_from_double(v)
    #define FIXED32_TO_INT(v) fixed32_to_int(v)
    #define FIXED32_TO_FLOAT(v) fixed32_to_float(v)
    #define FIXED32_TO_DOUBLE(v) fixed32_to_double(v)
#endif  /* !defined(FIXED32_NO_ROUNDING) */

#define FIXED32_ITRUNC(v) fixed32_to_int(v)
#define FIXED32_IROUND(v) fixed32_to_int_round(v)

#endif  /* TOFU_LIBS_FPMATH_H */