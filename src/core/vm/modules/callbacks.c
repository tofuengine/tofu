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

#pragma pack(push, 1)
typedef struct rgba_t {
    uint8_t r, g, b, a;
} rgba_t;
#pragma pack(pop)

void surface_callback_palette(void *parameters, GL_Surface_t *surface, const void *data)
{
    const GL_Palette_t *palette = (const GL_Palette_t *)parameters;

    const rgba_t *src = (const rgba_t *)data;
    GL_Pixel_t *dst = surface->data;

    for (size_t i = surface->data_size; i; --i) {
        rgba_t rgba = *(src++);
        GL_Color_t color = (GL_Color_t){ .r = rgba.r, .g = rgba.g, .b = rgba.b, .a = rgba.a };
        *(dst++) = GL_palette_find_nearest_color(palette, color);
    }
}

void surface_callback_indexes(void *parameters, GL_Surface_t *surface, const void *data)
{
    const GL_Pixel_t *indexes = (const GL_Pixel_t *)parameters;
    const GL_Pixel_t bg_index = indexes[0];
    const GL_Pixel_t fg_index = indexes[1];

    const uint32_t *src = (const uint32_t *)data; // Faster than the `rgba_t` struct, we don't need to unpack components.
    GL_Pixel_t *dst = surface->data;

    const uint32_t background = *src; // The top-left pixel color defines the background.

    for (size_t i = surface->data_size; i; --i) {
        uint32_t rgba = *(src++);
        *(dst++) = rgba == background ? bg_index : fg_index;
    }
}
