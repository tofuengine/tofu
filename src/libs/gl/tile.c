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

#include "tile.h"

#include <config.h>
#include <libs/imath.h>
#include <libs/sincos.h>

#include <math.h>

#ifdef __DEBUG_GRAPHICS__
static inline void pixel(const GL_Surface_t *context, int x, int y, int index)
{
    surface->data[y * surface->width + x]= 240 + (index % 16);
}
#endif

void GL_context_tile(const GL_Context_t *context, const GL_Surface_t *source, GL_Rectangle_t area, GL_Point_t position, GL_Point_t offset)
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

    const size_t width = drawing_region.x1 - drawing_region.x0 + 1;
    const size_t height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width == 0) || (height == 0)) { // Nothing to draw! Bail out! (can't be negative, by definition)
        return;
    }

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t ssize = area.height * source->width;

    const size_t dskip = dwidth - width;

    const GL_Pixel_t *sptr = sdata + (area.y + skip_y) * swidth + (area.x + skip_x);
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const GL_Pixel_t *sveod = sptr + ssize;
    sptr += offset.y * swidth;
    for (size_t i = height; i; --i) {
        const GL_Pixel_t *sheod = sptr + width;
        const GL_Pixel_t *sss = sptr + offset.x;
        for (size_t j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
            pixel(surface, drawing_region.x0 + (int)width - (int)j, drawing_region.y0 + (int)height - (int)i, (int)i + (int)j);
#endif
            const GL_Pixel_t index = shifting[*(sss++)];
            if (transparent[index]) {
                dptr++;
            } else {
                *(dptr++) = index;
            }
            if (sss >= sheod) {
                sss -= area.width;
            }
        }
        sptr += swidth;
        dptr += dskip;
        if (sptr >= sveod) {
            sptr -= ssize;
        }
    }
}

void GL_context_tile_s(const GL_Context_t *context, const GL_Surface_t *source, GL_Rectangle_t area, GL_Point_t position, GL_Point_t offset, int scale_x, int scale_y)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = context->surface;

    const size_t sw = area.width * IABS(scale_x);
    const size_t sh = area.height * IABS(scale_y);

    size_t skip_x = 0; // Offset into the (source) surface/texture, updated during clipping.
    size_t skip_y = 0;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)sw - 1,
            .y1 = position.y + (int)sh - 1
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

    const size_t width = drawing_region.x1 - drawing_region.x0 + 1;
    const size_t height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width == 0) || (height == 0)) { // Nothing to draw! Bail out! (can't be negative, by definition)
        return;
    }

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t dskip = dwidth - width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const int su = IABS(scale_x);
    const int sv = IABS(scale_y);
    const int ou0 = (int)skip_x / su;
    const int ov0 = (int)skip_y / sv;
    const int ru0 = skip_x % su;
    const int rv0 = skip_y % sv;

    const int ou = area.x + (scale_x < 0 ? (int)area.width - 1 - ou0 : ou0); // Offset to the correct margin, according to flipping.
    const int ov = area.y + (scale_y < 0 ? (int)area.height - 1 - ov0 : ov0);

    const int du = scale_x > 0 ? 1 : -1;
    const int dv = scale_y > 0 ? 1 : -1;

    int v = ov;
    int rv = rv0;
    for (size_t i = height; i; --i) {
        const int y = (int)v;
        const GL_Pixel_t *sptr = sdata + y * swidth;

        int u = ou;
        int ru = ru0;
        for (size_t j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
            pixel(surface, drawing_region.x0 + (int)width - (int)j, drawing_region.y0 + (int)height - (int)i, (int)i + (int)j);
#endif
            const int x = (int)u;
            GL_Pixel_t index = shifting[sptr[x]];
            if (transparent[index]) {
                dptr++;
            } else {
                *(dptr++) = index;
            }
            ru += 1;
            if (ru == su) {
                u += du;
                ru = 0;
            }
        }

        rv += 1;
        if (rv == sv) {
            v += dv;
            rv = 0;
        }
        dptr += dskip;
    }
}
