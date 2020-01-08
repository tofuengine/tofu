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

#include "context.h"

#include <config.h>
#include <libs/imath.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "surface.h"

#define LOG_CONTEXT "gl"

static inline void reset_state(GL_State_t *state, GL_Surface_t *surface)
{
    *state = (GL_State_t){
            .surface = surface,
            .clipping_region = (GL_Quad_t){ .x0 = 0, .y0 = 0, .x1 = surface->width - 1, .y1 = surface->height - 1 },
            .background = 0,
#ifdef __STENCIL_SUPPORT__
            .stencil = NULL,
            .threshold = 0
#endif
        };
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        state->shifting[i] = i;
        state->transparent[i] = GL_BOOL_FALSE;
    }
    state->transparent[0] = GL_BOOL_TRUE;
}

bool GL_context_create(GL_Context_t *context, size_t width, size_t height)
{
    *context = (GL_Context_t){ 0 };

    GL_surface_create(&context->buffer, width, height);

    reset_state(&context->state, &context->buffer);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context created");
    return true;
}

void GL_context_delete(GL_Context_t *context)
{
    arrfree(context->stack);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context stack deallocated");

    GL_surface_delete(&context->buffer);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context buffer deallocated");

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context deleted");
}

void GL_context_push(GL_Context_t *context)
{
    arrpush(context->stack, context->state); // Store current state into stack.
}

void GL_context_pop(GL_Context_t *context)
{
    if (arrlen(context->stack) < 1) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "no states to pop from context");
        return;
    }
    context->state = arrpop(context->stack);
}

void GL_context_reset(GL_Context_t *context)
{
    arrfree(context->stack);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context stack deallocated");

    reset_state(&context->state, &context->buffer);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context reset");
}

void GL_context_sanitize(GL_Context_t *context, const GL_Surface_t *surface)
{
    for (int i = arrlen(context->stack) - 1; i >= 0; --i) {
#ifdef __GL_MASK_SUPPORT__
        if (context->stack[i].surface == surface || context->stack[i].mask.stencil == surface) {
#else
        if (context->stack[i].surface == surface) {
#endif
            arrdel(context->stack, i);
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "state #%d sanitized from context", i);
        }
    }
}

void GL_context_surface(GL_Context_t *context, GL_Surface_t *surface)
{
    GL_Surface_t *buffer = !surface ? &context->buffer : surface;
    if (context->state.surface != buffer) {
        context->state.surface = buffer;
        GL_context_clipping(context, NULL); // Reset the clipping region on surface change.
    }
}

void GL_context_shifting(GL_Context_t *context, const size_t *from, const size_t *to, size_t count)
{
    GL_State_t *state = &context->state;
    if (!from) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            state->shifting[i] = i;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            state->shifting[from[i]] = to[i];
        }
    }
}

void GL_context_transparent(GL_Context_t *context, const GL_Pixel_t *indexes, const GL_Bool_t *transparent, size_t count)
{
    GL_State_t *state = &context->state;
    if (!indexes) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            state->transparent[i] = GL_BOOL_FALSE;
        }
        state->transparent[0] = GL_BOOL_TRUE;
    } else {
        for (size_t i = 0; i < count; ++i) {
            state->transparent[indexes[i]] = transparent[i];
        }
    }
}

void GL_context_clipping(GL_Context_t *context, const GL_Rectangle_t *region)
{
    GL_State_t *state = &context->state;
    if (!region) {
        state->clipping_region = (GL_Quad_t){
                .x0 = 0,
                .y0 = 0,
                .x1 = state->surface->width - 1,
                .y1 = state->surface->height - 1
            };
    } else {
        state->clipping_region = (GL_Quad_t){
                .x0 = imax(0, region->x),
                .y0 = imax(0, region->y),
                .x1 = imin(state->surface->width, region->x + region->width) - 1,
                .y1 = imin(state->surface->height, region->y + region->height) - 1
            };
    }
}

void GL_context_background(GL_Context_t *context, const GL_Pixel_t index)
{
    GL_State_t *state = &context->state;
    state->background = index;
}

void GL_context_color(GL_Context_t *context, GL_Pixel_t index)
{
    GL_State_t *state = &context->state;
    state->color = index;
}

void GL_context_pattern(GL_Context_t *context, uint32_t pattern)
{
    GL_State_t *state = &context->state;
    state->pattern = pattern;
}

#ifdef __GL_MASK_SUPPORT__
void GL_context_mask(GL_Context_t *context, const GL_Mask_t *mask)
{
    GL_State_t *state = &context->state;
    if (!mask) {
        state->mask = (GL_Mask_t){ 0 };
    } else {
        state->mask = *mask;
    }
}
#endif

void GL_context_clear(const GL_Context_t *context)
{
    const GL_State_t *state = &context->state;
    const GL_Pixel_t color = state->background;
    GL_Pixel_t *dst = state->surface->data;
    for (size_t i = state->surface->data_size; i; --i) {
        *(dst++) = color;
    }
}

void GL_context_to_surface(const GL_Context_t *context, const GL_Surface_t *to)
{
    const GL_State_t *state = &context->state;
    const GL_Surface_t *from = state->surface;

    size_t width = imin(from->width, to->width);
    size_t height = imin(from->height, to->height);

    const GL_Pixel_t *src = from->data;
    GL_Pixel_t *dst = to->data;

    const int src_skip = from->width - width;
    const int dst_skip = to->width - width;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            *(dst++) = *(src++);
        }
        src += src_skip;
        dst += dst_skip;
    }
}
