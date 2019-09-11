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

#include "sheet.h"

#include "gl.h"

#include "../log.h"

#include <stdlib.h>

GL_Rectangle_t *precompute_cells(size_t width, size_t height, size_t cell_width, size_t cell_height)
{
    size_t columns = width / cell_width;
    size_t rows = height / cell_height;
    size_t amount = columns * rows;
    GL_Rectangle_t *offsets = malloc(amount * sizeof(GL_Rectangle_t));
    size_t k = 0;
    for (size_t i = 0; i < rows; ++i) {
        int32_t y = i * cell_height;
        for (size_t j = 0; j < columns; ++j) {
            int32_t x = j * cell_width;
            offsets[k++] = (GL_Rectangle_t){
                    .x = x,
                    .y = y,
                    .width = cell_width,
                    .height = cell_height
                };
        }
    }
    return offsets;
}

bool GL_sheet_load(GL_Sheet_t *sheet, const char *pathfile, size_t cell_width, size_t cell_height, GL_Surface_Callback_t callback, void *parameters)
{
    GL_Surface_t atlas;
    if (!GL_surface_load(&atlas, pathfile, callback, parameters)) {
        return false;
    }

    *sheet = (GL_Sheet_t){
            .atlas = atlas,
            .cells = precompute_cells(atlas.width, atlas.height, cell_width, cell_height),
            .size = (GL_Size_t){ cell_width, cell_height }
        };
    Log_write(LOG_LEVELS_DEBUG, "<GL> sheet #%p created", sheet);

    return true;
}

bool GL_sheet_decode(GL_Sheet_t *sheet, const void *buffer, size_t size, size_t cell_width, size_t cell_height, GL_Surface_Callback_t callback, void *parameters)
{
    GL_Surface_t atlas;
    if (!GL_surface_decode(&atlas, buffer, size, callback, parameters)) {
        return false;
    }

    *sheet = (GL_Sheet_t){
            .atlas = atlas,
            .cells = precompute_cells(atlas.width, atlas.height, cell_width, cell_height),
            .size = (GL_Size_t){ cell_width, cell_height }
        };
    Log_write(LOG_LEVELS_DEBUG, "<GL> sheet #%p created", sheet);

    return true;
}

void GL_sheet_delete(GL_Sheet_t *sheet)
{
    free(sheet->cells);
    GL_surface_delete(&sheet->atlas);
    Log_write(LOG_LEVELS_DEBUG, "<GL> sheet #%p deleted", sheet);
    *sheet = (GL_Sheet_t){};
}

void GL_sheet_blit(const GL_Context_t *context, const GL_Sheet_t *sheet, size_t cell_id, GL_Point_t position)
{
    GL_context_blit(context, &sheet->atlas, sheet->cells[cell_id], position);
}

void GL_sheet_blit_s(const GL_Context_t *context, const GL_Sheet_t *sheet, size_t cell_id, GL_Point_t position, float sx, float sy)
{
    GL_context_blit_s(context, &sheet->atlas, sheet->cells[cell_id], position, sx, sy);
}

void GL_sheet_blit_r(const GL_Context_t *context, const GL_Sheet_t *sheet, size_t cell_id, GL_Point_t position, float rotation)
{
    GL_context_blit_r(context, &sheet->atlas, sheet->cells[cell_id], position, rotation);
}

void GL_sheet_blit_sr(const GL_Context_t *context, const GL_Sheet_t *sheet, size_t cell_id, GL_Point_t position, float sx, float sy, float rotation)
{
    GL_context_blit_sr(context, &sheet->atlas, sheet->cells[cell_id], position, sx, sy, rotation);
}
