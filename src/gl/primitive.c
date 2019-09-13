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
