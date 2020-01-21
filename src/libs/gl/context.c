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

static inline void _reset_state(GL_State_t *state, const GL_Surface_t *surface)
{
    *state = (GL_State_t){
            .background = 0,
            .color = 1,
            .clipping_region = (GL_Quad_t){ .x0 = 0, .y0 = 0, .x1 = surface->width - 1, .y1 = surface->height - 1 },
            .shifting = { 0 },
            .transparent = { 0 }
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

GL_Context_t *GL_context_decode(const void *buffer, size_t buffer_size, const GL_Surface_Callback_t callback, void *user_data)
{
    GL_Surface_t *surface = GL_surface_decode(buffer, buffer_size, callback, user_data);
    if (!surface) {
        return NULL;
    }

    GL_Context_t *context = malloc(sizeof(GL_Context_t));
    if (!context) {
        return NULL;
    }

    *context = (GL_Context_t){
            .surface = surface
        };

    _reset_state(&context->state, surface);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context decoded");
    return context;
}

GL_Context_t *GL_context_create(size_t width, size_t height)
{
    GL_Surface_t *surface = GL_surface_create(width, height);
    if (!surface) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "context create %dx%d surface", width, height);
        return NULL;
    }

    GL_Context_t *context = malloc(sizeof(GL_Context_t));
    if (!context) {
        return NULL;
    }

    *context = (GL_Context_t){
            .surface = surface
        };

    _reset_state(&context->state, surface);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context created");
    return context;
}

void GL_context_destroy(GL_Context_t *context)
{
    arrfree(context->stack);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context stack deallocated");

    GL_surface_destroy(context->surface);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context surface deleted");

    free(context);
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

    _reset_state(&context->state, context->surface);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context reset");
}

void GL_context_background(GL_Context_t *context, GL_Pixel_t index)
{
    GL_State_t *state = &context->state;
    state->background = index;
}

void GL_context_color(GL_Context_t *context, GL_Pixel_t index)
{
    GL_State_t *state = &context->state;
    state->color = index;
}

void GL_context_clipping(GL_Context_t *context, const GL_Rectangle_t *region)
{
    GL_State_t *state = &context->state;
    if (!region) {
        state->clipping_region = (GL_Quad_t){
                .x0 = 0,
                .y0 = 0,
                .x1 = context->surface->width - 1,
                .y1 = context->surface->height - 1
            };
    } else {
        state->clipping_region = (GL_Quad_t){
                .x0 = imax(0, region->x),
                .y0 = imax(0, region->y),
                .x1 = imin(context->surface->width, region->x + region->width) - 1,
                .y1 = imin(context->surface->height, region->y + region->height) - 1
            };
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

void GL_context_clear(const GL_Context_t *context, GL_Pixel_t index)
{
    GL_Pixel_t *dst = context->surface->data;
    for (size_t i = context->surface->data_size; i; --i) {
        *(dst++) = index;
    }
}

void GL_context_to_surface(const GL_Context_t *context, const GL_Surface_t *to)
{
    const GL_Surface_t *from = context->surface;

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
