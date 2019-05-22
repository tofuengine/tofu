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

#include "palette.h"

#include <stdlib.h>
#include <string.h>

void GL_palette_greyscale(GL_Palette_t *palette, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        unsigned char v = (unsigned char)(((double)i / (double)(count - 1)) * 255.0);
        palette->colors[i] = (GL_Color_t){ v, v, v, 255 };
    }
    palette->count = count;
}

GL_Color_t GL_palette_parse_color(const char *argb)
{
    GL_Color_t color;
    char hex[3] = {};
    strncpy(hex, argb    , 2); color.a = strtol(hex, NULL, 16);
    strncpy(hex, argb + 2, 2); color.r = strtol(hex, NULL, 16);
    strncpy(hex, argb + 4, 2); color.g = strtol(hex, NULL, 16);
    strncpy(hex, argb + 6, 2); color.b = strtol(hex, NULL, 16);
    return color;
}

void GL_palette_normalize(const GL_Palette_t *palette, GLfloat *colors) // palette->count * 3
{
    for (size_t i = 0; i < palette->count; ++i) {
        int j = i * 3;
        colors[j    ] = (GLfloat)palette->colors[i].r / (GLfloat)255.0;
        colors[j + 1] = (GLfloat)palette->colors[i].g / (GLfloat)255.0;
        colors[j + 2] = (GLfloat)palette->colors[i].b / (GLfloat)255.0;
    }
}