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
            .clipping_region = (GL_Quad_t){ .x0 = 0, .y0 = 0, .x1 = (int)surface->width - 1, .y1 = (int)surface->height - 1 },
            .shifting = { 0 },
            .transparent = { 0 }
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

GL_Context_t *GL_context_create(size_t width, size_t height) // TODO: use `GL_Size_t`?
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

void GL_context_pop(GL_Context_t *context, size_t levels)
{
    const size_t length = arrlen(context->stack);
    if (length < 1) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "no more states to pop from context");
        return;
    }

    for (size_t i = imin(length, levels); i; --i) {
        context->state = arrpop(context->stack);
    }

#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d length state(s) popped from context");
#endif  /* VERBOSE_DEBUG */
}

void GL_context_reset(GL_Context_t *context)
{
    _reset_state(&context->state, context->surface);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context reset");
}

void GL_context_set_background(GL_Context_t *context, GL_Pixel_t index)
{
    GL_State_t *state = &context->state;
    state->background = index;
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

void GL_context_set_color(GL_Context_t *context, GL_Pixel_t index)
{
    GL_State_t *state = &context->state;
    state->color = index;
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

GL_Size_t GL_context_get_size(const GL_Context_t *context)
{
    return (GL_Size_t){ .width = context->surface->width, .height = context->surface->height };
}

GL_Surface_t *GL_context_get_surface(const GL_Context_t *context)
{
    return context->surface;
}

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

    const size_t dwidth = surface->width;

    const size_t dskip = dwidth;

    const GL_Pixel_t match = ddata[seed.y * dwidth + seed.x];
    const GL_Pixel_t replacement = shifting[index];

    GL_Point_t *stack = NULL;
    arrpush(stack, seed);

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

void GL_context_process(const GL_Context_t *context, const GL_Surface_t *source, GL_Rectangle_t area, GL_Point_t position, GL_Process_Callback_t callback, void *user_data)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Surface_t *surface = context->surface;

    size_t skip_x = 0; // Offset into the (source) surface/texture, update during clipping.
    size_t skip_y = 0;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)area.width - 1,
            .y1 = position.y + (int)area.height - 1
        };

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
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t sskip = swidth - width;
    const size_t dskip = dwidth - width;

    const GL_Pixel_t *sptr = sdata + (area.y + skip_y) * swidth + (area.x + skip_x);
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    int y = drawing_region.y0;
    for (int i = height; i; --i) {
        int x = drawing_region.x0; // TODO: optimize?
        for (int j = width; j; --j) {
            GL_Pixel_t from = *dptr;
            GL_Pixel_t to = *(sptr++);
            *(dptr++) = callback(user_data, (GL_Point_t){ .x = x, .y = y}, from, to);
            x += 1;
        }
        sptr += sskip;
        dptr += dskip;
        y += 1;
    }
}

// Note: currently the `GL_context_copy()` function is equal to `GL_context_blit()`. However, we are keeping them
// separate, as in the future they might be different (with the `*_copy()` variant optimized).
void GL_context_copy(const GL_Context_t *context, const GL_Surface_t *source, GL_Rectangle_t area, GL_Point_t position)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = context->surface;

    size_t skip_x = 0; // Offset into the (source) surface/texture, update during clipping.
    size_t skip_y = 0;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)area.width - 1,
            .y1 = position.y + (int)area.height - 1
        };

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
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t sskip = swidth - width;
    const size_t dskip = dwidth - width;

    const GL_Pixel_t *sptr = sdata + (area.y + skip_y) * swidth + (area.x + skip_x);
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            GL_Pixel_t index = shifting[*(sptr++)];
            if (transparent[index]) {
                ++dptr;
            } else {
                *(dptr++) = index;
            }
        }
        sptr += sskip;
        dptr += dskip;
    }
}

typedef bool (*GL_Pixel_Comparator_t)(GL_Pixel_t value, GL_Pixel_t threshold);

static bool _never(GL_Pixel_t value, GL_Pixel_t threshold)
{
    return false;
}

static bool _less(GL_Pixel_t value, GL_Pixel_t threshold)
{
    return value < threshold;
}

static bool _less_or_equal(GL_Pixel_t value, GL_Pixel_t threshold)
{
    return value <= threshold;
}

static bool _greater(GL_Pixel_t value, GL_Pixel_t threshold)
{
    return value > threshold;
}

static bool _greater_or_equal(GL_Pixel_t value, GL_Pixel_t threshold)
{
    return value >= threshold;
}

static bool _equal(GL_Pixel_t value, GL_Pixel_t threshold)
{
    return value == threshold;
}

static bool _not_equal(GL_Pixel_t value, GL_Pixel_t threshold)
{
    return value != threshold;
}

static bool _always(GL_Pixel_t value, GL_Pixel_t threshold)
{
    return true;
}

const GL_Pixel_Comparator_t _pixel_comparators[GL_Comparators_t_CountOf] = {
    _never, _less, _less_or_equal, _greater, _greater_or_equal, _equal, _not_equal, _always
};

void GL_context_stencil(const GL_Context_t *context, const GL_Surface_t *source, const GL_Surface_t *mask, GL_Comparators_t comparator, GL_Pixel_t threshold, GL_Rectangle_t area, GL_Point_t position)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting; // TODO: should `GL_context_copy()` and `GL_context_mask()` skip shifting and transparency?
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = context->surface;
    const GL_Pixel_Comparator_t should_write = _pixel_comparators[comparator];

#ifdef __DEFENSIVE_CHECKS__
    if (source->width != mask->width || source->height != mask->height) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "source and mask surfaces need to match in size");
        return;
    }
#endif

    size_t skip_x = 0; // Offset into the (source) surface/texture, update during clipping.
    size_t skip_y = 0;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)area.width - 1,
            .y1 = position.y + (int)area.height - 1
        };

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
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    const GL_Pixel_t *sdata = source->data;
    const GL_Pixel_t *mdata = mask->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t mwidth = mask->width;
    const size_t dwidth = surface->width;

    const size_t sskip = swidth - width;
    const size_t mskip = mwidth - width;
    const size_t dskip = dwidth - width;

    const GL_Pixel_t *sptr = sdata + (area.y + skip_y) * swidth + (area.x + skip_x);
    const GL_Pixel_t *mptr = mdata + (area.y + skip_y) * mwidth + (area.x + skip_x);
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
            pixel(surface, drawing_region.x0 + width - j, drawing_region.y0 + height - i, i + j);
#endif
            const GL_Pixel_t value = *(mptr++);
            const GL_Pixel_t index = shifting[*(sptr++)];
            if (transparent[index] || !should_write(value, threshold)) {
                ++dptr;
            } else {
                *(dptr++) = index;
            }
        }
        sptr += sskip;
        mptr += mskip;
        dptr += dskip;
    }
}

GL_Pixel_t GL_context_peek(const GL_Context_t *context, GL_Point_t position)
{
    const GL_Surface_t *surface = context->surface;
    return surface->data[position.y * surface->width + position.x];
}

void GL_context_poke(GL_Context_t *context, GL_Point_t position, GL_Pixel_t index)
{
    GL_Surface_t *surface = context->surface;
    surface->data[position.y * surface->width + position.x] = index;
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
