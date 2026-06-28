/*
 * MIT License
 * 
 * Copyright (c) 2026 Marco Lizza
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

#ifndef FIX32_H
#define FIX32_H

#define FIX32_H_INCLUDED

#define FIX32_STRINGIFY(x)     #x
#define FIX32_XSTRINGIFY(x)    FIX32_STRINGIFY(x)

#define FIX32_VERSION_MAJOR    0
#define FIX32_VERSION_MINOR    3
#define FIX32_VERSION_REVISION 0
#define FIX32_VERSION_STRING   FIX32_XSTRINGIFY(FIX32_VERSION_MAJOR) "." FIX32_XSTRINGIFY(FIX32_VERSION_MINOR) "." FIX32_XSTRINGIFY(FIX32_VERSION_REVISION)

#include <stdint.h>

// The library refers to the actual type as `fix32_t` as internally it is
// just an integer.
//
// If not specified otherwise, the fixed-point numbers are represented as 16.16
// (i.e. 16 bits for the integer part and 16 bits for the fractional part).
//
// In addition to `float`, also `double` is supported. This is due to the fact
// that IEEE-754 single-precision floats have only 23 bits of precision, which
// is not enough to represent all the values of a 16.16 fixed-point number
// without loss of precision. On the other hand, IEEE-754 double-precision
// floats have 52 bits of precision, which is more than enough to represent all
// the values of a 16.16 fixed-point number.

// Public API overview:
//
// Include-time macros and versioning:
// - `FIX32_H` / `FIX32_H_INCLUDED`: internal include guards.
// - `FIX32_STRINGIFY(x)` / `FIX32_XSTRINGIFY(x)`: stringification helpers
//   used to build `FIX32_VERSION_STRING`.
// - `FIX32_VERSION_MAJOR`, `FIX32_VERSION_MINOR`, `FIX32_VERSION_REVISION`,
//   `FIX32_VERSION_STRING`: library version identifiers.
//
// Configuration macros defined before including this header:
// - `FIX32_IMPLEMENTATION`: emits the external function definitions from this
//   header. Define it in exactly one translation unit, after choosing the same
//   configuration macros used by the rest of the program.
// - `FIX32_STATIC_INLINE`: emits `static inline` definitions in the current
//   translation unit instead of external declarations. This enables an
//   all-inline behavior for performance-sensitive code.
// - `FIX32_INLINE`: overrides the inline spelling used by `FIX32_STATIC_INLINE`.
// - `FIX32_FRACTIONAL_BITS`: number of fractional bits in the fixed-point
//   representation, from `1` to `30` (default to `16`).
// - `FIX32_USE_ARITHMETIC_SHIFT_FLOOR`: selects the arithmetic-shift floor
//   fast path (`1`, default) or the portable division/remainder fallback (`0`).
// - `FIX32_USE_SIGNED_SHIFT_MUL`: selects signed right-shift scaling for
//   `fix32_mul()` (`1`) or the truncating divide-based scaling (`0`, default).
// - `FIX32_USE_SIGNED_SHIFT_DIV`: selects signed left-shift scaling for
//   `fix32_div()` (`1`) or the multiply-by-scale path (`0`, default).
// - `FIX32_USE_64_BIT`: selects the wider internal multiply path for helpers
//   such as `fix32_mul_by_int()` and `fix32_round_to_int()`. If omitted, the
//   header derives it from `FIX32_INTEGER_BITS` when present, otherwise it
//   defaults to `1`.
// - `FIX32_INTEGER_BITS`: optional compile-time hint for `FIX32_USE_64_BIT`
//   auto-selection. With the default `FIX32_FRACTIONAL_BITS` of `16`, the
//   integer part has 15 bits of precision. If the integer part has more than
//   15 bits, `FIX32_USE_64_BIT` will be automatically selected.
//
// Constants and type:
// - `FIX32_ONE`: raw fixed-point scale factor.
// - `FIX32_HALF`: half of `FIX32_ONE`.
// - `FIX32_FRACTIONAL_MASK`: mask for the fractional bits.
// - `FIX32_INTEGER_MASK`: mask for the integer bits.
// - `FIX32_INT_MAX` / `FIX32_INT_MIN`: whole-number range that fits in the raw
//   storage when scaled by `FIX32_ONE`.
// - `fix32_t`: signed 32-bit storage type used by the library.
//
// Functions:
// - `fix32_from_raw()` / `fix32_to_raw()`: identity conversions between raw
//   storage and `fix32_t`.
// - `fix32_from_int()`: converts an integer to fixed-point by scaling it.
// - `fix32_from_float()` / `fix32_from_double()`: truncating float/double to
//   fixed-point conversion.
// - `fix32_round_from_float()` / `fix32_round_from_double()`: half-away-from-
//   zero float/double to fixed-point conversion.
// - `fix32_from_rational()`: converts an integer numerator and denominator
//   directly to fixed point with truncation toward zero. This is mainly a
//   semantic helper for deterministic integer-ratio conversion, as it avoids an
//   intermediate floating-point value. The denominator must be nonzero and the
//   scaled intermediate must fit the selected arithmetic path.
// - `fix32_add()` / `fix32_sub()`: fixed-point addition and subtraction.
// - `fix32_mul_by_int()`: fixed-point multiplied by an integer.
// - `fix32_mul()`: fixed-point multiplication with selectable scaling path.
// - `fix32_div_by_int()`: fixed-point divided by an integer.
// - `fix32_div()`: fixed-point division with selectable scaling path.
// - `fix32_reciprocal_by_int()`: reciprocal of an integer in fixed-point.
// - `fix32_reciprocal()`: reciprocal of a fixed-point value.
// - `fix32_floor_to_int()` / `fix32_ceil_to_int()` / `fix32_trunc_to_int()` /
//   `fix32_round_to_int()`: convert fixed-point to integer using the named
//   rounding policy.
// - `fix32_to_float()` / `fix32_to_double()`: exact value conversion back to
//   floating point.
// - `fix32_floor_to_float()` / `fix32_ceil_to_float()` /
//   `fix32_round_to_float()`: fixed-point to float using integer-style rounding
//   first.
// - `fix32_floor_to_double()` / `fix32_ceil_to_double()` /
//   `fix32_round_to_double()`: fixed-point to double using integer-style
//   rounding first.
// - `fix32_floor()` / `fix32_ceil()` / `fix32_round()`: rounding while keeping
//   the result in fixed-point representation.

#if !defined(FIX32_FRACTIONAL_BITS)
    #define FIX32_FRACTIONAL_BITS 16
#endif  /* !defined(FIX32_FRACTIONAL_BITS) */

#if (FIX32_FRACTIONAL_BITS < 1) || (FIX32_FRACTIONAL_BITS > 30)
#error "FIX32_FRACTIONAL_BITS must be between 1 and 30"
#endif

// Retain signedness with these macros, for the masks.
#define FIX32_ONE             (INT32_C(1) << FIX32_FRACTIONAL_BITS)
#define FIX32_HALF            (FIX32_ONE / 2)
#define FIX32_FRACTIONAL_MASK (FIX32_ONE - INT32_C(1))
#define FIX32_INTEGER_MASK    (~FIX32_FRACTIONAL_MASK)
#define FIX32_INT_MAX         (INT32_MAX / FIX32_ONE)
#define FIX32_INT_MIN         (INT32_MIN / FIX32_ONE)

// Set to `0` before including this header to use the portable floor-to-int path.
// The default uses an arithmetic right shift for speed, which is
// implementation-defined for negative signed integers in C99.
#if !defined(FIX32_USE_ARITHMETIC_SHIFT_FLOOR)
    #define FIX32_USE_ARITHMETIC_SHIFT_FLOOR 1
#endif  /* !defined(FIX32_USE_ARITHMETIC_SHIFT_FLOOR) */

// Set to 1 before including this header to scale multiplication results with a
// signed right shift. This is implementation-defined when the result is
// negative. The default multiplication path is defined for every `fix32_t`
// value and truncates toward zero instead of relying on shift semantics.
#if !defined(FIX32_USE_SIGNED_SHIFT_MUL)
    #define FIX32_USE_SIGNED_SHIFT_MUL 0
#endif  /* !defined(FIX32_USE_SIGNED_SHIFT_MUL) */

// Set to 1 before including this header to scale division numerators with a
// signed left shift. This invokes undefined behavior when the numerator is
// negative. The default division path is defined for every `fix32_t`
// value and is typically optimized to the same machine instruction.
#if !defined(FIX32_USE_SIGNED_SHIFT_DIV)
    #define FIX32_USE_SIGNED_SHIFT_DIV 0
#endif  /* !defined(FIX32_USE_SIGNED_SHIFT_DIV) */

// Optional hint: define the maximum magnitude bits expected to the left of the
// radix point in operands passed to `fix32_mul()`. This does not change the
// stored fixed-point format; it only describes the operand range the
// programmer promises to use for auto-selecting the path.
//
// If this hint is not provided, the header falls back to the safe 64-bit
// path by default.
#if !defined(FIX32_USE_64_BIT)
    #if defined(FIX32_INTEGER_BITS)
        #define FIX32_USE_64_BIT (((FIX32_INTEGER_BITS + FIX32_FRACTIONAL_BITS) > 15) ? 1 : 0)
    #else   /* defined(FIX32_INTEGER_BITS) */
        #define FIX32_USE_64_BIT 1
    #endif  /* defined(FIX32_INTEGER_BITS) */
#endif  /* !defined(FIX32_USE_64_BIT) */

// Using 32-bit integers is a reasonable default for our use case, since we
// won't be handling very large numbers, and it allows for better performances
// due to the lower memory bandwidth and better cache usage.
typedef int32_t fix32_t;

#if defined(FIX32_IMPLEMENTATION) && defined(FIX32_STATIC_INLINE)
#error "Define only one of FIX32_IMPLEMENTATION and FIX32_STATIC_INLINE"
#endif  /* defined(FIX32_IMPLEMENTATION) && defined(FIX32_STATIC_INLINE) */

#if !defined(FIX32_IMPLEMENTATION) && !defined(FIX32_STATIC_INLINE)

#ifdef __cplusplus
extern "C" {
#endif

extern fix32_t fix32_from_raw(int32_t raw_value);
extern int32_t fix32_to_raw(fix32_t value);
extern fix32_t fix32_from_int(int32_t value);
extern fix32_t fix32_from_float(float value);
extern fix32_t fix32_round_from_float(float value);
extern fix32_t fix32_from_double(double value);
extern fix32_t fix32_round_from_double(double value);
extern fix32_t fix32_from_rational(int32_t numerator, int32_t denominator);
extern fix32_t fix32_add(fix32_t left, fix32_t right);
extern fix32_t fix32_sub(fix32_t left, fix32_t right);
extern fix32_t fix32_mul_by_int(fix32_t left, int right);
extern fix32_t fix32_mul(fix32_t left, fix32_t right);
extern fix32_t fix32_div_by_int(fix32_t numerator, int32_t denominator);
extern fix32_t fix32_div(fix32_t numerator, fix32_t denominator);
extern fix32_t fix32_reciprocal_by_int(int32_t value);
extern fix32_t fix32_reciprocal(fix32_t value);
extern int32_t fix32_floor_to_int(fix32_t value);
extern int32_t fix32_ceil_to_int(fix32_t value);
extern int fix32_trunc_to_int(fix32_t value);
extern int32_t fix32_round_to_int(fix32_t value);
extern float fix32_to_float(fix32_t value);
extern float fix32_floor_to_float(fix32_t value);
extern float fix32_ceil_to_float(fix32_t value);
extern float fix32_round_to_float(fix32_t value);
extern double fix32_to_double(fix32_t value);
extern double fix32_floor_to_double(fix32_t value);
extern double fix32_ceil_to_double(fix32_t value);
extern double fix32_round_to_double(fix32_t value);
extern fix32_t fix32_floor(fix32_t value);
extern fix32_t fix32_ceil(fix32_t value);
extern fix32_t fix32_round(fix32_t value);

#ifdef __cplusplus
}
#endif

#endif  /* !defined(FIX32_STATIC_INLINE) && !defined(FIX32_IMPLEMENTATION) */

#if defined(FIX32_STATIC_INLINE) || defined(FIX32_IMPLEMENTATION)

// If no explicit definition is provided, fallback to `static inline`.
#if !defined(FIX32_INLINE)
    #define FIX32_INLINE static inline
#endif  /* !defined(FIX32_INLINE) */

#if defined(FIX32_STATIC_INLINE)
    #if !defined(FIX32_INLINE)
        #define FIX32_DEF static inline
    #else   /* !defined(FIX32_INLINE) */
        #define FIX32_DEF FIX32_INLINE
    #endif  /* !defined(FIX32_INLINE) */
#else   /* defined(FIX32_STATIC_INLINE) */
    #define FIX32_DEF
#endif  /* defined(FIX32_STATIC_INLINE) */

#if defined(__cplusplus) && defined(FIX32_IMPLEMENTATION)
extern "C" {
#endif

FIX32_DEF fix32_t fix32_from_raw(int32_t raw_value)
{
    return raw_value;
}

FIX32_DEF int32_t fix32_to_raw(fix32_t value)
{
    return value;
}

// No check are performed on the input values, as the user is expected to know
// what they are doing when using fixed-point arithmetic and not cause undefined
// behavior by overflowing the intermediate calculations.
FIX32_DEF fix32_t fix32_from_int(int32_t value)
{
#if FIX32_USE_64_BIT
    return value * (int64_t)FIX32_ONE;
#else   /* FIX32_USE_64_BIT */
    return value * (int64_t)FIX32_ONE;
#endif  /* FIX32_USE_64_BIT */
}

FIX32_DEF fix32_t fix32_from_float(float value)
{
    return value * (float)FIX32_ONE;
}

FIX32_DEF fix32_t fix32_round_from_float(float value)
{
    const float scaled = value * (float)FIX32_ONE;
    return (fix32_t)(scaled + ((scaled >= 0.0f) ? 0.5f : -0.5f));
}

FIX32_DEF fix32_t fix32_from_double(double value)
{
    return value * (double)FIX32_ONE;
}

FIX32_DEF fix32_t fix32_round_from_double(double value)
{
    const double scaled = value * (double)FIX32_ONE;
    return (fix32_t)(scaled + ((scaled >= 0.0) ? 0.5 : -0.5));
}

FIX32_DEF fix32_t fix32_from_rational(int32_t numerator, int32_t denominator)
{
#if FIX32_USE_64_BIT
    return (fix32_t)(((int64_t)numerator * (int64_t)FIX32_ONE) /
                     (int64_t)denominator);
#else   /* FIX32_USE_64_BIT */
    return (fix32_t)((numerator * FIX32_ONE) / denominator);
#endif  /* FIX32_USE_64_BIT */
}

FIX32_DEF fix32_t fix32_add(fix32_t left, fix32_t right)
{
    return left + right;
}

FIX32_DEF fix32_t fix32_sub(fix32_t left, fix32_t right)
{
    return left - right;
}

FIX32_DEF fix32_t fix32_mul_by_int(fix32_t left, int right)
{
#if FIX32_USE_64_BIT
    return (fix32_t)((int64_t)left * (int64_t)right);
#else   /* FIX32_USE_64_BIT */
    return (fix32_t)(left * right);
#endif  /* FIX32_USE_64_BIT */
}

FIX32_DEF fix32_t fix32_mul(fix32_t left, fix32_t right)
{
#if FIX32_USE_64_BIT
    int64_t product = (int64_t)left * (int64_t)right;
#else   /* FIX32_USE_64_BIT */
    int32_t product = left * right;
#endif  /* FIX32_USE_64_BIT */
#if FIX32_USE_SIGNED_SHIFT_MUL
    return (fix32_t)(product >> FIX32_FRACTIONAL_BITS);
#else   /* FIX32_USE_SIGNED_SHIFT_MUL */
    return (fix32_t)(product / FIX32_ONE);
#endif  /* FIX32_USE_SIGNED_SHIFT_MUL */
}

FIX32_DEF fix32_t fix32_div_by_int(fix32_t numerator, int32_t denominator)
{
    return numerator / denominator;
}

FIX32_DEF fix32_t fix32_div(fix32_t numerator, fix32_t denominator)
{
    // Direct division of fixed-point numbers cancels out the fractional bits,
    // so we need to rescale the numerator first in order not to loose
    // precision.
    //
    //   S = 2^FRACTIONAL_BITS
    //   N = n * S (numerator in fixed-point)
    //   D = d * S (denominator in fixed-point)
    //
    //   N / D = (n * S) / (d * S) = n / d (precision loss due to cancellation of S)
    //
    //   (N * S) / D = (n * S * S) / (d * S) = n * S / d = (n / d) * S (correct scaling, no precision loss)
    //
#if FIX32_USE_SIGNED_SHIFT_DIV
    const int64_t scaled_numerator = (int64_t)numerator << FIX32_FRACTIONAL_BITS;
#else   /* FIX32_USE_SIGNED_SHIFT_DIV */
    const int64_t scaled_numerator = (int64_t)numerator * (int64_t)FIX32_ONE;
#endif  /* FIX32_USE_SIGNED_SHIFT_DIV */
    const int64_t quotient = scaled_numerator / (int64_t)denominator;
    // We could round here, for better precision, but it would be more
    // expensive, and probably not that worth.
    return (fix32_t)quotient;
}

FIX32_DEF fix32_t fix32_reciprocal_by_int(int32_t value)
{
    return FIX32_ONE / value;
}

FIX32_DEF fix32_t fix32_reciprocal(fix32_t value)
{
    return fix32_div(FIX32_ONE, value);
}

// Floors toward negative infinity, which is what we want for negative numbers.
//
// It's an implementation-defined behavior if `value` is negative and has a
// fractional part, but it works as expected on two's complement
// architectures, which are the vast majority of modern ones.
//
// Note: we are using bitwise shifting only to calculate the floor value.
FIX32_DEF int32_t fix32_floor_to_int(fix32_t value)
{
#if FIX32_USE_ARITHMETIC_SHIFT_FLOOR
    // Fast path: relies on the target doing arithmetic right shifts for signed values.
    return value >> FIX32_FRACTIONAL_BITS;
#else
    const int32_t whole = value / FIX32_ONE;
    const int32_t fractional = value % FIX32_ONE;

    if (fractional != 0 && value < 0) {
        return whole - 1;
    }

    return whole;
#endif
}

FIX32_DEF int32_t fix32_ceil_to_int(fix32_t value)
{
#if FIX32_USE_ARITHMETIC_SHIFT_FLOOR
    const int32_t whole = value >> FIX32_FRACTIONAL_BITS;

    // The fractional bits live in the low raw bits because the scale is a power of two.
    return whole + ((value & FIX32_FRACTIONAL_MASK) != 0);
#else
    const int32_t whole = value / FIX32_ONE;
    const int32_t fractional = value % FIX32_ONE;

    if (fractional != 0 && value > 0) {
        return whole + 1;
    }

    return whole;
#endif
}

FIX32_DEF int fix32_trunc_to_int(fix32_t value)
{
    return value / FIX32_ONE; // This will truncate toward zero, which is what we want for negative numbers.
}

FIX32_DEF int32_t fix32_round_to_int(fix32_t value)
{
#if FIX32_USE_64_BIT
    int64_t aux = (int64_t)value; // Use `int64_t` to avoid overflow when adding `FIX32_ONE_HALF`.
#else   /* FIX32_USE_64_BIT */
    int32_t aux = value;
#endif  /* FIX32_USE_64_BIT */
    aux += (aux >= 0) ? FIX32_HALF : -FIX32_HALF;
    return (int32_t)(aux / FIX32_ONE); // Dividing (not shifting) will round to the nearest integer, with ties rounding away from zero, which is what we want for negative numbers.
}

FIX32_DEF float fix32_to_float(fix32_t value)
{
    return (float)value / (float)FIX32_ONE; // No need to add `FIX32_ONE_HALF` for rounding, as the division will already round to the nearest float.
}

FIX32_DEF float fix32_floor_to_float(fix32_t value)
{
    return (float)fix32_floor_to_int(value);
}

FIX32_DEF float fix32_ceil_to_float(fix32_t value)
{
    return (float)fix32_ceil_to_int(value);
}

FIX32_DEF float fix32_round_to_float(fix32_t value)
{
    return (float)fix32_round_to_int(value);
}

FIX32_DEF double fix32_to_double(fix32_t value)
{
    return (double)value / (double)FIX32_ONE; // Ditto.
}

FIX32_DEF double fix32_floor_to_double(fix32_t value)
{
    return (double)fix32_floor_to_int(value);
}

FIX32_DEF double fix32_ceil_to_double(fix32_t value)
{
    return (double)fix32_ceil_to_int(value);
}

FIX32_DEF double fix32_round_to_double(fix32_t value)
{
    return (double)fix32_round_to_int(value);
}

FIX32_DEF fix32_t fix32_floor(fix32_t value)
{
    return value & FIX32_INTEGER_MASK;
}

FIX32_DEF fix32_t fix32_ceil(fix32_t value)
{
    // The masking already produces a "floor" value, so we don't need to check
    // for the sign of `value` to decide if we need to add `FIX32_ONE` or not, as
    // the result will be the same.
    return (value & FIX32_INTEGER_MASK) + ((value & FIX32_FRACTIONAL_MASK) ? FIX32_ONE : 0);
}

FIX32_DEF fix32_t fix32_round(fix32_t value)
{
#if FIX32_USE_64_BIT
    int64_t biased = (int64_t)value;
#else   /* FIX32_USE_64_BIT */
    int32_t biased = value;
#endif  /* FIX32_USE_64_BIT */
    biased += (value >= 0) ? FIX32_HALF : (FIX32_HALF - INT32_C(1));
    return (fix32_t)(biased & FIX32_INTEGER_MASK);
}

#if defined(__cplusplus) && defined(FIX32_IMPLEMENTATION)
}
#endif

#undef FIX32_DEF
#endif  /* !defined(FIX32_IMPLEMENTATION) && !defined(FIX32_STATIC_INLINE) */

#endif  /* FIX32_H */
