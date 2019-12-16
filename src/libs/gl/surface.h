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

#ifndef __GL_SURFACE_H__
#define __GL_SURFACE_H__

#include <stdbool.h>

#include "common.h"
#include "palette.h"

typedef struct _GL_Surface_t {
    size_t width, height;
    GL_Pixel_t *data;
    size_t data_size;
} GL_Surface_t;

typedef void (*GL_Surface_Callback_t)(void *parameters, GL_Surface_t *surface, const void *data);

extern bool GL_surface_decode(GL_Surface_t *surface, const void *buffer, size_t buffer_size, const GL_Surface_Callback_t callback, void *parameters);
extern bool GL_surface_fetch(GL_Surface_t *surface, GL_Image_t image, const GL_Surface_Callback_t callback, void *parameters);
extern bool GL_surface_create(GL_Surface_t *surface, size_t width, size_t height);
extern void GL_surface_delete(GL_Surface_t *surface);

extern void GL_surface_to_rgba(const GL_Surface_t *context, const GL_Palette_t *palette, GL_Color_t *vram);

#endif  /* __GL_SURFACE_H__ */