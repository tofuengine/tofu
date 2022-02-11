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

#ifndef __MODULES_UTILS_CALLBACKS_H__
#define __MODULES_UTILS_CALLBACKS_H__

#include <libs/gl/gl.h>

typedef struct Callback_Palette_Closure_s {
    const GL_Color_t *palette;
    GL_Pixel_t transparent;
    uint8_t threshold;
} Callback_Palette_Closure_t;

typedef struct Callback_Indexes_Closure_s {
    GL_Pixel_t background;
    GL_Pixel_t foreground;
} Callback_Indexes_Closure_t;

extern void surface_callback_palette(void *user_data, GL_Surface_t *surface, const void *pixels);
extern void surface_callback_indexes(void *user_data, GL_Surface_t *surface, const void *pixels);

#endif  /* __MODULES_UTILS_CALLBACKS_H__ */
