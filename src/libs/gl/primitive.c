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

#include "primitive.h"

#include <config.h>
#include <libs/imath.h>

#define REGION_INSIDE   0
#define REGION_LEFT     1
#define REGION_ABOVE    2
#define REGION_RIGHT    4
#define REGION_BELOW    8

static void _point(const GL_Surface_t *surface, const GL_Quad_t *clipping_region, int x, int y, GL_Pixel_t index)
{
    if (x < clipping_region->x0) {
        return;
    } else
    if (x >= clipping_region->x1) {
        return;
    }
    if (y < clipping_region->y0) {
        return;
    } else
    if (y >= clipping_region->y1) {
        return;
    }

    surface->data[y * surface->width + x] = index;
}

// https://sighack.com/post/cohen-sutherland-line-clipping-algorithm
static inline int _compute_code(const GL_Quad_t *clipping_region, int x, int y)
{
    int code = REGION_INSIDE;
    if (x < clipping_region->x0) {
        code |= REGION_LEFT;
    } else
    if (x >= clipping_region->x1) {
        code |= REGION_RIGHT;
    }
    if (y < clipping_region->y0) {
        code |= REGION_ABOVE;
    } else
    if (y >= clipping_region->y1) {
        code |= REGION_BELOW;
    }
    return code;
}

// DDA algorithm, no branches in the inner-loop.
static void _line(const GL_Surface_t *surface, const GL_Quad_t *clipping_region, int x0, int y0, int x1, int y1, GL_Pixel_t index)
{
    int code0 = _compute_code(clipping_region, x0, y0);
    int code1 = _compute_code(clipping_region, x1, y1);

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
                y = clipping_region->y1 - 1;
                x = (x0 + (x1 - x0) * (y - y0) / (y1 - y0));
            } else
            if (code & REGION_LEFT) {
                x = clipping_region->x0;
                y = (y0 + (y1 - y0) * (x - x0) / (x1 - x0));
            } else
            if (code & REGION_RIGHT) {
                x = clipping_region->x1 - 1;
                y = (y0 + (y1 - y0) * (x - x0) / (x1 - x0));
            }

            // Now we move outside point to intersection point to clip and get ready for next pass.
            if (code == code0) {
                code0 = _compute_code(clipping_region, x, y);
                x0 = x;
                y0 = y;
            } else {
                code1 = _compute_code(clipping_region, x, y);
                x1 = x;
                y1 = y;
            }
        }
    }

#ifndef __NON_DDA_LINES__
    GL_Pixel_t *ddata = surface->data;

    const int dwidth = (int)surface->width;

    const int dx = x1 - x0;
    const int dy = y1 - y0;

    const int delta = iabs(dx) >= iabs(dy) ? iabs(dx) : iabs(dy); // Move along the longest delta

    const float xin = (float)dx / (float)delta;
    const float yin = (float)dy / (float)delta;

    float x = (float)x0 + 0.5f;
    float y = (float)y0 + 0.5f;
    for (int i = delta + 1; i; --i) { // One more step, to reach and ending pixel.
        GL_Pixel_t *dptr = ddata + (int)y * dwidth + (int)x;
        *dptr = index;

        x += xin;
        y += yin;
    }
#else
    const int dwidth = (int)surface->width;

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

#if 0
static void _texture_line(const GL_Context_t *context, const GL_Quad_t *clipping_region, int x0, int y0, int x1, int y1,
                          const GL_Surface_t *texture, int sx0, int sy0, int sx1, int sy1)
{
    GL_Pixel_t *ddata = surface->data;
    const GL_Pixel_t *sdata = texture->data;

    const int dwidth = (int)surface->width;
    const int swidth = (int)texture->width;

    const int dx = x1 - x0;
    const int dy = y1 - y0;

    const int sdx = sx1 - sx0;
    const int sdy = sy1 - sy0;

    const int delta = iabs(dx) >= iabs(dy) ? iabs(dx) : iabs(dy); // Move along the longest delta

    const float xin = (float)dx / (float)delta;
    const float yin = (float)dy / (float)delta;

    const float sxin = (float)sdx / (float)delta;
    const float syin = (float)sdy / (float)delta;

    float x = x0 + 0.5f;
    float y = y0 + 0.5f;
    float sx = sx0 + 0.5f;
    float sy = sy0 + 0.5f;
    for (int i = delta + 1; i; --i) { // One more step, to reach and ending pixel.
        const GL_Pixel_t *sptr = sdata + (int)sy * swidth + (int)sx;
        GL_Pixel_t *dptr = ddata + (int)y * dwidth + (int)x;
        *dptr = *sptr;

        x += xin;
        y += yin;
        sx += sxin;
        sy += syin;
    }
}
#endif

static void _hline(const GL_Surface_t *surface, const GL_Quad_t *clipping_region, int x, int y, size_t length, GL_Pixel_t index)
{
    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = x,
            .y0 = y,
            .x1 = x + (int)length,
            .y1 = y + 1
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

    const int dwidth = (int)surface->width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    for (int i = width; i; --i) {
        *(dptr++) = index;
    }
}

static void _vline(const GL_Surface_t *surface, const GL_Quad_t *clipping_region, int x, int y, size_t length, GL_Pixel_t index)
{
    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = x,
            .y0 = y,
            .x1 = x + 1,
            .y1 = y + (int)length
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

    const int dwidth = (int)surface->width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const int dskip = dwidth;

    for (int i = height; i; --i) {
        *dptr = index;
        dptr += dskip;
    }
}

void GL_context_point(const GL_Context_t *context, GL_Point_t position, GL_Pixel_t index)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    _point(surface, clipping_region, position.x, position.y, index);
}

void GL_context_hline(const GL_Context_t *context, GL_Point_t origin, size_t w, GL_Pixel_t index)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    _hline(surface, clipping_region, origin.x, origin.y, w, index);
}

void GL_context_vline(const GL_Context_t *context, GL_Point_t origin, size_t h, GL_Pixel_t index)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    _vline(surface, clipping_region, origin.x, origin.y, h, index);
}

void GL_context_polyline(const GL_Context_t *context, const GL_Point_t *vertices, size_t count, GL_Pixel_t index)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

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
        _line(surface, clipping_region, from->x, from->y, to->x, to->y, index);
        from = to;
    }
}

void GL_context_filled_rectangle(const GL_Context_t *context, GL_Rectangle_t rectangle, GL_Pixel_t index)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = rectangle.x,
            .y0 = rectangle.y,
            .x1 = rectangle.x + (int)rectangle.width,
            .y1 = rectangle.y + (int)rectangle.height
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

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            *(dptr++) = index;
        }
        dptr += dskip;
    }
}

// http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
// https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage
// https://github.com/dpethes/2D-rasterizer/blob/master/rasterizer2d.pas
// https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
// https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
void GL_context_filled_triangle(const GL_Context_t *context, GL_Point_t v0, GL_Point_t v1, GL_Point_t v2, GL_Pixel_t index)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = imin(imin(v0.x, v1.x), v2.x),
            .y0 = imin(imin(v0.y, v1.y), v2.y),
            .x1 = imax(imax(v0.x, v1.x), v2.x) + 1,
            .y1 = imax(imax(v0.y, v1.y), v2.y) + 1
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
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out! (can be negative due to clipping region)
        return;
    }

#ifdef __GL_FIX_WINDING__
#ifdef __GL_CLOCKWISE_WINDING__
    if ((v1.x - v0.x) * (v2.y - v0.y) < (v2.x - v0.x) * (v1.y - v0.y)) { // Ensure CW winding.
#else
    if ((v1.x - v0.x) * (v2.y - v0.y) > (v2.x - v0.x) * (v1.y - v0.y)) { // Ensure CCW winding.
#endif
        const GL_Point_t t = v1;
        v1 = v2;
        v2 = t;
    }
#endif  /* __GL_FIX_WINDING__ */

    // Fast edge-function calculation, using a DDA method.
    // Note: swap `v1` and `v2` in the formulas to change winding.
#ifdef __GL_CLOCKWISE_WINDING__
    const int DW0X = v1.y - v2.y, DW0Y = v2.x - v1.x;
    const int DW1X = v2.y - v0.y, DW1Y = v0.x - v2.x;
    const int DW2X = v0.y - v1.y, DW2Y = v1.x - v0.x;
#else
    const int DW0X = v2.y - v1.y, DW0Y = v1.x - v2.x;
    const int DW1X = v0.y - v2.y, DW1Y = v2.x - v0.x;
    const int DW2X = v1.y - v0.y, DW2Y = v0.x - v1.x;
#endif

    const GL_Point_t p = (GL_Point_t){ .x = drawing_region.x0, .y = drawing_region.y0 };

    int w0_row = DW0Y * (p.y - v1.y) + DW0X * (p.x - v1.x); // Reuse and simplify `orient2d()`.
    int w1_row = DW1Y * (p.y - v2.y) + DW1X * (p.x - v2.x);
    int w2_row = DW2Y * (p.y - v0.y) + DW2X * (p.x - v0.x);

    GL_Pixel_t *ddata = surface->data;

    const size_t dwidth = surface->width;

    const size_t dskip = dwidth - width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    for (int y = 0; y < height; ++y) { // Pinada's edge function is linear, we can cast it...
        int w0 = w0_row;
        int w1 = w1_row;
        int w2 = w2_row;

        for (int x = 0; x < width; ++x) {
            if ((w0 | w1 | w2) >= 0) { // Check the sign bit only.
                *dptr = index;
            }
            ++dptr;

            w0 += DW0X;
            w1 += DW1X;
            w2 += DW2X;
        }
        dptr += dskip;

        w0_row += DW0Y;
        w1_row += DW1Y;
        w2_row += DW2Y;
    }
}

// https://www.javatpoint.com/computer-graphics-bresenhams-circle-algorithm
void GL_context_filled_circle(const GL_Context_t *context, GL_Point_t center, size_t radius, GL_Pixel_t index)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    const int cx = center.x;
    const int cy = center.y;

    int x = 0;
    int y = (int)radius;
    int d = 3 - 2 * (int)radius;

    while (x <= y) {
        const int length_x = iabs(2 * x) + 1;
        const int length_y = iabs(2 * y) + 1;
        _hline(surface, clipping_region, cx - x, cy - y, length_x, index);
        _hline(surface, clipping_region, cx - y, cy - x, length_y, index);
        _hline(surface, clipping_region, cx - y, cy + x, length_y, index);
        _hline(surface, clipping_region, cx - x, cy + y, length_x, index);

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

void GL_context_circle(const GL_Context_t *context, GL_Point_t center, size_t radius, GL_Pixel_t index)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    index = shifting[index];

    if (transparent[index]) {
        return;
    }

    const int cx = center.x;
    const int cy = center.y;

    int x = 0;
    int y = (int)radius;
    int d = 3 - 2 * (int)radius;

    while (x <= y) {
        _point(surface, clipping_region, cx + x, cy + y, index);
        _point(surface, clipping_region, cx + y, cy + x, index);
        _point(surface, clipping_region, cx - y, cy + x, index);
        _point(surface, clipping_region, cx - x, cy + y, index);
        _point(surface, clipping_region, cx - x, cy - y, index);
        _point(surface, clipping_region, cx - y, cy - x, index);
        _point(surface, clipping_region, cx + y, cy - x, index);
        _point(surface, clipping_region, cx + x, cy - y, index);

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
