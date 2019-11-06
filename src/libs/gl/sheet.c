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

#include <config.h>
#include <libs/log.h>
#include <libs/gl/gl.h>

#include <stdlib.h>
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif

GL_Rectangle_t *precompute_cells(size_t width, size_t height, size_t cell_width, size_t cell_height)
{
    size_t columns = width / cell_width;
    size_t rows = height / cell_height;
    size_t amount = columns * rows;
    GL_Rectangle_t *cells = malloc(amount * sizeof(GL_Rectangle_t));
    size_t k = 0;
    for (size_t i = 0; i < rows; ++i) {
        int y = i * cell_height;
        for (size_t j = 0; j < columns; ++j) {
            int x = j * cell_width;
            cells[k++] = (GL_Rectangle_t){
                    .x = x,
                    .y = y,
                    .width = cell_width,
                    .height =cell_height
                };
        }
    }
    return cells;
}

bool GL_sheet_decode(GL_Sheet_t *sheet, const void *buffer, size_t size, size_t cell_width, size_t cell_height, GL_Surface_Callback_t callback, void *parameters)
{
    GL_Surface_t atlas;
    if (!GL_surface_decode(&atlas, buffer, size, callback, parameters)) {
        return false;
    }
    GL_sheet_attach(sheet, &atlas, cell_width, cell_height);
    Log_write(LOG_LEVELS_DEBUG, "<GL> sheet #%p decoded", sheet);
    return true;
}

void GL_sheet_delete(GL_Sheet_t *sheet)
{
    GL_surface_delete(&sheet->atlas); // Delete prior detach or the atlas will be cleared!
    GL_sheet_detach(sheet);
    Log_write(LOG_LEVELS_DEBUG, "<GL> sheet #%p deleted", sheet);
}

void GL_sheet_attach(GL_Sheet_t *sheet, const GL_Surface_t *atlas, size_t cell_width, size_t cell_height)
{
    *sheet = (GL_Sheet_t){
            .atlas = *atlas,
            .cells = precompute_cells(atlas->width, atlas->height, cell_width, cell_height),
            .size = (GL_Size_t){ cell_width, cell_height }
        };
    Log_write(LOG_LEVELS_DEBUG, "<GL> sheet #%p attached", sheet);
}

void GL_sheet_detach(GL_Sheet_t *sheet)
{
    free(sheet->cells);
    *sheet = (GL_Sheet_t){ 0 };
    Log_write(LOG_LEVELS_DEBUG, "<GL> sheet #%p detached", sheet);
}
