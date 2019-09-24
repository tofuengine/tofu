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

#include "gl.h"

//#define __DDA__
#ifndef __DDA__
static int iabs(int v)
{
    return v >= 0 ? v : -v;
}
#endif

static int imin(int a, int b)
{
    return a < b ? a : b;
}

static int imax(int a, int b)
{
    return a > b ? a : b;
}

static bool GL_is_visible(const GL_Context_t *context, GL_Point_t position)
{
    return true;
}

void GL_primitive_point(const GL_Context_t *context, GL_Point_t position, GL_Pixel_t index)
{
    if (!GL_is_visible(context, position)) {
        return;
    }

    index = context->shifting[index];

    if (context->transparent[index]) {
        return;
    }

    const GL_Color_t color = context->palette.colors[index];

    GL_Color_t *dst = (GL_Color_t *)context->vram_rows[position.y] + position.x;
    *dst = color;
}

// DDA algorithm, no branches in the inner-loop.
void GL_primitive_line(const GL_Context_t *context, GL_Point_t from, GL_Point_t to, GL_Pixel_t index)
{
    index = context->shifting[index];

    if (context->transparent[index]) {
        return;
    }

    const GL_Color_t color = context->palette.colors[index];

#ifdef __DDA__
    int dx = to.x - from.x;
    int dy = to.y - from.y;

    int step = (dx >= dy) ? dx : dy;

    float xin = (float)dx / (float)step;
    float yin = (float)dy / (float)step;

    float x = from.x + 0.5f;
    float y = from.y + 0.5f;
    for (int i = 0; i < step; ++i) {
        GL_Color_t *dst = (GL_Color_t *)context->vram_rows[(int)y] + (int)x;
        *dst = color;

        x += xin;
        y += yin;
    }
#else
    const int dx = iabs(to.x - from.x);
    const int dy = -iabs(to.y - from.y);

    const int sx = from.x < to.x ? 1 : -1;
    const int sy = from.y < to.y ? context->width : -context->width;

    int err = dx + dy;

    GL_Color_t *dst = (GL_Color_t *)context->vram_rows[from.y] + from.x;
    GL_Color_t *eod = (GL_Color_t *)context->vram_rows[to.y] + to.x;

    for (;;) {
        *dst = color;

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

void GL_primitive_hline(const GL_Context_t *context, GL_Point_t origin, size_t width, GL_Pixel_t index)
{
    if (!GL_is_visible(context, origin)) {
        return;
    }

    index = context->shifting[index];

    if (context->transparent[index]) {
        return;
    }

    const GL_Color_t color = context->palette.colors[index];

    GL_Color_t *dst = (GL_Color_t *)context->vram_rows[origin.y] + origin.x;
    for (size_t i = 0; i < width; ++i) {
        *(dst++) = color;
    }
}

void GL_primitive_vline(const GL_Context_t *context, GL_Point_t origin, size_t height, GL_Pixel_t index)
{
    if (!GL_is_visible(context, origin)) {
        return;
    }

    index = context->shifting[index];

    if (context->transparent[index]) {
        return;
    }

    const GL_Color_t color = context->palette.colors[index];

    for (size_t i = 0; i < height; ++i) {
        GL_Color_t *dst = (GL_Color_t *)context->vram_rows[origin.y + i] + origin.x;
        *(dst++) = color;
    }
}

// http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
#define SWAP(a, b)      { GL_Point_t t = (a); (a) = (b); (b) = t; }
#define GL_ROUND(x)     (int)((x) + 0.5f)

static void bottom_flat_triangle(const GL_Context_t *context, GL_Quad_t region, GL_Point_t a, GL_Point_t b, GL_Point_t c, GL_Color_t color)
{
    const float dsx = (float)(b.x - a.x) / (float)(b.y - a.y);
    const float dex = (float)(c.x - a.x) / (float)(c.y - a.y);

    float sx = (float)a.x + 0.5f;
    float ex = sx;

    GL_Color_t *dst = (GL_Color_t *)context->vram_rows[a.y];
    for (int y = a.y; y <= b.y; ++y) {
        int ax = GL_ROUND(sx);
        int bx = GL_ROUND(ex);
        if (ax > bx) {
            int t = ax; ax = bx; bx = t;
        }
        for (int x = ax; x <= bx; ++x) {
            dst[x] = color;
        }
        dst += context->width;
        sx += dsx;
        ex += dex;
    }
}

static void top_flat_triangle(const GL_Context_t *context, GL_Quad_t region, GL_Point_t a, GL_Point_t b, GL_Point_t c, GL_Color_t color)
{
    const float dsx = (float)(c.x - a.x) / (float)(c.y - a.y);
    const float dex = (float)(c.x - b.x) / (float)(c.y - b.y);

    float sx = (float)c.x + 0.5f;
    float ex = sx;

    GL_Color_t *dst = (GL_Color_t *)context->vram_rows[c.y];
    for (int y = c.y; y > a.y; --y) {
        int ax = GL_ROUND(sx);
        int bx = GL_ROUND(ex);
        if (ax > bx) {
            int t = ax; ax = bx; bx = t;
        }
        for (int x = ax; x <= bx; ++x) {
            dst[x] = color;
        }
        dst -= context->width;
        sx -= dsx;
        ex -= dex;
    }
}

void GL_primitive_filled_triangle(const GL_Context_t *context, GL_Point_t a, GL_Point_t b, GL_Point_t c, GL_Pixel_t index)
{
    const GL_Quad_t clipping_region = context->clipping_region;
    const GL_Pixel_t *shifting = context->shifting;
    const GL_Bool_t *transparent = context->transparent;
    const GL_Color_t *colors = context->palette.colors;

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

    const GL_Color_t color = colors[index];

    // sort a, b, c by increasing y coordinates.
    if (a.y > b.y) {
        SWAP(a, b);
    }
    if (a.y > c.y) {
        SWAP(a, c);
    }
    if (b.y > c.y) {
        SWAP(b, c);
    }

    if (b.y == c.y) { // Bottom flat.
        bottom_flat_triangle(context, drawing_region, a, b, c, color);
    } else
    if (a.y == b.y) { // Top flat.
        top_flat_triangle(context, drawing_region, a, b, c, color);
    } else {
        GL_Point_t d = (GL_Point_t){
                .x = (int)(a.x + ((float)(b.y - a.y) / (float)(c.y - a.y)) * (c.x - a.x)),
                .y = b.y
            };
        bottom_flat_triangle(context, drawing_region, a, b, d, color);
        top_flat_triangle(context, drawing_region, b, d, c, color);
    }
}
