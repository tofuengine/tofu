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

typedef void (*GL_Texture_Callback_t)(void *parameters, void *data, size_t width, size_t height);

extern bool GL_texture_load(GL_Texture_t *texture, const char *pathfile, const GL_Texture_Callback_t callback, void *parameters);
extern bool GL_texture_decode(GL_Texture_t *texture, const void *buffer, const size_t size, const GL_Texture_Callback_t callback, void *parameters);
extern void GL_texture_create(GL_Texture_t *texture, const size_t width, const size_t height, const void *data);
extern void GL_texture_delete(GL_Texture_t *texture);
extern void GL_texture_blit(const GL_Texture_t *texture, const GL_Quad_t source, const GL_Quad_t destination, GLfloat rotation, const GL_Color_t color);
extern void GL_texture_blit_fast(const GL_Texture_t *texture, const GL_Quad_t source, const GL_Quad_t destination, const GL_Color_t color);

#endif  /* __GL_TEXTURE_H__ */