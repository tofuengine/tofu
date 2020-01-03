/*
 * Copyright (c) 2019-2020 by Marco Lizza (marco.lizza@gmail.com)
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

#include <config.h>
#include <libs/imath.h>
#include <libs/gl/gl.h>
#include <libs/stb.h>

#include <math.h>

#define REGION_INSIDE   0
#define REGION_LEFT     1
#define REGION_ABOVE    2
#define REGION_RIGHT    4
#define REGION_BELOW    8

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

    surface->data[y * surface->width + x] = index;
}

// https://sighack.com/post/cohen-sutherland-line-clipping-algorithm
static inline int compute_code(const GL_Quad_t *clipping_region, int x, int y)
{
    int code = REGION_INSIDE;
    if (x < clipping_region->x0) {
        code |= REGION_LEFT;
    } else
    if (x > clipping_region->x1) {
        code |= REGION_RIGHT;
    }
    if (y < clipping_region->y0) {
        code |= REGION_ABOVE;
    } else
    if (y > clipping_region->y1) {
        code |= REGION_BELOW;
    }
    return code;
}

// DDA algorithm, no branches in the inner-loop.
static void line(const GL_Surface_t *surface, const GL_Quad_t *clipping_region, int x0, int y0, int x1, int y1, GL_Pixel_t index)
{
    int code0 = compute_code(clipping_region, x0, y0);
    int code1 = compute_code(clipping_region, x1, y1);

    for (;;) {
        if (!(code0 | code1)) { // bitwise OR is 0: both points inside window; trivially accept and exit loop
            break;
        }  else
        if (code0 & code1) {
            // bitwise AND is not 0: both points share an outside zone (LEFT, RIGHT, ABOVE,
            // or BELOW), so both must be outside window; exit loop bailing out
            return;
        } else {
            // at least one endpoint is outside the clip rectangle; pick it.
            int code = code0 ? code0 : code1;

            // Now find the intersection point;
            // use formulas:
            //
            //   slope = (y1 - y0) / (x1 - x0)
            //   x = x0 + (1 / slope) * (ym - y0), where ym is ymin or ymax
            //   y = y0 + slope * (xm - x0), where xm is xmin or xmax
            //
            // No need to worry about divide-by-zero because, in each case, the
            // outcode bit being tested guarantees the denominator is non-zero
            //
            // Note that we are safe in using integer math (only, the division need to be performed at last).
            int x = 0, y = 0;
            if (code & REGION_ABOVE) {
                y = clipping_region->y0;
                x = (x0 + (x1 - x0) * (y - y0) / (y1 - y0));
            } else
            if (code & REGION_BELOW) {
                y = clipping_region->y1;
                x = (x0 + (x1 - x0) * (y - y0) / (y1 - y0));
            } else
            if (code & REGION_LEFT) {
                x = clipping_region->x0;
                y = (y0 + (y1 - y0) * (x - x0) / (x1 - x0));
            } else
            if (code & REGION_RIGHT) {
                x = clipping_region->x1;
                y = (y0 + (y1 - y0) * (x - x0) / (x1 - x0));
            }

            // Now we move outside point to intersection point to clip and get ready for next pass.
            if (code == code0) {
                code0 = compute_code(clipping_region, x, y);
                x0 = x;
                y0 = y;
            } else {
                code1 = compute_code(clipping_region, x, y);
                x1 = x;
                y1 = y;
            }
        }
    }

#ifdef __DDA__
    GL_Pixel_t *ddata = surface->data;

    const int dwidth = surface->width;

    const int dx = x1 - x0;
    const int dy = y1 - y0;

    const int delta = iabs(dx) >= iabs(dy) ? iabs(dx) : iabs(dy); // Move along the longest delta

    const float xin = (float)dx / (float)delta;
    const float yin = (float)dy / (float)delta;

    float x = x0 + 0.5f;
    float y = y0 + 0.5f;
    for (int i = delta + 1; i; --i) { // One more step, to reach and ending pixel.
        GL_Pixel_t *dptr = ddata + (int)y * dwidth + (int)x;
        *dptr = index;

        x += xin;
        y += yin;
    }
#else
    const int dwidth = surface->width;

    const int dx = iabs(x1 - x0);
    const int dy = -iabs(y1 - y0);

    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? dwidth : -dwidth;

    int err = dx + dy;

    GL_Pixel_t *ddata = surface->data;

    GL_Pixel_t *dptr = ddata + y0 * dwidth + x0;
    GL_Pixel_t *eod = ddata + y1 * dwidth + x1;

    for (;;) {
        *dptr = index;
        if (dptr == eod) {
            break;
        }
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            dptr += sx;
        }
        if (e2 <= dx) {
            err += dx;
            dptr += sy;
        }
    }
#endif
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

    GL_Pixel_t *ddata = surface->data;

    const int dwidth = surface->width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    for (int i = width; i; --i) {
        *(dptr++) = index;
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

    GL_Pixel_t *ddata = surface->data;

    const int dwidth = surface->width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const int dskip = dwidth;

    for (int i = height; i; --i) {
        *dptr = index;
        dptr += dskip;
    }
}

void GL_primitive_point(const GL_Context_t *context, GL_Point_t position, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    point(surface, clipping_region, position.x, position.y, index);
}

void GL_primitive_hline(const GL_Context_t *context, GL_Point_t origin, size_t w, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    hline(surface, clipping_region, origin.x, origin.y, w, index);
}

void GL_primitive_vline(const GL_Context_t *context, GL_Point_t origin, size_t h, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    vline(surface, clipping_region, origin.x, origin.y, h, index);
}

void GL_primitive_polyline(const GL_Context_t *context, const GL_Point_t *vertices, size_t count, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    if (count < 2) {
        return;
    }

    const GL_Point_t *from = vertices;
    for (size_t i = 1; i < count; ++i) {
        const GL_Point_t *to = vertices + i;
        line(surface, clipping_region, from->x, from->y, to->x, to->y, index);
        from = to;
    }
}

void GL_context_process(const GL_Context_t *context, GL_Rectangle_t rectangle)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
    const GL_Surface_t *surface = state->surface;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = rectangle.x,
            .y0 = rectangle.y,
            .x1 = rectangle.x + rectangle.width - 1,
            .y1 = rectangle.y + rectangle.height - 1
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

    const int sdwidth = surface->width;

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

void GL_primitive_filled_rectangle(const GL_Context_t *context, GL_Rectangle_t rectangle, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
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

    GL_Pixel_t *ddata = surface->data;

    const int dwidth = surface->width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const int dskip = dwidth - width;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            *(dptr++) = index;
        }
        dptr += dskip;
    }
}

// http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
// https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage
// https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
// https://github.com/dpethes/2D-rasterizer/blob/master/rasterizer2d.pas
void GL_primitive_filled_triangle(const GL_Context_t *context, GL_Point_t a, GL_Point_t b, GL_Point_t c, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
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

    GL_Pixel_t *ddata = surface->data;

    const int dwidth = surface->width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const int dskip = dwidth;

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
        dptr += eod;
        for (int i = count; i; --i) { // Backward copy, simpler math.
            *(dptr--) = index;
        }
        dptr += dskip - offset;
    }
}

// https://www.javatpoint.com/computer-graphics-bresenhams-circle-algorithm
void GL_primitive_filled_circle(const GL_Context_t *context, GL_Point_t center, int radius, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
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
        hline(surface, clipping_region, cx - x, cy - y, length_x, index);
        hline(surface, clipping_region, cx - y, cy - x, length_y, index);
        hline(surface, clipping_region, cx - y, cy + x, length_y, index);
        hline(surface, clipping_region, cx - x, cy + y, length_x, index);

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
    const GL_Quad_t *clipping_region = &state->clipping_region;
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
        point(surface, clipping_region, cx + x, cy + y, index);
        point(surface, clipping_region, cx + y, cy + x, index);
        point(surface, clipping_region, cx - y, cy + x, index);
        point(surface, clipping_region, cx - x, cy + y, index);
        point(surface, clipping_region, cx - x, cy - y, index);
        point(surface, clipping_region, cx - y, cy - x, index);
        point(surface, clipping_region, cx + y, cy - x, index);
        point(surface, clipping_region, cx + x, cy - y, index);

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
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Surface_t *surface = state->surface;

    if (seed.x < clipping_region->x0 || seed.x > clipping_region->x1
        || seed.y < clipping_region->y0 || seed.y > clipping_region->y1) {
        return;
    }

    GL_Pixel_t *ddata = surface->data;

    const int dwidth = surface->width;

    const GL_Pixel_t match = ddata[seed.y * dwidth + seed.x];
    const GL_Pixel_t replacement = shifting[index];

    GL_Point_t *stack = NULL;
    arrpush(stack, seed);

    const int dskip = state->surface->width;

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
