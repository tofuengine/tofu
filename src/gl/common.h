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

#ifndef __GL_COMMON_H__
#define __GL_COMMON_H__

#include <stddef.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

typedef struct _GL_Point_t {
    GLfloat x, y;
} GL_Point_t;

typedef struct _GL_Size_t {
    GLfloat width, height;
} GL_Size_t;

typedef struct _GL_Rectangle_t {
    GLfloat x, y;
    GLfloat width, height;
} GL_Rectangle_t;

typedef struct _GL_Quad_t {
    GLfloat x0, y0;
    GLfloat x1, y1;
} GL_Quad_t;

#pragma pack(push, 1)
typedef struct _GL_Color_t {
    GLubyte r, g, b, a;
} GL_Color_t;
#pragma pack(pop)

#define GL_DEGREES_OVER_RADIANS 57.295779513082320876798154814105

#endif  /* __GL_COMMON_H__ */