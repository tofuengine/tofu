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

#ifndef TOFU_LIBS_GL_PALETTE_H
#define TOFU_LIBS_GL_PALETTE_H

#include "common.h"

#include <core/config.h>

#include <stdbool.h>
#include <stddef.h>

// FIXME: move to `common.h`
#define GL_MAX_PALETTE_COLORS       256

extern void GL_palette_set_greyscale(GL_Color_t *palette, size_t size);
extern void GL_palette_set_quantized(GL_Color_t *palette, size_t red_bits, size_t green_bits, size_t blue_bits);

extern GL_Pixel_t GL_palette_find_nearest_color(const GL_Color_t *palette, GL_Color_t color);

extern GL_Color_t GL_palette_mix(GL_Color_t from, GL_Color_t to, float ratio);

extern void GL_palette_copy(GL_Color_t *palette, const GL_Color_t *source);
extern size_t GL_palette_merge(GL_Color_t *palette, size_t to, const GL_Color_t *other, size_t from, size_t count, bool remove_duplicates);
extern void GL_palette_lerp(GL_Color_t *palette, GL_Color_t color, float ratio);
// TODO: add other functions, too...

#endif  /* TOFU_LIBS_GL_PALETTE_H */
