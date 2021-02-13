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

#include <stdbool.h>

#include "common.h"
#include "palette.h"

typedef struct _GL_Surface_t {
    size_t width, height;
    GL_Pixel_t *data;
    size_t data_size;
} GL_Surface_t;

typedef enum _copper_list_command_t {
    WAIT,
    PALETTE,
    MODULO
} copper_list_command_t;

typedef struct _copper_list_entry_t {
    copper_list_command_t command;
    union {
        uint32_t u32;
        uint16_t u16;
        uint8_t u8;
        int32_t i32;
        int16_t i16;
        int8_t i8;
        unsigned int u;
        int i;
        float f;
        double d;
    } args[2];
} copper_list_entry_t;

typedef void (*GL_Surface_Callback_t)(void *user_data, GL_Surface_t *surface, const void *pixels); // RGBA888 format.

extern GL_Surface_t *GL_surface_decode(size_t width, size_t height, const void *pixels, const GL_Surface_Callback_t callback, void *user_data);
extern GL_Surface_t *GL_surface_create(size_t width, size_t height);
extern void GL_surface_destroy(GL_Surface_t *surface);

extern void GL_surface_to_rgba(const GL_Surface_t *context, const GL_Palette_t *palette, GL_Color_t *vram);

extern void GL_surface_to_rgba_run(const GL_Surface_t *surface, const GL_Palette_t *palette, const copper_list_entry_t *copper_list, size_t copper_list_entries, GL_Color_t *vram);

#endif  /* __GL_SURFACE_H__ */
