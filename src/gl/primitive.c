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

#include "primitive.h"

#include "../core/imath.h"
#include "gl.h"

#include <math.h>

#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif
#include <stb/stb_ds.h>

static void point(const GL_Surface_t *surface, const GL_Quad_t *clipping_region, int x, int y, GL_Pixel_t index)
{
    if (x < clipping_region->x0) {
        return;
    }
    if (y < clipping_region->y0) {
        return;
    }
    if (x > clipping_region->x1) {
        return;
    }
    if (y > clipping_region->y1) {
        return;
    }

    GL_Pixel_t *dst = surface->data_rows[y] + x;

    *dst = index;
}

static void hline(const GL_Surface_t *surface, const GL_Quad_t *clipping_region, int x, int y, int length, GL_Pixel_t index)
{
    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = x,
            .y0 = y,
            .x1 = x + length - 1,
            .y1 = y
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

    GL_Pixel_t *dst = surface->data_rows[drawing_region.y0] + drawing_region.x0;

    for (int i = width; i; --i) {
        *(dst++) = index;
    }
}

static void vline(const GL_Surface_t *surface, const GL_Quad_t *clipping_region, int x, int y, int length, GL_Pixel_t index)
{
    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = x,
            .y0 = y,
            .x1 = x,
            .y1 = y + length - 1
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

    GL_Pixel_t *dst = surface->data_rows[drawing_region.y0] + drawing_region.x0;

    const int skip = surface->width;

    for (int i = height; i; --i) {
        *dst = index;
        dst += skip;
    }
}

void GL_primitive_point(const GL_Context_t *context, GL_Point_t position, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    point(surface, &clipping_region, position.x, position.y, index);
}

// DDA algorithm, no branches in the inner-loop.
void GL_primitive_line(const GL_Context_t *context, GL_Point_t from, GL_Point_t to, GL_Pixel_t index)
{
    // TODO: implement line clipping.
    const GL_State_t *state = &context->state;
    //const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

#ifdef __DDA__
    int dx = to.x - from.x;
    int dy = to.y - from.y;

    int step = (dx >= dy) ? dx : dy;

    float xin = (float)dx / (float)step;
    float yin = (float)dy / (float)step;

    float x = from.x + 0.5f;
    float y = from.y + 0.5f;
    for (int i = 0; i < step; ++i) {
        GL_Pixel_t *dst = context->vram_rows[(int)y] + (int)x;
        *dst = index;

        x += xin;
        y += yin;
    }
#else
    const int dx = iabs(to.x - from.x);
    const int dy = -iabs(to.y - from.y);

    const int sx = from.x < to.x ? 1 : -1;
    const int sy = from.y < to.y ? surface->width : -surface->width;

    int err = dx + dy;

    GL_Pixel_t *dst = surface->data_rows[from.y] + from.x;
    GL_Pixel_t *eod = surface->data_rows[to.y] + to.x;

    for (;;) {
        *dst = index;

        int e2 = 2 * err;
        if (e2 >= dy) {
            if (dst == eod) {
                break;
            }
            err += dy;
            dst += sx;
        }
        if (e2 <= dx) {
            if (dst == eod) {
                break;
            }
            err += dx;
            dst += sy;
        }
    }
#endif
}

void GL_primitive_hline(const GL_Context_t *context, GL_Point_t origin, size_t w, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    hline(surface, &clipping_region, origin.x, origin.y, w, index);
}

void GL_primitive_vline(const GL_Context_t *context, GL_Point_t origin, size_t h, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    vline(surface, &clipping_region, origin.x, origin.y, h, index);
}

void GL_primitive_filled_rectangle(const GL_Context_t *context, GL_Rectangle_t rectangle, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = rectangle.x,
            .y0 = rectangle.y,
            .x1 = rectangle.x + rectangle.width - 1,
            .y1 = rectangle.y + rectangle.height - 1
        };

    if (drawing_region.x0 < clipping_region.x0) {
        drawing_region.x0 = clipping_region.x0;
    }
    if (drawing_region.y0 < clipping_region.y0) {
        drawing_region.y0 = clipping_region.y0;
    }
    if (drawing_region.x1 > clipping_region.x1) {
        drawing_region.x1 = clipping_region.x1;
    }
    if (drawing_region.y1 > clipping_region.y1) {
        drawing_region.y1 = clipping_region.y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    GL_Pixel_t *dst = surface->data_rows[drawing_region.y0] + drawing_region.x0;

    const int skip = surface->width - width;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            *(dst++) = index;
        }
        dst += skip;
    }
}

// http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
// https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage
// https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
// https://github.com/dpethes/2D-rasterizer/blob/master/rasterizer2d.pas
void GL_primitive_filled_triangle(const GL_Context_t *context, GL_Point_t a, GL_Point_t b, GL_Point_t c, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = imin(imin(a.x, b.x), c.x),
            .y0 = imin(imin(a.y, b.y), c.y),
            .x1 = imax(imax(a.x, b.x), c.x),
            .y1 = imax(imax(a.y, b.y), c.y)
        };

    if (drawing_region.x0 < clipping_region.x0) {
        drawing_region.x0 = clipping_region.x0;
    }
    if (drawing_region.y0 < clipping_region.y0) {
        drawing_region.y0 = clipping_region.y0;
    }
    if (drawing_region.x1 > clipping_region.x1) {
        drawing_region.x1 = clipping_region.x1;
    }
    if (drawing_region.y1 > clipping_region.y1) {
        drawing_region.y1 = clipping_region.y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    if ((b.x - a.x) * (c.y - a.y) > (c.x - a.x) * (b.y - a.y)) { // Ensure CCW winding.
        GL_Point_t t = a;
        a = b;
        b = t;
    }

    int DX12 = a.x - b.x;
    int DX23 = b.x - c.x;
    int DX31 = c.x - a.x;
    int DY12 = a.y - b.y;
    int DY23 = b.y - c.y;
    int DY31 = c.y - a.y;

    int C1 = DY12 * a.x - DX12 * a.y;
    int C2 = DY23 * b.x - DX23 * b.y;
    int C3 = DY31 * c.x - DX31 * c.y;

    if ((DY12 < 0) || ((DY12 == 0) && (DX12 > 0))) { C1 += 1; }
    if ((DY23 < 0) || ((DY23 == 0) && (DX23 > 0))) { C2 += 1; }
    if ((DY31 < 0) || ((DY31 == 0) && (DX31 > 0))) { C3 += 1; }

    int CY1 = C1 + DX12 * drawing_region.y0 - DY12 * drawing_region.x0;
    int CY2 = C2 + DX23 * drawing_region.y0 - DY23 * drawing_region.x0;
    int CY3 = C3 + DX31 * drawing_region.y0 - DY31 * drawing_region.x0;

    GL_Pixel_t *dst = surface->data_rows[drawing_region.y0] + drawing_region.x0;

    const int skip = surface->width;

    for (int y = 0; y <= height; ++y) { // Pinada's edge function is linear, we can cast it...
        int CX1 = CY1;
        int CX2 = CY2;
        int CX3 = CY3;
        int count = 0;
        int eod = 0;
        for (int x = 0; x <= width; ++x) {
            if ((CX1 | CX2 | CX3) > 0) { // Check the sign bit only.
                count += 1;
                eod = x;
            }
            CX1 -= DY12;
            CX2 -= DY23;
            CX3 -= DY31;
        }
        CY1 += DX12;
        CY2 += DX23;
        CY3 += DX31;

        const int offset = eod - count;
        dst += eod;
        for (int i = 0; i < count; ++i) { // Backward copy, simpler math.
            *(dst--) = index;
        }
        dst += skip - offset;
    }
}

// https://www.javatpoint.com/computer-graphics-bresenhams-circle-algorithm
void GL_primitive_filled_circle(const GL_Context_t *context, GL_Point_t center, int radius, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    const int cx = center.x;
    const int cy = center.y;

    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (x <= y) {
        const int length_x = iabs(2 * x) + 1;
        const int length_y = iabs(2 * y) + 1;
        hline(surface, &clipping_region, cx - x, cy - y, length_x, index);
        hline(surface, &clipping_region, cx - y, cy - x, length_y, index);
        hline(surface, &clipping_region, cx - y, cy + x, length_y, index);
        hline(surface, &clipping_region, cx - x, cy + y, length_x, index);

        if (d < 0) {
            d += 4 * x + 6;
            x += 1;
        } else {
            d += 4 * (x - y) + 10;
            x += 1;
            y -= 1;
        }
    }
}

void GL_primitive_circle(const GL_Context_t *context, GL_Point_t center, int radius, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    const int cx = center.x;
    const int cy = center.y;

    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (x <= y) {
        point(surface, &clipping_region, cx + x, cy + y, index);
        point(surface, &clipping_region, cx + y, cy + x, index);
        point(surface, &clipping_region, cx - y, cy + x, index);
        point(surface, &clipping_region, cx - x, cy + y, index);
        point(surface, &clipping_region, cx - x, cy - y, index);
        point(surface, &clipping_region, cx - y, cy - x, index);
        point(surface, &clipping_region, cx + y, cy - x, index);
        point(surface, &clipping_region, cx + x, cy - y, index);

        if (d < 0) {
            d += 4 * x + 6;
            x += 1;
        } else {
            d += 4 * (x - y) + 10;
            x += 1;
            y -= 1;
        }
    }
}

// https://lodev.org/cgtutor/floodfill.html
void GL_context_fill(const GL_Context_t *context, GL_Point_t seed, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Surface_t *surface = state->surface;

    if (seed.x < clipping_region.x0 || seed.x > clipping_region.x1
        || seed.y < clipping_region.y0 || seed.y > clipping_region.y1) {
        return;
    }

    const GL_Pixel_t match = *surface->data_rows[seed.y] + seed.x;
    const GL_Pixel_t replacement = shifting[index];

    GL_Point_t *stack = NULL;
    arrpush(stack, seed);

    const int skip = state->surface->width;

    while (arrlen(stack) > 0) {
        const GL_Point_t position = arrpop(stack);

        int x = position.x;
        int y = position.y;

        GL_Pixel_t *dst = surface->data_rows[y] + x;
        while (x >= clipping_region.x0 && *dst == match) {
            --x;
            --dst;
        }
        ++x;
        ++dst;

        bool above = false;
        bool below = false;

        while (x <= clipping_region.x1 && *dst == match) {
            *dst = replacement;

            const GL_Pixel_t pixel_above = *(dst - skip);
            if (!above && y >= clipping_region.y0 && pixel_above == match) {
                const GL_Point_t p = (GL_Point_t){ .x = x, .y = y - 1 };
                arrpush(stack, p);
                above = true;
            } else
            if (above && y >= clipping_region.y0 && pixel_above != match) {
                above = false;
            }

            const GL_Pixel_t pixel_below = *(dst + skip);
            if (!below && y < clipping_region.y1 && pixel_below == match) {
                const GL_Point_t p = (GL_Point_t){ .x = x, .y = y + 1 };
                arrpush(stack, p);
                below = true;
            } else
            if (below && y < clipping_region.y1 && pixel_below != match) {
                above = false;
            }

            ++x;
            ++dst;
        }
    }

    arrfree(stack);
}
