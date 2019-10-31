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

#include "../config.h"

#include <stdbool.h>

#include "common.h"
#include "palette.h"
#include "surface.h"

#define GL_XFORM_TABLE_MAX_OPERATIONS       16

#ifdef __GL_MASK_SUPPORT__
typedef struct _GL_Mask_t {
    const GL_Surface_t *stencil;
    GL_Pixel_t threshold;
} GL_Mask_t;
#endif

typedef struct _GL_State_t {
    GL_Surface_t *surface;
    GL_Quad_t clipping_region;
    GL_Pixel_t background;
    GL_Pixel_t color; // TODO: use it!
    uint32_t pattern; // TODO: ditto
    GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS];
    GL_Bool_t transparent[GL_MAX_PALETTE_COLORS];
#ifdef __GL_MASK_SUPPORT__
    GL_Mask_t mask;
#endif
} GL_State_t;

typedef struct _GL_Context_t {
    GL_Surface_t buffer;
    GL_State_t state;
    GL_State_t *stack;
} GL_Context_t;

extern bool GL_context_create(GL_Context_t *context, size_t width, size_t height);
extern void GL_context_delete(GL_Context_t *context); // TODO: rename to `*_destroy()`?

extern void GL_context_push(GL_Context_t *context);
extern void GL_context_pop(GL_Context_t *context);
extern void GL_context_sanitize(GL_Context_t *context, const GL_Surface_t *surface);

extern void GL_context_surface(GL_Context_t *context, GL_Surface_t *surface);
extern void GL_context_shifting(GL_Context_t *context, const size_t *from, const size_t *to, size_t count);
extern void GL_context_transparent(GL_Context_t *context, const GL_Pixel_t *indexes, const GL_Bool_t *transparent, size_t count);
extern void GL_context_clipping(GL_Context_t *context, const GL_Quad_t *clipping_region);
extern void GL_context_background(GL_Context_t *context, GL_Pixel_t index);
extern void GL_context_color(GL_Context_t *context, GL_Pixel_t index);
extern void GL_context_pattern(GL_Context_t *context, uint32_t pattern);
#ifdef __GL_MASK_SUPPORT__
extern void GL_context_mask(GL_Context_t *context, const GL_Mask_t *mask);
#endif

extern void GL_context_clear(const GL_Context_t *context);
extern void GL_context_to_surface(const GL_Context_t *context, const GL_Surface_t *to);

#endif  /* __GL_CONTEXT_H__ */