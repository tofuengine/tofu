/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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

#ifndef __GL_CONTEXT_H__
#define __GL_CONTEXT_H__

#include "common.h"
#include "palette.h"
#include "surface.h"

#include <stdbool.h>

#ifdef __GL_MASK_SUPPORT__
typedef struct _GL_Mask_t {
    const GL_Surface_t *stencil;
    GL_Pixel_t threshold;
} GL_Mask_t;
#endif

typedef enum _GL_Modes_t {
    GL_MODE_SET,
    GL_MODE_ADD,
    GL_MODE_SUB,
    GL_MODE_MULT,
} GL_Modes_t;

typedef struct _GL_State_t { // FIXME: rename to `GL_State_s`
    GL_Pixel_t background, color;
    GL_Pattern_t pattern;
    GL_Quad_t clipping_region;
    GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS];
    GL_Bool_t transparent[GL_MAX_PALETTE_COLORS];
#ifdef __GL_MASK_SUPPORT__
    GL_Mask_t mask;
#endif
    GL_Modes_t mode;
} GL_State_t;

typedef struct _GL_Context_t {
    GL_Surface_t *surface;
    GL_State_t state;
    GL_State_t *stack;
} GL_Context_t;

typedef GL_Pixel_t (*GL_Process_Callback_t)(void *user_data, GL_Pixel_t from, GL_Pixel_t to);

// TODO: rename decode to convert/grab.
// FIXME: change width-height to `GL_Size_t`.
extern GL_Context_t *GL_context_decode(size_t width, size_t height, const void *pixels, const GL_Surface_Callback_t callback, void *user_data);
extern GL_Context_t *GL_context_create(size_t width, size_t height);
extern void GL_context_destroy(GL_Context_t *context);

extern void GL_context_push(GL_Context_t *context);
extern void GL_context_pop(GL_Context_t *context);
extern void GL_context_reset(GL_Context_t *context);

extern void GL_context_set_background(GL_Context_t *context, GL_Pixel_t index);
extern void GL_context_set_color(GL_Context_t *context, GL_Pixel_t index);
extern void GL_context_set_pattern(GL_Context_t *context, GL_Pattern_t pattern);
extern void GL_context_set_clipping(GL_Context_t *context, const GL_Rectangle_t *region);
extern void GL_context_set_shifting(GL_Context_t *context, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count);
extern void GL_context_set_transparent(GL_Context_t *context, const GL_Pixel_t *indexes, const GL_Bool_t *transparent, size_t count);
#ifdef __GL_MASK_SUPPORT__
extern void GL_context_set_mask(GL_Context_t *context, const GL_Mask_t *mask);
#endif

extern void GL_context_fill(const GL_Context_t *context, GL_Point_t seed, GL_Pixel_t index);
extern void GL_context_process(const GL_Context_t *context, GL_Point_t position, GL_Rectangle_t area, GL_Process_Callback_t callback, void *user_data);
extern void GL_context_copy(const GL_Context_t *context, GL_Point_t position, GL_Rectangle_t area);
extern GL_Pixel_t GL_context_peek(const GL_Context_t *context, int x, int y);
extern void GL_context_poke(GL_Context_t *context, int x, int y, GL_Pixel_t index);

extern void GL_context_clear(const GL_Context_t *context, GL_Pixel_t index);
extern void GL_context_to_surface(const GL_Context_t *context, const GL_Surface_t *to);

#endif  /* __GL_CONTEXT_H__ */
