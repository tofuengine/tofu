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
 * The library refers to the actual type as `fix32_t` as internally it is
 * just an integer.
 * If not specified otherwise, the fixed-point numbers are represented as 16.16
 * (i.e. 16 bits for the integer part and 16 bits for the fractional part).
 */

#if !defined(FIX32_FRACTIONAL_BITS)
    #define FIX32_FRACTIONAL_BITS 16
#endif  /* !defined(FIX32_FRACTIONAL_BITS) */

#if FIX32_FRACTIONAL_BITS < 1 || FIX32_FRACTIONAL_BITS > 30
    #error "FIX32_FRACTIONAL_BITS must be between 1 and 30 (inclusive)."
#endif  /* FIX32_FRACTIONAL_BITS < 1 || FIX32_FRACTIONAL_BITS > 30 */

// Retain signedness with these macros, for the masks.
#define FIX32_ONE             (INT32_C(1) << FIX32_FRACTIONAL_BITS)
#define FIX32_ONE_HALF        (FIX32_ONE >> 1)
#define FIX32_FRACTIONAL_MASK (FIX32_ONE - INT32_C(1))
#define FIX32_INTEGER_MASK    (~FIX32_FRACTIONAL_MASK)

// The user can define `FIX32_USE_64_BIT` to use 64-bit integers for the
// intermediate calculations. However, we leave this as optional in case we
// know that the represented values will not exceed internally `INT32_MAX`,
// as we can have faster operations with 32-bit integers.
//
// Define the macro below to represent the whole `fix32_t` range.

//#define FIX32_USE_64_BIT

//typedef int32_t fp32_16_t;
typedef int32_t fix32_t;

// All the functions are defined as `static inline` to allow the compiler to
// optimize them as much as possible, and to avoid potential issues with
// multiple definitions when included in different translation units.

// No check are performed on the input values, as the user is expected to know
// what they are doing when using fixed-point arithmetic and not cause undefined
// behavior by overflowing the intermediate calculations.
static inline fix32_t fix32_from_int(int v)
{
#if defined(FIX32_USE_64_BIT)
    return (fix32_t)(v * (int64_t)FIX32_ONE);
#else   /* defined(FIX32_USE_64_BIT) */
    return (fix32_t)(v * (int32_t)FIX32_ONE);
#endif  /* defined(FIX32_USE_64_BIT) */
}

static inline fix32_t fix32_from_float(float v)
{
    return (fix32_t)(v * (float)FIX32_ONE);
}

static inline fix32_t fix32_from_float_round(float v)
{
    return (fix32_t)(v * (float)FIX32_ONE + (v >= 0 ? 0.5f : -0.5f));
}

static inline fix32_t fix32_from_double(double v)
{
    return (fix32_t)(v * (double)FIX32_ONE);
}

static inline fix32_t fix32_from_double_round(double v)
{
    return (fix32_t)(v * (double)FIX32_ONE + (v >= 0 ? 0.5 : -0.5));
}

static inline int fix32_to_int_floor(fix32_t v)
{
    // Floors toward negative infinity, which is what we want for negative numbers.
    //
    // It's an implementation-defined behavior if `v` is negative and has a
    // fractional part, but it works as expected on two's complement
    // architectures, which are the vast majority of modern ones.
    //
    // Note: we are using bitwise shifting only to calculate the floor value.
    return v >> FIX32_FRACTIONAL_BITS; 
}

static inline int fix32_to_int_trunc(fix32_t v)
{
    return v / FIX32_ONE; // This will truncate toward zero, which is what we want for negative numbers.
}

static inline int fix32_to_int_round(fix32_t v)
{
#if defined(FIX32_USE_64_BIT)
    int64_t aux = (int64_t)v; // Use `int64_t` to avoid overflow when adding `FIX32_ONE_HALF`.
#else   /* defined(FIX32_USE_64_BIT) */
    int32_t aux = v;
#endif
    aux += v >= 0 ? FIX32_ONE_HALF : -FIX32_ONE_HALF;
    return (int)(aux / FIX32_ONE); // Dividing (not shifting) will round to the nearest integer, with ties rounding away from zero, which is what we want for negative numbers.
}

static inline float fix32_to_float(fix32_t v)
{
    return (float)v / (float)FIX32_ONE; // No need to add `FIX32_ONE_HALF` for rounding, as the division will already round to the nearest float.
}

static inline double fix32_to_double(fix32_t v)
{
    return (double)v / (double)FIX32_ONE; // Ditto.
}

static inline fix32_t fix32_floor(fix32_t v)
{
    return v & FIX32_INTEGER_MASK;
}

static inline fix32_t fix32_ceil(fix32_t v)
{
    // The masking already produces a "floor" value, so we don't need to check
    // for the sign of `v` to decide if we need to add `FIX32_ONE` or not, as
    // the result will be the same.
    return (v & FIX32_INTEGER_MASK) + ((v & FIX32_FRACTIONAL_MASK) ? FIX32_ONE : 0);
}

static inline fix32_t fix32_round(fix32_t v)
{
    int aux = fix32_to_int_round(v); // Convert and rescale, to avoid flooring issues with negative numbers.
    return fix32_from_int(aux);
}

// Helper macros. The user can still call the functions directly if they want
// to, but these macros provide a more convenient interface. Also, they allow
// to switch beetween rounding and non-rounding versions of the functions by
// defining the `FIX32_NO_ROUNDING` macro.
#define FIX32_FROM_INT(v) fix32_from_int(v)
#if !defined(FIX32_NO_ROUNDING)
    #define FIX32_FROM_FLOAT(v) fix32_from_float_round(v)
    #define FIX32_FROM_DOUBLE(v) fix32_from_double_round(v)
    #define FIX32_TO_INT(v) fix32_to_int_round(v)
#else   /* !defined(FIX32_NO_ROUNDING) */
    #define FIX32_FROM_FLOAT(v) fix32_from_float(v)
    #define FIX32_FROM_DOUBLE(v) fix32_from_double(v)
    #define FIX32_TO_INT(v) fix32_to_int_trunc(v)
#endif  /* !defined(FIX32_NO_ROUNDING) */

#define FIX32_TO_FLOAT(v) fix32_to_float(v)
#define FIX32_TO_DOUBLE(v) fix32_to_double(v)

#define FIX32_ITRUNC(v) fix32_to_int_trunc(v)
#define FIX32_IFLOOR(v) fix32_to_int_floor(v)
#define FIX32_IROUND(v) fix32_to_int_round(v)

#endif  /* TOFU_LIBS_FPMATH_H */