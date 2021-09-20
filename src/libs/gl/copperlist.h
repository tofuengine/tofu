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

#ifndef __GL_COPPERLIST_H__
#define __GL_COPPERLIST_H__

#include "common.h"
#include "palette.h"
#include "program.h"
#include "surface.h"

typedef struct GL_Copperlist_s { // FIXME: rename to something better!!!
    GL_Palette_t *palette;
    GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS];
    GL_Program_t *program;
} GL_Copperlist_t;

extern GL_Copperlist_t *GL_copperlist_create(void);
extern void GL_copperlist_destroy(GL_Copperlist_t *copperlist);

extern void GL_copperlist_reset(GL_Copperlist_t *copperlist);

extern void GL_copperlist_set_palette(GL_Copperlist_t *copperlist, const GL_Palette_t *palette);
extern void GL_copperlist_set_shifting(GL_Copperlist_t *copperlist, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count);
extern void GL_copperlist_set_program(GL_Copperlist_t *copperlist, const GL_Program_t *program);

extern void GL_copperlist_surface_to_rgba(const GL_Surface_t *surface, const GL_Copperlist_t *copperlist, GL_Color_t *pixels);

#endif  /* __GL_COPPERLIST_H__ */
