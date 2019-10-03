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

#ifndef __GL_CONTEXT_H__
#define __GL_CONTEXT_H__

#include <stdbool.h>

#include "common.h"
#include "palette.h"
#include "surface.h"

// TODO: move from `float` to 'double`.

typedef struct _GL_Context_t {
    GL_Surface_t surface;
    GL_Quad_t clipping_region;
    GL_Pixel_t background;
    GL_Pixel_t color;
    uint32_t mask;
    GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS];
    GL_Bool_t transparent[GL_MAX_PALETTE_COLORS];
} GL_Context_t;

typedef enum _GL_XForm_Registers_t {
    GL_XForm_Registers_t_First,
    GL_XFORM_REGISTER_H = GL_XForm_Registers_t_First,
    GL_XFORM_REGISTER_V,
    GL_XFORM_REGISTER_A,
    GL_XFORM_REGISTER_B,
    GL_XFORM_REGISTER_C,
    GL_XFORM_REGISTER_D,
    GL_XFORM_REGISTER_X,
    GL_XFORM_REGISTER_Y,
    GL_XForm_Registers_t_Last = GL_XFORM_REGISTER_Y,
    GL_XForm_Registers_t_CountOf
} GL_XForm_Registers_t;

typedef void (*GL_XForm_Callback_t)(float registers[GL_XForm_Registers_t_CountOf], size_t scan_line, void *parameters);

typedef enum _GL_XForm_Clamps_t {
    GL_XFORM_CLAMP_EDGE,
    GL_XFORM_CLAMP_BORDER,
    GL_XFORM_CLAMP_REPEAT
} GL_XForm_Clamps_t;

typedef struct _GL_XForm_t {
    float registers[GL_XForm_Registers_t_CountOf];
    GL_XForm_Clamps_t clamp;
    GL_XForm_Callback_t callback;
    void *callback_parameters;
} GL_XForm_t;

extern bool GL_context_create(GL_Context_t *context, size_t width, size_t height);
extern void GL_context_delete(GL_Context_t *context); // TODO: rename to `*_destroy()`?

extern void GL_context_push(const GL_Context_t *context);
extern void GL_context_pop(const GL_Context_t *context);

extern void GL_context_clear(const GL_Context_t *context);
extern void GL_context_screenshot(const GL_Context_t *context, const GL_Palette_t *palette, const char *pathfile);
extern void GL_context_to_rgba(const GL_Context_t *context, const GL_Palette_t *palette, void *vram);
extern void GL_context_to_surface(const GL_Context_t *context, const GL_Surface_t *surface);

extern void GL_context_blit(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position);
extern void GL_context_blit_s(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position, float sx, float sy);
extern void GL_context_blit_sr(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position, float sx, float sy, float rotation, float ax, float ay);
extern void GL_context_blit_x(const GL_Context_t *context, const GL_Surface_t *surface, GL_Point_t position, GL_XForm_t transformation);

extern void GL_context_shifting(GL_Context_t *context, const size_t *from, const size_t *to, size_t count);
extern void GL_context_transparent(GL_Context_t *context, const GL_Pixel_t *indexes, const GL_Bool_t *transparent, size_t count);
extern void GL_context_clipping(GL_Context_t *context, const GL_Quad_t *clipping_region);
extern void GL_context_background(GL_Context_t *context, GL_Pixel_t index);
extern void GL_context_color(GL_Context_t *context, GL_Pixel_t index);
extern void GL_context_pattern(GL_Context_t *context, uint32_t mask);

extern void GL_context_fill(const GL_Context_t *context, GL_Point_t seed, GL_Pixel_t index);

#endif  /* __GL_CONTEXT_H__ */