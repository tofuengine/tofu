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

#define LOG_CONTEXT "gl"

static inline void _reset_state(GL_State_t *state, const GL_Surface_t *surface)
{
    *state = (GL_State_t){
            .background = 0,
            .color = 1,
            .pattern = 0,
            .clipping_region = (GL_Quad_t){ .x0 = 0, .y0 = 0, .x1 = (int)surface->width - 1, .y1 = (int)surface->height - 1 },
            .shifting = { 0 },
            .transparent = { 0 }
#ifdef __STENCIL_SUPPORT__
            .stencil = NULL,
            .threshold = 0
#endif
        };
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        state->shifting[i] = (GL_Pixel_t)i;
        state->transparent[i] = GL_BOOL_FALSE;
    }
    state->transparent[0] = GL_BOOL_TRUE;
}

GL_Context_t *GL_context_decode(size_t width, size_t height, const void *pixels, const GL_Surface_Callback_t callback, void *user_data)
{
    GL_Surface_t *surface = GL_surface_decode(width, height, pixels, callback, user_data);
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
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context stack freed");

    GL_surface_destroy(context->surface);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context surface destroyed");

    free(context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context freed");
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
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context stack freed");

    _reset_state(&context->state, context->surface);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context reset");
}

void GL_context_set_background(GL_Context_t *context, GL_Pixel_t index)
{
    GL_State_t *state = &context->state;
    state->background = index;
}

void GL_context_set_color(GL_Context_t *context, GL_Pixel_t index)
{
    GL_State_t *state = &context->state;
    state->color = index;
}

void GL_context_set_pattern(GL_Context_t *context, GL_Pattern_t pattern)
{
    GL_State_t *state = &context->state;
    state->pattern = pattern;
}

void GL_context_set_clipping(GL_Context_t *context, const GL_Rectangle_t *region)
{
    GL_State_t *state = &context->state;
    if (!region) {
        state->clipping_region = (GL_Quad_t){
                .x0 = 0,
                .y0 = 0,
                .x1 = (int)context->surface->width - 1,
                .y1 = (int)context->surface->height - 1
            };
    } else {
        state->clipping_region = (GL_Quad_t){
                .x0 = imax(0, region->x),
                .y0 = imax(0, region->y),
                .x1 = imin((int)context->surface->width, region->x + (int)region->width) - 1,
                .y1 = imin((int)context->surface->height, region->y + (int)region->height) - 1
            };
    }
}

void GL_context_set_shifting(GL_Context_t *context, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count)
{
    GL_State_t *state = &context->state;
    if (!from) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            state->shifting[i] = (GL_Pixel_t)i;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            state->shifting[from[i]] = to[i];
        }
    }
}

void GL_context_set_transparent(GL_Context_t *context, const GL_Pixel_t *indexes, const GL_Bool_t *transparent, size_t count)
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
void GL_context_set_mask(GL_Context_t *context, const GL_Mask_t *mask)
{
    GL_State_t *state = &context->state;
    if (!mask) {
        state->mask = (GL_Mask_t){ 0 };
    } else {
        state->mask = *mask;
    }
}
#endif

// https://lodev.org/cgtutor/floodfill.html
void GL_context_fill(const GL_Context_t *context, GL_Point_t seed, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Surface_t *surface = context->surface;

    if (seed.x < clipping_region->x0 || seed.x > clipping_region->x1
        || seed.y < clipping_region->y0 || seed.y > clipping_region->y1) {
        return;
    }

    GL_Pixel_t *ddata = surface->data;

    const int dwidth = (int)surface->width;

    const GL_Pixel_t match = ddata[seed.y * dwidth + seed.x];
    const GL_Pixel_t replacement = shifting[index];

    GL_Point_t *stack = NULL;
    arrpush(stack, seed);

    const int dskip = (int)context->surface->width;

    while (arrlen(stack) > 0) {
        const GL_Point_t position = arrpop(stack);

        int x = position.x;
        int y = position.y;

        GL_Pixel_t *dptr = ddata + y * dwidth + x;
        while (x >= clipping_region->x0 && *dptr == match) {
            --x;
            --dptr;
        }
        ++x;
        ++dptr;

        bool above = false;
        bool below = false;

        while (x <= clipping_region->x1 && *dptr == match) {
            *dptr = replacement;

            const GL_Pixel_t pixel_above = *(dptr - dskip);
            if (!above && y >= clipping_region->y0 && pixel_above == match) {
                const GL_Point_t p = (GL_Point_t){ .x = x, .y = y - 1 };
                arrpush(stack, p);
                above = true;
            } else
            if (above && y >= clipping_region->y0 && pixel_above != match) {
                above = false;
            }

            const GL_Pixel_t pixel_below = *(dptr + dskip);
            if (!below && y < clipping_region->y1 && pixel_below == match) {
                const GL_Point_t p = (GL_Point_t){ .x = x, .y = y + 1 };
                arrpush(stack, p);
                below = true;
            } else
            if (below && y < clipping_region->y1 && pixel_below != match) {
                above = false;
            }

            ++x;
            ++dptr;
        }
    }

    arrfree(stack);
}

void GL_context_process(const GL_Context_t *context, GL_Rectangle_t rectangle)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = context->surface;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = rectangle.x,
            .y0 = rectangle.y,
            .x1 = rectangle.x + (int)rectangle.width - 1,
            .y1 = rectangle.y + (int)rectangle.height - 1
        };

    if (drawing_region.x0 < clipping_region->x0) {
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    GL_Pixel_t *sddata = surface->data;

    const int sdwidth = (int)surface->width;

    GL_Pixel_t *sdptr = sddata + drawing_region.y0 * sdwidth + drawing_region.x0;

    const int sdskip = sdwidth - width;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            GL_Pixel_t index = shifting[*sdptr];
            if (transparent[index]) {
                sdptr++;
            } else {
                *(sdptr++) = index;
            }
        }
        sdptr += sdskip;
    }
}

void GL_context_copy(const GL_Context_t *context, GL_Point_t position, GL_Rectangle_t area)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = context->surface;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)area.width - 1,
            .y1 = position.y + (int)area.height - 1
        };

    int skip_x = 0; // Offset into the (source) surface/texture, update during clipping.
    int skip_y = 0;

    if (drawing_region.x0 < clipping_region->x0) {
        skip_x = clipping_region->x0 - drawing_region.x0;
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        skip_y = clipping_region->y0 - drawing_region.y0;
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    GL_Pixel_t *sddata = surface->data;

    const int sdwidth = (int)surface->width;

    const int sdskip = sdwidth - width;

    const GL_Pixel_t *sptr = sddata + (position.y + skip_y) * sdwidth + (position.x + skip_x);
    GL_Pixel_t *dptr = sddata + drawing_region.y0 * sdwidth + drawing_region.x0;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            GL_Pixel_t index = shifting[*(sptr++)];
            if (transparent[index]) {
                dptr++;
            } else {
                *(dptr++) = index;
            }
        }
        sptr += sdskip;
        dptr += sdskip;
    }
}

GL_Pixel_t GL_context_peek(const GL_Context_t *context, int x, int y)
{
    const GL_Surface_t *surface = context->surface;
    return surface->data[y * surface->width + x];
}

void GL_context_poke(GL_Context_t *context, int x, int y, GL_Pixel_t index)
{
    GL_Surface_t *surface = context->surface;
    surface->data[y * surface->width + x] = index;
}

void GL_context_clear(const GL_Context_t *context, GL_Pixel_t index)
{
#ifdef __NO_MEMSET_MEMCPY__
    GL_Pixel_t *dst = context->surface->data;
    for (size_t i = context->surface->data_size; i; --i) {
        *(dst++) = index;
    }
#else
    const GL_Surface_t *surface = context->surface;
    memset(surface->data, index, surface->data_size);
#endif
}

void GL_context_to_surface(const GL_Context_t *context, const GL_Surface_t *to)
{
    const GL_Surface_t *from = context->surface;

    size_t width = (size_t)imin((int)from->width, (int)to->width);
    size_t height = (size_t)imin((int)from->height, (int)to->height);

    const GL_Pixel_t *src = from->data;
    GL_Pixel_t *dst = to->data;

    const int src_skip = (int)from->width - (int)width;
    const int dst_skip = (int)to->width - (int)width;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            *(dst++) = *(src++);
        }
        src += src_skip;
        dst += dst_skip;
    }
}
