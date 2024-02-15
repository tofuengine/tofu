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
 * Copyright (c) 2019-2024 Marco Lizza
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

#ifndef TOFU_LIBS_GL_COMMON_H
#define TOFU_LIBS_GL_COMMON_H

#include <core/platform.h>

#include <stddef.h>
#include <stdint.h>

typedef uint8_t GL_Pixel_t;

typedef int8_t GL_Bool_t;

typedef int GL_Cell_t;

typedef struct GL_Point_s {
    int x, y;
} GL_Point_t;

typedef struct GL_Size_s {
    size_t width, height;
} GL_Size_t;

typedef struct GL_Rectangle_s {
    int x, y;
    size_t width, height;
} GL_Rectangle_t;

typedef struct GL_Quad_s {
    int x0, y0; // FIXME: rename to left, top, right, and bottom.
    int x1, y1;
} GL_Quad_t;

#pragma pack(push, 1)
typedef struct GL_Color_s {
#if PLATFORM_ID == PLATFORM_WINDOWS
    uint8_t b, g, r, a;
#else
    uint8_t r, g, b, a;
#endif
} GL_Color_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct GL_Rectangle32_s {
    int32_t x, y;
    uint32_t width, height;
} GL_Rectangle32_t;
#pragma pack(pop)

typedef enum GL_Comparators_e {
    GL_COMPARATOR_NEVER,
    GL_COMPARATOR_LESS,
    GL_COMPARATOR_LESS_OR_EQUAL,
    GL_COMPARATOR_GREATER,
    GL_COMPARATOR_GREATER_OR_EQUAL,
    GL_COMPARATOR_EQUAL,
    GL_COMPARATOR_NOT_EQUAL,
    GL_COMPARATOR_ALWAYS,
    GL_Comparators_t_CountOf
} GL_Comparators_t;

typedef enum GL_Functions_e {
    GL_FUNCTIONS_REPLACE,
    GL_FUNCTIONS_ADD,
    GL_FUNCTIONS_ADD_CLAMPED,
    GL_FUNCTIONS_SUBTRACT,
    GL_FUNCTIONS_SUBTRACT_CLAMPED,
    GL_FUNCTIONS_REVERSE_SUBTRACT,
    GL_FUNCTIONS_REVERSE_SUBTRACT_CLAMPED,
    GL_FUNCTIONS_MULTIPLY,
    GL_FUNCTIONS_MULTIPLY_CLAMPED,
    GL_FUNCTIONS_MIN,
    GL_FUNCTIONS_MAX,
    GL_Functions_t_CountOf
} GL_Functions_t;

#define GL_BOOL_FALSE   ((GL_Bool_t)0)
#define GL_BOOL_TRUE    ((GL_Bool_t)1)

// FIXME: does this makes sense?
#define GL_CELL_NIL     ((GL_Cell_t)-1)

#define GL_MAX_PALETTE_COLORS   256

#endif  /* TOFU_LIBS_GL_COMMON_H */
