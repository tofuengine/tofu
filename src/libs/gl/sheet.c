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
#include <libs/stb.h>

#define LOG_CONTEXT "gl-sheet"

static GL_Rectangle_t *_clone(const GL_Rectangle_t *cells, size_t count)
{
    GL_Rectangle_t *clone = malloc(sizeof(GL_Rectangle_t) * count);
    if (!clone) {
        return NULL;
    }
#ifdef __NO_MEMSET_MEMCPY__
    for (size_t i = 0; i < count; ++i) {
        clone[i] = cells[i];
    }
#else
    memcpy(clone, cells, sizeof(GL_Rectangle_t) * count);
#endif
    return clone;
}

static GL_Rectangle_t *_precompute_cells(size_t width, size_t height, size_t cell_width, size_t cell_height, size_t *count)
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
        size_t y = i * cell_height;
        for (size_t j = 0; j < columns; ++j) {
            size_t x = j * cell_width;
            cells[k++] = (GL_Rectangle_t){
                    .x = (int)x,
                    .y = (int)y,
                    .width = cell_width,
                    .height = cell_height
                };
        }
    }
    *count = amount;
    return cells;
}

static GL_Sheet_t *_attach(GL_Surface_t *atlas, GL_Rectangle_t *cells, size_t count)
{
    if (!cells || count == 0) {
        return NULL;
    }
    GL_Sheet_t *sheet = malloc(sizeof(GL_Sheet_t));
    if (!sheet) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate sheet");
        return NULL;
    }
    *sheet = (GL_Sheet_t){
            .atlas = atlas,
            .cells = cells,
            .count = count
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached", sheet);
    return sheet;
}

static void _detach(GL_Sheet_t *sheet)
{
    free(sheet->cells);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet cells freed");

    free(sheet);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p freed", sheet);
}

GL_Sheet_t *GL_sheet_create_rect(const GL_Surface_t *atlas, size_t cell_width, size_t cell_height)
{
    size_t count;
    GL_Rectangle_t *cells = _precompute_cells(atlas->width, atlas->height, cell_width, cell_height, &count);
    if (!cells) {
        return NULL;
    }
    GL_Sheet_t *sheet = _attach((GL_Surface_t *)atlas, cells, count);
    if (!sheet) {
        free(cells);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached", sheet);
    return sheet;
}

GL_Sheet_t *GL_sheet_create(const GL_Surface_t *atlas, const GL_Rectangle_t *cells, size_t count)
{
    GL_Sheet_t *sheet = _attach((GL_Surface_t *)atlas, _clone(cells, count), count);
    if (!sheet) {
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached", sheet);
    return sheet;
}

void GL_sheet_destroy(GL_Sheet_t *sheet)
{
    if (!sheet) {
        return;
    }
    _detach(sheet);
}
