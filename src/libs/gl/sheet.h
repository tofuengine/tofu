/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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

#ifndef __GL_SHEET_H__
#define __GL_SHEET_H__

#include <stdbool.h>

#include "common.h"
#include "surface.h"

typedef struct _GL_Sheet_t {
    GL_Surface_t *atlas;
    GL_Rectangle_t *cells;
    size_t count;
} GL_Sheet_t;

// TODO: pass a ready to use surface to the sheet? Decoding should be done solely
// TODO: at surface level, it's a refernce... the atlas is not owned by the sheet (or font)

extern GL_Sheet_t *GL_sheet_decode_rect(size_t width, size_t height, const void *pixels, size_t cell_width, size_t cell_height, GL_Surface_Callback_t callback, void *user_data);
extern GL_Sheet_t *GL_sheet_decode(size_t width, size_t height, const void *pixels, const GL_Rectangle_t *cells, size_t count, GL_Surface_Callback_t callback, void *user_data);
extern void GL_sheet_destroy(GL_Sheet_t *sheet);

extern GL_Sheet_t *GL_sheet_attach_rect(const GL_Surface_t *atlas, size_t cell_width, size_t cell_height);
extern GL_Sheet_t *GL_sheet_attach(const GL_Surface_t *atlas, const GL_Rectangle_t *cells, size_t count);
extern void GL_sheet_detach(GL_Sheet_t *sheet);

#endif  /* __GL_SHEET_H__ */