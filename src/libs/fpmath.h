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

#include "fix32.h"

// Helper macros. The user can still call the functions directly if they want
// to, but these macros provide a more convenient interface. Also, they allow
// to switch between rounding and non-rounding versions of the functions by
// defining the `FPMATH_NO_ROUNDING` macro.
#define FPMATH_FROM_INT(v) fix32_from_int(v)
#define FPMATH_FROM_RATIONAL(n, d) fix32_from_rational((n), (d))
#if !defined(FPMATH_NO_ROUNDING)
    #define FPMATH_FROM_FLOAT(v) fix32_round_from_float(v)
    #define FPMATH_FROM_DOUBLE(v) fix32_round_from_double(v)
    #define FPMATH_TO_INT(v) fix32_round_to_int(v)
#else   /* !defined(FPMATH_NO_ROUNDING) */
    #define FPMATH_FROM_FLOAT(v) fix32_from_float(v)
    #define FPMATH_FROM_DOUBLE(v) fix32_from_double(v)
    #define FPMATH_TO_INT(v) fix32_trunc_to_int(v)
#endif  /* !defined(FPMATH_NO_ROUNDING) */
#define FPMATH_TO_FLOAT(v) fix32_to_float(v)
#define FPMATH_TO_DOUBLE(v) fix32_to_double(v)

#define FPMATH_ITRUNC(v) fix32_trunc_to_int(v)
#define FPMATH_IFLOOR(v) fix32_floor_to_int(v)
#define FPMATH_IROUND(v) fix32_round_to_int(v)

#endif  /* TOFU_LIBS_FPMATH_H */