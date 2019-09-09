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

#ifndef __GL_PRIMITIVE_H__
#define __GL_PRIMITIVE_H__

#include <stdbool.h>

#include "common.h"

extern bool GL_primitive_initialize();
extern void GL_primitive_terminate();

extern void GL_primitive_points(const GL_Point_t *points, const size_t count, const GL_Color_t color);
extern void GL_primitive_polyline(const GL_Point_t *points, const size_t count, const GL_Color_t color);
extern void GL_primitive_strip(const GL_Point_t *points, const size_t count, const GL_Color_t color);
extern void GL_primitive_fan(const GL_Point_t *points, const size_t count, const GL_Color_t color);
extern void GL_primitive_cluster(const GL_Point_t *points, const GL_Color_t *colors, const size_t count);


#endif  /* __GL_PRIMITIVE_H__ */