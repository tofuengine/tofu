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

#ifndef __GL_SHEET_H__
#define __GL_SHEET_H__

#include <stdbool.h>

#include "common.h"
#include "surface.h"

typedef struct _GL_Sheet_t {
    GL_Surface_t atlas;
    GL_Rectangle_t *cells;
    GL_Size_t size;
} GL_Sheet_t;

// TODO: is the GL_Sheet_t really needed?

extern bool GL_sheet_load(GL_Sheet_t *sheet, const char *pathfile, size_t cell_width, size_t cell_height, GL_Surface_Callback_t callback, void *parameters);
extern bool GL_sheet_decode(GL_Sheet_t *sheet, const void *buffer, size_t size, size_t cell_width, size_t cell_height, GL_Surface_Callback_t callback, void *parameters);
extern void GL_sheet_delete(GL_Sheet_t *sheet);

#endif  /* __GL_SHEET_H__ */