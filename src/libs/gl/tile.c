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

#ifdef __DEBUG_GRAPHICS__
static inline void _pixel(const GL_Surface_t *surface, int x, int y, int index)
{
    surface->data[y * surface->width + x]= (GL_Pixel_t)(240 + (index % 16));
}
#endif

extern void GL_surface_tile(const GL_Surface_t *surface, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, GL_Point_t offset)
{
    const GL_State_t *state = &surface->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    size_t skip_x = offset.x; // Offset into the (source) surface/texture, update during clipping.
    size_t skip_y = offset.y;

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

    const size_t dskip = dwidth - width;

    const GL_Pixel_t *sptr = sdata + area.y * swidth + area.x;
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const int ou = IMOD(skip_x, area.width);
    const int ov = IMOD(skip_y, area.height);

    int v = ov;
    for (int i = height; i; --i) {
        const GL_Pixel_t *srow = sptr + v * swidth;

        int u = ou;
        for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
            _pixel(surface, drawing_region.x0 + width - j, drawing_region.y0 + height - i, i + j);
#endif
            const GL_Pixel_t index = shifting[srow[u]];
            if (transparent[index]) {
                ++dptr;
            } else {
                *(dptr++) = index;
            }
            u = (u + 1) % (int)area.width; // Prefer modulo over branch.
        }
        v = (v + 1) % (int)area.height;
        dptr += dskip;
    }
}

void GL_surface_tile_s(const GL_Surface_t *surface, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, GL_Point_t offset, int scale_x, int scale_y)
{
    const GL_State_t *state = &surface->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    const size_t sw = area.width * IABS(scale_x);
    const size_t sh = area.height * IABS(scale_y);

    size_t skip_x = 0; // Offset into the (source) surface/texture, updated during clipping.
    size_t skip_y = 0;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)sw,
            .y1 = position.y + (int)sh
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

    const int width = drawing_region.x1 - drawing_region.x0;
    const int height = drawing_region.y1 - drawing_region.y0;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t dskip = dwidth - width;

    const GL_Pixel_t *sptr = sdata + area.y * swidth + area.x;
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    // We are implementing a DDA integer-remainders algorithm, as the scaling factors are integer. For both `x` and `y`
    // (which we are calling `u` and `v` in the source surface texture) we need to pre-calculate:
    //
    //   - the absolute value of the scaling factor, i.e. the count limit after which a pixel need to change;
    //   - the current remainder value, in the source texture;
    //   - the source origin `u` and `v`, scaled, given that we might have skipped some pixels due to clipping, and
    //     moved to the correct margin due to x/y flipping;
    //   - the movement vector, according to x/y flipping.
    const int su = IABS(scale_x);
    const int sv = IABS(scale_y);
    const int ru0 = (int)skip_x % su;
    const int rv0 = (int)skip_y % sv;

    const int ou0 = (int)skip_x / su;
    const int ov0 = (int)skip_y / sv;
    const int ou1 = (scale_x < 0 ? (int)area.width - 1 - ou0 : ou0); // Offset to the correct margin, according to flipping.
    const int ov1 = (scale_y < 0 ? (int)area.height - 1 - ov0 : ov0);
    const int ou = IMOD(ou1 + offset.x, area.width); // Pre-add the offset (wrapping around). Negative offset is supported.
    const int ov = IMOD(ov1 + offset.y, area.height);

    const int du = scale_x > 0 ? 1 : -1;
    const int dv = scale_y > 0 ? 1 : -1;

    int v = ov;
    int rv = rv0;
    for (int i = height; i; --i) {
        const GL_Pixel_t *srow = sptr + v * swidth;

        int u = ou;
        int ru = ru0;
        for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
            _pixel(surface, drawing_region.x0 + width - j, drawing_region.y0 + height - i, i + j);
#endif
            GL_Pixel_t index = shifting[srow[u]];
            if (transparent[index]) {
                ++dptr;
            } else {
                *(dptr++) = index;
            }
            ru += 1;
            if (ru == su) { // The remainder has reached the (scaling) limit, move to the next pixel and reset.
                u = IMOD(u + du, area.width); // Prefer modulo over branch.
                ru = 0;
            }
        }

        rv += 1;
        if (rv == sv) { // Ditto.
            v = IMOD(v + dv, area.height); // Ditto.
            rv = 0;
        }
        dptr += dskip;
    }
}
