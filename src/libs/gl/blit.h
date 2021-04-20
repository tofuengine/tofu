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

#ifndef __GL_BLIT_H__
#define __GL_BLIT_H__

#include "common.h"
#include "context.h"
#include "surface.h"

extern void GL_context_blit(const GL_Context_t *context, const GL_Surface_t *source, GL_Rectangle_t area, GL_Point_t position);
extern void GL_context_blit_s(const GL_Context_t *context, const GL_Surface_t *source, GL_Rectangle_t area, GL_Point_t position, float sx, float sy);
extern void GL_context_blit_sr(const GL_Context_t *context, const GL_Surface_t *source, GL_Rectangle_t area, GL_Point_t position, float sx, float sy, int rotation, float ax, float ay);

// TODO: implement stencil blit.
//extern void GL_context_blit_m(const GL_Context_t *context, const GL_Surface_t *source, const GL_Surface_t *stencil, GL_Pixel_t threshold, GL_Rectangle_t area, GL_Point_t position);

#endif  /* __GL_BLIT_H__ */
