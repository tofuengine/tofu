/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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

#include "draw.h"

#include <core/config.h>
#include <libs/imath.h>
#define _LOG_TAG "gl-draw"
#include <libs/log.h>
#include <libs/stb.h>

#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
static inline void _pixel(const GL_Surface_t *surface, int x, int y, int index)
{
    surface->data[y * surface->width + x]= (GL_Pixel_t)(240 + (index % 16));
}
#endif


// https://lodev.org/cgtutor/floodfill.html
void GL_context_fill(const GL_Context_t *context, GL_Point_t seed, GL_Pixel_t index, bool transparency)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    if (seed.x < clipping_region->x0 || seed.x >= clipping_region->x1
        || seed.y < clipping_region->y0 || seed.y >= clipping_region->y1) {
        return;
    }

    GL_Pixel_t *ddata = surface->data;

    const size_t dwidth = surface->width;

    const size_t dskip = dwidth;

    const GL_Pixel_t match = ddata[seed.y * dwidth + seed.x];
    const GL_Pixel_t replacement = shifting[index];

    if (transparency && transparent[replacement]) {
        return;
    }

    GL_Point_t *stack = NULL;
    arrpush(stack, seed);

    while (arrlenu(stack) > 0) {
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

        while (x < clipping_region->x1 && *dptr == match) {
            *dptr = replacement;

            const GL_Pixel_t pixel_above = *(dptr - dskip);
            if (!above && y > clipping_region->y0 && pixel_above == match) {
                const GL_Point_t p = (GL_Point_t){ .x = x, .y = y - 1 };
                arrpush(stack, p);
                above = true;
            } else
            if (above && y > clipping_region->y0 && pixel_above != match) {
                above = false;
            }

            const GL_Pixel_t pixel_below = *(dptr + dskip);
            if (!below && y < clipping_region->y1 - 1 && pixel_below == match) {
                const GL_Point_t p = (GL_Point_t){ .x = x, .y = y + 1 };
                arrpush(stack, p);
                below = true;
            } else
            if (below && y < clipping_region->y1 - 1 && pixel_below != match) {
                below = false;
            }

            ++x;
            ++dptr;
        }
    }

    arrfree(stack);
}

void GL_context_scan(const GL_Context_t *context, GL_Rectangle_t area, GL_Context_Scan_Callback_t callback, void *user_data)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = area.x,
            .y0 = area.y,
            .x1 = area.x + (int)area.width,
            .y1 = area.y + (int)area.height
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

    const int width = drawing_region.x1 - drawing_region.x0;
    const int height = drawing_region.y1 - drawing_region.y0;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    GL_Pixel_t *ddata = surface->data;

    const size_t dwidth = surface->width;

    const size_t dskip = dwidth - width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    int y = drawing_region.y0;
    for (int i = height; i; --i) {
        int x = drawing_region.x0; // TODO: optimize?
        for (int j = width; j; --j) {
            const GL_Pixel_t index = shifting[callback(user_data, (GL_Point_t){ .x = x, .y = y }, *dptr)];
            if (transparent[index]) {
                ++dptr;
            } else {
                *(dptr++) = index;
            }
            x += 1;
        }
        dptr += dskip;
        y += 1;
    }
}

void GL_context_process(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, GL_Context_Process_Callback_t callback, void *user_data)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    int skip_x = area.x; // Offset into the (source) surface/texture, updated during clipping.
    int skip_y = area.y;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)area.width,
            .y1 = position.y + (int)area.height
        };

    if (drawing_region.x0 < clipping_region->x0) {
        skip_x += clipping_region->x0 - drawing_region.x0;
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        skip_y += clipping_region->y0 - drawing_region.y0;
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0;
    const int height = drawing_region.y1 - drawing_region.y0;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t sskip = swidth - width;
    const size_t dskip = dwidth - width;

    const GL_Pixel_t *sptr = sdata + skip_y * swidth + skip_x;
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    int y = drawing_region.y0;
    for (int i = height; i; --i) {
        int x = drawing_region.x0; // TODO: optimize?
        for (int j = width; j; --j) {
            const GL_Pixel_t from = *dptr;
            const GL_Pixel_t to = *(sptr++);
            const GL_Pixel_t index = shifting[callback(user_data, (GL_Point_t){ .x = x, .y = y }, from, to)];
            if (transparent[index]) {
                ++dptr;
            } else {
                *(dptr++) = index;
            }
            x += 1;
        }
        sptr += sskip;
        dptr += dskip;
        y += 1;
    }
}

// Note: `GL_context_copy()` is optimized, when compared to `GL_context_blit()`, since *no* shifting *nor*
// transparency are applied.
void GL_context_copy(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;

    int skip_x = area.x; // Offset into the (source) surface/texture, update during clipping.
    int skip_y = area.y;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)area.width,
            .y1 = position.y + (int)area.height
        };

    if (drawing_region.x0 < clipping_region->x0) {
        skip_x += clipping_region->x0 - drawing_region.x0;
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        skip_y += clipping_region->y0 - drawing_region.y0;
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0;
    const int height = drawing_region.y1 - drawing_region.y0;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t sskip = swidth - width;
    const size_t dskip = dwidth - width;

    const GL_Pixel_t *sptr = sdata + skip_y * swidth + skip_x;
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            *(dptr++) = *(sptr++);
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

void GL_context_stencil(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, const GL_Surface_t *mask, GL_Comparators_t comparator, GL_Pixel_t threshold)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent; // TODO: should `GL_surface_copy()` and `GL_surface_mask()` skip shifting and transparency?
    const GL_Pixel_Comparator_t should_write = _pixel_comparators[comparator];

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (source->width != mask->width || source->height != mask->height) {
        LOG_W("source and mask surfaces need to match in size");
        return;
    }
#endif

    int skip_x = area.x; // Offset into the (source) surface/texture, update during clipping.
    int skip_y = area.y;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)area.width,
            .y1 = position.y + (int)area.height
        };

    if (drawing_region.x0 < clipping_region->x0) {
        skip_x += clipping_region->x0 - drawing_region.x0;
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        skip_y += clipping_region->y0 - drawing_region.y0;
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0;
    const int height = drawing_region.y1 - drawing_region.y0;
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

    const GL_Pixel_t *sptr = sdata + skip_y * swidth + skip_x;
    const GL_Pixel_t *mptr = mdata + skip_y * mwidth + skip_x;
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
            _pixel(surface, drawing_region.x0 + width - j, drawing_region.y0 + height - i, i + j);
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

typedef GL_Pixel_t (*GL_Pixel_Function_t)(GL_Pixel_t destination, GL_Pixel_t source);

static GL_Pixel_t _replace(GL_Pixel_t destination, GL_Pixel_t source)
{
    return source;
}

static GL_Pixel_t _add(GL_Pixel_t destination, GL_Pixel_t source)
{
    return destination + source;
}

static GL_Pixel_t _add_clamped(GL_Pixel_t destination, GL_Pixel_t source)
{
    const int value = (int)destination + (int)source;
    return (GL_Pixel_t)ICLAMP(value, 0, 255);
}

static GL_Pixel_t _subtract(GL_Pixel_t destination, GL_Pixel_t source)
{
    return destination - source;
}

static GL_Pixel_t _subtract_clamped(GL_Pixel_t destination, GL_Pixel_t source)
{
    const int value = (int)destination - (int)source;
    return (GL_Pixel_t)ICLAMP(value, 0, 255);
}

static GL_Pixel_t _reverse_subtract(GL_Pixel_t destination, GL_Pixel_t source)
{
    return source - destination;
}

static GL_Pixel_t _reverse_subtract_clamped(GL_Pixel_t destination, GL_Pixel_t source)
{
    const int value = (int)source - (int)destination;
    return (GL_Pixel_t)ICLAMP(value, 0, 255);
}

static GL_Pixel_t _multiply(GL_Pixel_t destination, GL_Pixel_t source)
{
    return destination * source;
}

static GL_Pixel_t _multiply_clamped(GL_Pixel_t destination, GL_Pixel_t source)
{
    const int value = (int)destination * (int)source;
    return (GL_Pixel_t)ICLAMP(value, 0, 255);
}

static GL_Pixel_t _min(GL_Pixel_t destination, GL_Pixel_t source)
{
    return destination < source ? destination : source;
}

static GL_Pixel_t _max(GL_Pixel_t destination, GL_Pixel_t source)
{
    return destination > source ? destination : source;
}

const GL_Pixel_Function_t _pixel_functions[GL_Functions_t_CountOf] = {
    _replace,
    _add, _add_clamped,
    _subtract, _subtract_clamped,
    _reverse_subtract, _reverse_subtract_clamped,
    _multiply, _multiply_clamped,
    _min,
    _max
};

void GL_context_blend(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, GL_Functions_t function)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Pixel_Function_t blend = _pixel_functions[function];

    int skip_x = area.x; // Offset into the (source) surface/texture, update during clipping.
    int skip_y = area.y;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)area.width,
            .y1 = position.y + (int)area.height
        };

    if (drawing_region.x0 < clipping_region->x0) {
        skip_x += clipping_region->x0 - drawing_region.x0;
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        skip_y += clipping_region->y0 - drawing_region.y0;
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0;
    const int height = drawing_region.y1 - drawing_region.y0;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t sskip = swidth - width;
    const size_t dskip = dwidth - width;

    const GL_Pixel_t *sptr = sdata + skip_y * swidth + skip_x;
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
            _pixel(surface, drawing_region.x0 + width - j, drawing_region.y0 + height - i, i + j);
#endif
            const GL_Pixel_t index = shifting[blend(*dptr, *(sptr++))];
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
