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

#ifndef __GL_CONTEXT_H__
#define __GL_CONTEXT_H__

#include "common.h"
#include "palette.h"
#include "surface.h"

#include <stdbool.h>

typedef struct GL_State_s {
    GL_Quad_t clipping_region;
    GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS];
    GL_Bool_t transparent[GL_MAX_PALETTE_COLORS];
} GL_State_t;

typedef struct GL_Context_s {
    GL_Surface_t *surface;
    struct {
        GL_State_t current;
        GL_State_t *stack;
    } state;
} GL_Context_t;

typedef GL_Pixel_t (*GL_Context_Scan_Callback_t)(void *user_data, GL_Point_t position, GL_Pixel_t index);

typedef GL_Pixel_t (*GL_Context_Process_Callback_t)(void *user_data, GL_Point_t position, GL_Pixel_t from, GL_Pixel_t to);

extern GL_Context_t *GL_context_create(const GL_Surface_t *surface);
extern void GL_context_destroy(GL_Context_t *context);

extern void GL_context_reset(GL_Context_t *context);
extern void GL_context_push(GL_Context_t *context);
extern void GL_context_pop(GL_Context_t *context, size_t levels);

extern void GL_context_set_clipping(GL_Context_t *context, const GL_Rectangle_t *region);
extern void GL_context_set_shifting(GL_Context_t *context, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count);
extern void GL_context_set_transparent(GL_Context_t *context, const GL_Pixel_t *indexes, const GL_Bool_t *transparent, size_t count);

extern void GL_context_fill(const GL_Context_t *context, GL_Point_t seed, GL_Pixel_t index);
extern void GL_context_scan(const GL_Context_t *context, GL_Rectangle_t area, GL_Context_Scan_Callback_t callback, void *user_data);

extern void GL_context_process(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, GL_Context_Process_Callback_t callback, void *user_data);
extern void GL_context_copy(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area);
extern void GL_context_stencil(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, const GL_Surface_t *mask, GL_Comparators_t comparator, GL_Pixel_t threshold);
extern void GL_context_blend(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, GL_Functions_t function);

#endif  /* __GL_CONTEXT_H__ */
