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

#ifndef __GL_SURFACE_H__
#define __GL_SURFACE_H__

#include "common.h"

#include <stdbool.h>

typedef struct GL_Surface_s {
    size_t width, height; // FIXME: use `GL_Size_t`.
    GL_Pixel_t *data;
    size_t data_size;
    bool is_power_of_two;
} GL_Surface_t;

typedef void (*GL_Surface_Callback_t)(void *user_data, GL_Surface_t *surface, const void *pixels); // RGBA888 format.

// TODO: rename decode to convert/grab.
// FIXME: change width-height to `GL_Size_t`.
extern GL_Surface_t *GL_surface_decode(size_t width, size_t height, const void *pixels, GL_Surface_Callback_t callback, void *user_data);
extern GL_Surface_t *GL_surface_create(size_t width, size_t height);
extern void GL_surface_destroy(GL_Surface_t *surface);

extern void GL_surface_clear(const GL_Surface_t *surface, GL_Pixel_t index);
extern GL_Pixel_t GL_surface_peek(const GL_Surface_t *surface, GL_Point_t position);
extern void GL_surface_poke(const GL_Surface_t *surface, GL_Point_t position, GL_Pixel_t index);

#endif  /* __GL_SURFACE_H__ */
