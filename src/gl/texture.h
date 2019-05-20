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

#ifndef __GL_TEXTURE_H__
#define __GL_TEXTURE_H__

#include <stdbool.h>

#include "common.h"

typedef struct _GL_Texture_t {
    GLuint id;
    GLuint width, height;
} GL_Texture_t;

typedef void (*GL_Texture_Callback_t)(void *parameters, void *data, int width, int height);

extern bool GL_create_texture(GL_Texture_t *texture, const char *pathfile, GL_Texture_Callback_t callback, void *parameters);
extern void GL_delete_texture(GL_Texture_t *texture);
extern void GL_draw_texture(const GL_Texture_t *texture, const GL_Rectangle_t source, const GL_Rectangle_t target, const GL_Point_t origin, GLfloat rotation, const GL_Color_t color);

#endif  /* __GL_TEXTURE_H__ */