/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#ifndef __GL_PALETTE_H__
#define __GL_PALETTE_H__

#include <stddef.h>

#include "common.h"

#define GL_MAX_PALETTE_COLORS       256

typedef struct _GL_Palette_t {
    GL_Color_t colors[GL_MAX_PALETTE_COLORS];
    size_t count;
} GL_Palette_t;

extern void GL_palette_greyscale(GL_Palette_t *palette, size_t count);
extern GL_Color_t GL_palette_parse_color(const char *argb);
extern void GL_palette_format_color(char *argb, const GL_Color_t color);
extern void GL_palette_normalize(const GL_Palette_t *palette, GLfloat *colors);
extern void GL_palette_normalize_color(const GL_Color_t color, GLfloat rgba[4]);

#endif  /* __GL_PALETTE_H__ */