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

#include <stddef.h>

#include "common.h"

#define GL_MAX_PALETTE_COLORS       256

typedef struct _GL_Palette_t {
    GL_Color_t colors[GL_MAX_PALETTE_COLORS];
    size_t size;
} GL_Palette_t;

extern void GL_palette_generate_greyscale(GL_Palette_t *palette, size_t size);
extern void GL_palette_generate_quantized(GL_Palette_t *palette, const size_t red_bits, const size_t green_bits, const size_t blue_bits);
extern GL_Pixel_t GL_palette_find_nearest_color(const GL_Palette_t *palette, const GL_Color_t color);
extern GL_Color_t GL_palette_lerp(const GL_Color_t from, const GL_Color_t to, float ratio);

#endif  /* __GL_PALETTE_H__ */
