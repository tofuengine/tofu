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

#include "callbacks.h"

#include <memory.h>

void to_indexed_atlas_callback(void *parameters, GL_Surface_t *surface, const void *data)
{
    const GL_Palette_t *palette = (const GL_Palette_t *)parameters;

    const GL_Color_t *src = (const GL_Color_t *)data;
    GL_Pixel_t *dst = surface->data;

    for (size_t i = surface->data_size; i; --i) {
        GL_Color_t color = *(src++);
        *(dst++) = GL_palette_find_nearest_color(palette, color);
    }
}

void to_font_atlas_callback(void *parameters, GL_Surface_t *surface, const void *data)
{
    const GL_Pixel_t *indexes = (const GL_Pixel_t *)parameters;
    const GL_Pixel_t bg_index = indexes[0];
    const GL_Pixel_t fg_index = indexes[1];

    const GL_Color_t *src = (const GL_Color_t *)data;
    GL_Pixel_t *dst = surface->data;

    const GL_Color_t background = *src; // The top-left pixel color defines the background.

    for (size_t i = surface->data_size; i; --i) {
        GL_Color_t color = *(src++);
        *(dst++) = memcmp(&color, &background, sizeof(GL_Color_t)) == 0 ? bg_index : fg_index;
    }
}
