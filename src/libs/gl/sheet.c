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

#include "sheet.h"

#include <config.h>
#include <libs/log.h>
#include <libs/gl/gl.h>
#include <libs/stb.h>

#include <stdlib.h>

#define LOG_CONTEXT "sheet"

static GL_Rectangle_t *_precompute_cells(size_t width, size_t height, size_t cell_width, size_t cell_height)
{
    size_t columns = width / cell_width;
    size_t rows = height / cell_height;
    size_t amount = columns * rows;
    GL_Rectangle_t *cells = malloc(amount * sizeof(GL_Rectangle_t));
    if (!cells) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d cells", amount);
        return NULL;
    }
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

static GL_Sheet_t *_attach(GL_Surface_t *atlas, size_t cell_width, size_t cell_height)
{
    GL_Rectangle_t *cells = _precompute_cells(atlas->width, atlas->height, cell_width, cell_height);
    if (!cells) {
        return NULL;
    }
    GL_Sheet_t *sheet = malloc(sizeof(GL_Sheet_t));
    if (!sheet) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate sheet");
        return NULL;
    }
    *sheet = (GL_Sheet_t){
            .atlas = atlas,
            .cells = cells
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached", sheet);
    return sheet;
}

GL_Sheet_t *GL_sheet_decode(const void *buffer, size_t size, size_t cell_width, size_t cell_height, GL_Surface_Callback_t callback, void *user_data)
{
    GL_Surface_t *atlas = GL_surface_decode(buffer, size, callback, user_data);
    if (!atlas) {
        return NULL;
    }
    GL_Sheet_t *sheet = _attach(atlas, cell_width, cell_height);
    if (!sheet) {
        GL_surface_destroy(atlas);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p decoded", sheet);
    return sheet;
}

void GL_sheet_destroy(GL_Sheet_t *sheet)
{
    GL_surface_destroy(sheet->atlas); // Delete prior detach or the atlas will be cleared!
    GL_sheet_detach(sheet);
}

GL_Sheet_t *GL_sheet_attach(const GL_Surface_t *surface, size_t cell_width, size_t cell_height)
{
    GL_Sheet_t *sheet = _attach((GL_Surface_t *)surface, cell_width, cell_height);
    if (!sheet) {
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached", sheet);
    return sheet;
}

void GL_sheet_detach(GL_Sheet_t *sheet)
{
    free(sheet->cells);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet cells freed");
    free(sheet);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p freed", sheet);
}
