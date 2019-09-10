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

static bool GL_is_visible(GL_t *gl, GL_Point_t position)
{
    return true;
}

void GL_primitive_point(GL_t *gl, GL_Point_t position, GL_Pixel_t color)
{
    const GL_Context_t *context = &gl->context;

    if (!GL_is_visible(gl, position)) {
        return;
    }

    GL_Pixel_t value = context->shifting[color];

    if (context->transparent[value]) {
        return;
    }

    GL_Pixel_t *dst = (GL_Pixel_t *)gl->surface.data_rows[position.y] + position.x;
    *dst = value;
}

void GL_primitive_line(GL_t *gl, GL_Point_t from, GL_Point_t to, GL_Pixel_t color)
{

}

void GL_primitive_hline(GL_t *gl, GL_Point_t origin, size_t width, GL_Pixel_t color)
{
    if (!GL_is_visible(gl, origin)) {
        return;
    }
    GL_Pixel_t *dst = (GL_Pixel_t *)gl->surface.data_rows[origin.y] + origin.x;
    for (size_t i = 0; i < width; ++i) {
        *(dst++) = color;
    }
}

void GL_primitive_vline(GL_t *gl, GL_Point_t origin, size_t height, GL_Pixel_t color)
{
    if (!GL_is_visible(gl, origin)) {
        return;
    }
    for (size_t i = 0; i < height; ++i) {
        GL_Pixel_t *dst = (GL_Pixel_t *)gl->surface.data_rows[origin.y] + origin.x;
        *dst = color;
    }
}

void GL_primitive_rectangle(GL_t *gl, GL_Rectangle_t rectangle, GL_Pixel_t color)
{

}

void GL_primitive_filled_rectangle(GL_t *gl, GL_Rectangle_t rectangle, GL_Pixel_t color)
{

}

void GL_primitive_circle(GL_t *gl, GL_Point_t center, float radius, GL_Pixel_t color)
{

}

void GL_primitive_filled_circle(GL_t *gl, GL_Point_t center, float radius, GL_Pixel_t color)
{

}
