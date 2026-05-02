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

#ifndef TOFU_LIBS_GL_COMMON_H
#define TOFU_LIBS_GL_COMMON_H

#include <core/config.h>
#include <core/platform.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GL_BOOL_FALSE   ((GL_Bool_t)0)
#define GL_BOOL_TRUE    ((GL_Bool_t)1)

// FIXME: does this makes sense?
#define GL_CELL_NIL     ((GL_Cell_t)-1)

// In any case, we are using an 8-bit-per-pixel color representation, so we can
// have up to 256 colors in the palette. This is the single hard limit we are
// imposing on the palette,
#define GL_PALETTE_AVAILABLE_COLORS 256

// The palette originally supported up-to 256 entries. However, we lowered this
// limit to 128 colors to free the MSB which is now used an alpha-bit (with
// inverted value, when `1` the pixel is transparent).
// However the amount of slot in the LUT is, still, `2^8`. We use this "padding"
// 128 entries to tell if a pixel is transparent just with a single LUT access
// (and not by testing the alpha-bit).
#define GL_PALETTE_MAX_COLORS   128
#define GL_PALETTE_LAST_INDEX   (GL_PALETTE_MAX_COLORS - 1)

#define GL_COLOR_LAST_VALUE     255

// Palette banks need to be a power of two, as they are used as bit-shift
// values, and merged with the pixel color index, to determine the actual
// palette index to use during the transferring to the framebuffer.
#define GL_PALETTE_MAX_BANKS     2
#define GL_PALETTE_LAST_BANK     (GL_PALETTE_MAX_BANKS - 1)
#define GL_PALETTE_DEFAULT_BANK  0

// Bit #7 is used to encode the bank number, so we can have up to 128 colors
// per bank, for a total of 256.
#define GL_PALETTE_BANK_SHIFT    7

#define GL_PIXEL_TRANSPARENCY_MASK 0x80
#define GL_PIXEL_INDEX_MASK        0x7F

#define GL_PIXEL_IS_TRANSPARENT(pixel) ((pixel) & GL_PIXEL_TRANSPARENCY_MASK)
#define GL_PIXEL_GET_INDEX(pixel)      ((pixel) & GL_PIXEL_INDEX_MASK)

typedef struct GL_IO_Callbacks_s {
    size_t (*read)(void *user_data, void *buffer, size_t bytes_to_read);
    bool   (*seek)(void *user_data, long offset, int whence);
    long   (*tell)(void *user_data);
    bool   (*eof)(void *user_data);
} GL_IO_Callbacks_t;

typedef struct GL_IO_Callbacks_Closure_s {
    const GL_IO_Callbacks_t *callbacks;
    void *user_data;
} GL_IO_Callbacks_Closure_t;

// A pixel is a single byte (eight bits) encoding the transparency (bit #7)
// and the color index (bits #0-6) of a pixel. The actual color is determined
// by looking up the color index in a 128-entries palette.
typedef uint8_t GL_Pixel_t;

typedef int8_t GL_Bool_t;

typedef int GL_Cell_t;

typedef struct GL_Point_s {
    int x, y;
} GL_Point_t;

typedef struct GL_PointF_s {
    float x, y;
} GL_PointF_t;

typedef struct GL_Size_s {
    size_t width, height;
} GL_Size_t;

typedef struct GL_SizeF_s {
    float width, height;
} GL_SizeF_t;

typedef struct GL_Rectangle_s {
    int x, y;
    size_t width, height;
} GL_Rectangle_t;

typedef struct GL_RectangleF_s {
    float x, y;
    float width, height;
} GL_RectangleF_t;

typedef struct GL_Quad_s {
    int x0, y0; // FIXME: rename to left, top, right, and bottom.
    int x1, y1;
    // TODO: optimize by adding `width` and `height` fields?
} GL_Quad_t;

typedef struct GL_QuadF_s {
    float x0, y0;
    float x1, y1;
} GL_QuadF_t;

#if TOFU_GRAPHICS_PIXEL_FORMAT == PIXEL_FORMAT_RGBA8888
#pragma pack(push, 1)
typedef struct GL_Color_s {
#if PLATFORM_ID == PLATFORM_WINDOWS
    uint8_t b, g, r, a;
#else
    uint8_t r, g, b, a;
#endif
} GL_Color_t;
#pragma pack(pop)
#elif TOFU_GRAPHICS_PIXEL_FORMAT == PIXEL_FORMAT_RGB565
typedef uint16_t GL_Color_t;
#else
    #error "unsupported TOFU_GRAPHICS_PIXEL_FORMAT"
#endif

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

#endif  /* TOFU_LIBS_GL_COMMON_H */
