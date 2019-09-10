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

#ifndef __GL_H__
#define __GL_H__

#include <stdbool.h>

#include "common.h"
#include "palette.h"
#include "primitive.h"
#include "program.h"
#include "sheet.h"
#include "surface.h"
#include "texture.h"

typedef enum _GL_Raster_Operations_t {
    RASTER_OPERATION_NOP,
    RASTER_OPERATION_SET,
    RASTER_OPERATION_AND,
    RASTER_OPERATION_OR,
    RASTER_OPERATION_NOT,
    RASTER_OPERATION_XOR,
    GL_Raster_Operations_t_CountOf
} GL_Raster_Operations_t;

typedef struct _GL_Context_t {
    GL_Pixel_t background;
    GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS];
    GL_Bool_t transparent[GL_MAX_PALETTE_COLORS];
    GL_Rectangle_t clipping_region;
    GL_Palette_t palette;
    GL_Raster_Operations_t raster_operation;
} GL_Context_t;

typedef struct _GL_t {
    GL_Surface_t surface;
    GL_Context_t context;
} GL_t;

extern bool GL_initialize(GL_t *gl, size_t width, size_t height);
extern void GL_terminate(GL_t *gl);
extern void GL_push(GL_t *gl);
extern void GL_pop(GL_t *gl);
extern void GL_clear(GL_t *gl);
extern void GL_prepare(const GL_t *gl, void *vram);

#endif  /* __GL_H__ */