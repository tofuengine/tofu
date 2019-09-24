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

#include <math.h>

//#define __DDA__
#ifndef __DDA__
static int iabs(int v)
{
    return v >= 0 ? v : -v;
}
#endif

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

// http://www-users.mat.uni.torun.pl/~wrona/3d_tutor/tri_fillers.html
/*
the coordinates of vertices are (A.x,A.y), (B.x,B.y), (C.x,C.y); we assume that A.y<=B.y<=C.y (you should sort them first)
dx1,dx2,dx3 are deltas used in interpolation
horizline draws horizontal segment with coordinates (S.x,Y), (E.x,Y)
S.x, E.x are left and right x-coordinates of the segment we have to draw
S=A means that S.x=A.x; S.y=A.y;

	if (B.y-A.y > 0) dx1=(B.x-A.x)/(B.y-A.y) else dx1=0;
	if (C.y-A.y > 0) dx2=(C.x-A.x)/(C.y-A.y) else dx2=0;
	if (C.y-B.y > 0) dx3=(C.x-B.x)/(C.y-B.y) else dx3=0;

	S=E=A;
	if(dx1 > dx2) {
		for(;S.y<=B.y;S.y++,E.y++,S.x+=dx2,E.x+=dx1)
			horizline(S.x,E.x,S.y,color);
		E=B;
		for(;S.y<=C.y;S.y++,E.y++,S.x+=dx2,E.x+=dx3)
			horizline(S.x,E.x,S.y,color);
	} else {
		for(;S.y<=B.y;S.y++,E.y++,S.x+=dx1,E.x+=dx2)
			horizline(S.x,E.x,S.y,color);
		S=B;
		for(;S.y<=C.y;S.y++,E.y++,S.x+=dx3,E.x+=dx2)
			horizline(S.x,E.x,S.y,color);
	}
*/
static void ensure_ccw(GL_Point_t *a, GL_Point_t *b, GL_Point_t *c)
{
    if ((b->x - a->x) * (c->y - a->y) > (c->x - a->x) * (b->y - a->y)) {
        GL_Point_t t = *a;
        *a = *b;
        *b = t;
    }
}

static int imax(int a, int b)
{
    return a > b ? a : b;
}

static int imin(int a, int b)
{
    return a < b ? a : b;
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage
// https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
// https://github.com/dpethes/2D-rasterizer/blob/master/rasterizer2d.pas
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

    ensure_ccw(&a, &b, &c);

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

    const GL_Color_t color = colors[index];

    GL_Color_t *dst = (GL_Color_t *)context->vram_rows[drawing_region.y0];

    const int skip = context->width;

    for (int y = drawing_region.y0; y <= drawing_region.y1; ++y) {
        int CX1 = CY1;
        int CX2 = CY2;
        int CX3 = CY3;
        int count = 0;
        int last_x = 0;
        for (int x = drawing_region.x0; x <= drawing_region.x1; ++x) {
            if ((CX1 | CX2 | CX3) > 0) {
                count += 1;
                last_x = x;
            }
            CX1 -= DY12;
            CX2 -= DY23;
            CX3 -= DY31;
        }
        CY1 += DX12;
        CY2 += DX23;
        CY3 += DX31;

        const int offset = last_x + 1 - count;
        for (int i = 0; i < count; ++i) {
            dst[offset + i] = color;
        }
        dst += skip;
    }
}
