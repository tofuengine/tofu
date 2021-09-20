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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __GL_PALETTE_H__
#define __GL_PALETTE_H__

#include <config.h>

#include <stdbool.h>
#include <stddef.h>

#include "common.h"

#define GL_MAX_PALETTE_COLORS       256

#ifdef __PALETTE_COLOR_MEMOIZATION__
typedef struct color_pixel_pair_s {
    GL_Color_t key;
    GL_Pixel_t value;
} color_pixel_pair_t;
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */

typedef struct GL_Palette_s {
    GL_Color_t colors[GL_MAX_PALETTE_COLORS];
    size_t size;
#ifdef __PALETTE_COLOR_MEMOIZATION__
    color_pixel_pair_t *cache; // Stores past executed colors matches.
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */
} GL_Palette_t;

extern GL_Palette_t *GL_palette_create(void);
extern void GL_palette_destroy(GL_Palette_t *palette);

extern void GL_palette_set_colors(GL_Palette_t *palette, const GL_Color_t *colors, size_t size);
extern void GL_palette_set_greyscale(GL_Palette_t *palette, size_t size);
extern void GL_palette_set_quantized(GL_Palette_t *palette, const size_t red_bits, const size_t green_bits, const size_t blue_bits);

extern GL_Color_t GL_palette_get(const GL_Palette_t *palette, GL_Pixel_t index);
extern void GL_palette_set(GL_Palette_t *palette, GL_Pixel_t index, GL_Color_t color);

#ifdef __PALETTE_COLOR_MEMOIZATION__
extern GL_Pixel_t GL_palette_find_nearest_color(GL_Palette_t *palette, const GL_Color_t color);
#else
extern GL_Pixel_t GL_palette_find_nearest_color(const GL_Palette_t *palette, const GL_Color_t color);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */

extern GL_Color_t GL_palette_mix(const GL_Color_t from, const GL_Color_t to, float ratio);

extern void GL_palette_copy(GL_Palette_t *palette, const GL_Palette_t *source);
extern void GL_palette_merge(GL_Palette_t *palette, const GL_Palette_t *other, bool remove_duplicates);
extern void GL_palette_lerp(GL_Palette_t *palette, const GL_Color_t color, float ratio);
// TODO: add other functions, too...

#endif  /* __GL_PALETTE_H__ */
