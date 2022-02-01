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

#ifndef __GL_SHEET_H__
#define __GL_SHEET_H__

#include <stdbool.h>

#include "common.h"
#include "context.h"
#include "surface.h"

typedef struct GL_Sheet_s {
    const GL_Surface_t *atlas;

    GL_Rectangle_t *cells;
    size_t count;
} GL_Sheet_t;

extern GL_Sheet_t *GL_sheet_create_fixed(const GL_Surface_t *atlas, GL_Size_t cell_size);
extern GL_Sheet_t *GL_sheet_create(const GL_Surface_t *atlas, const GL_Rectangle_u32_t *cells, size_t count);
extern void GL_sheet_destroy(GL_Sheet_t *sheet);

extern GL_Size_t GL_sheet_size(const GL_Sheet_t *sheet, size_t cell_id, float scale_x, float scale_y);

extern void GL_sheet_blit(const GL_Sheet_t *sheet, const GL_Context_t *context, GL_Point_t position, size_t cell_id);
extern void GL_sheet_blit_s(const GL_Sheet_t *sheet, const GL_Context_t *context, GL_Point_t position, size_t cell_id, float scale_x, float scale_y);
extern void GL_sheet_blit_sr(const GL_Sheet_t *sheet, const GL_Context_t *context, GL_Point_t position, size_t cell_id, float scale_x, float scale_y, int rotation, float anchor_x, float anchor_y);

extern void GL_sheet_tile(const GL_Sheet_t *sheet, const GL_Context_t *context, GL_Point_t position, size_t cell_id, GL_Point_t offset);
extern void GL_sheet_tile_s(const GL_Sheet_t *sheet, const GL_Context_t *context, GL_Point_t position, size_t cell_id, GL_Point_t offset, int scale_x, int scale_y);

#endif  /* __GL_SHEET_H__ */
