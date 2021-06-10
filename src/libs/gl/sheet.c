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

#include "sheet.h"

#include "blit.h"
#include "tile.h"

#include <config.h>
#include <libs/imath.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <math.h>

#define LOG_CONTEXT "gl-sheet"

static GL_Rectangle_t *_parse_cells(const GL_Rectangle_u32_t *rectangles, size_t count)
{
    GL_Rectangle_t *cells = malloc(sizeof(GL_Rectangle_t) * count);
    if (!cells) {
        return NULL;
    }

    for (size_t i = 0; i < count; ++i) {
        cells[i] = (GL_Rectangle_t){
                .x = rectangles[i].x,
                .y = rectangles[i].y,
                .width = rectangles[i].width,
                .height = rectangles[i].height
        };
    }

    return cells;
}

static GL_Rectangle_t *_generate_cells(GL_Size_t atlas_size, GL_Size_t cell_size, size_t *count)
{
    size_t columns = atlas_size.width / cell_size.width;
    size_t rows = atlas_size.height / cell_size.height;
    size_t amount = columns * rows;
    GL_Rectangle_t *cells = malloc(sizeof(GL_Rectangle_t) * amount);
    if (!cells) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d cells", amount);
        return NULL;
    }
    size_t k = 0;
    for (size_t i = 0; i < rows; ++i) {
        size_t y = i * cell_size.height;
        for (size_t j = 0; j < columns; ++j) {
            size_t x = j * cell_size.width;
            cells[k++] = (GL_Rectangle_t){
                    .x = (int)x,
                    .y = (int)y,
                    .width = cell_size.width,
                    .height = cell_size.height
                };
        }
    }
    *count = amount;
    return cells;
}

static GL_Sheet_t *_attach(GL_Surface_t *atlas, GL_Rectangle_t *cells, size_t count)
{
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

GL_Sheet_t *GL_sheet_create_fixed(const GL_Surface_t *atlas, GL_Size_t cell_size)
{
    size_t count;
    GL_Rectangle_t *cells = _generate_cells((GL_Size_t){ .width = atlas->width, .height = atlas->height }, cell_size, &count);
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

GL_Sheet_t *GL_sheet_create(const GL_Surface_t *atlas, const GL_Rectangle_u32_t *rectangles, size_t count)
{
    GL_Rectangle_t *cells = _parse_cells(rectangles, count);
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

void GL_sheet_destroy(GL_Sheet_t *sheet)
{
    _detach(sheet);
}

GL_Size_t GL_sheet_size(const GL_Sheet_t *sheet, size_t cell_id, float scale_x, float scale_y)
{
    const GL_Rectangle_t *cell = &sheet->cells[cell_id];
    return (GL_Size_t){
            .width = (size_t)IROUNDF(cell->width * fabsf(scale_x)),
            .height = (size_t)IROUNDF(cell->height * fabsf(scale_y))
        };
}

void GL_sheet_blit(const GL_Sheet_t *sheet, const GL_Surface_t *surface, GL_Point_t position, size_t cell_id)
{
    GL_surface_blit(surface, position, sheet->atlas, sheet->cells[cell_id]);
}

void GL_sheet_blit_s(const GL_Sheet_t *sheet, const GL_Surface_t *surface, GL_Point_t position, size_t cell_id, float scale_x, float scale_y)
{
    GL_surface_blit_s(surface, position, sheet->atlas, sheet->cells[cell_id], scale_x, scale_y);
}

void GL_sheet_blit_sr(const GL_Sheet_t *sheet, const GL_Surface_t *surface, GL_Point_t position, size_t cell_id, float scale_x, float scale_y, int rotation, float anchor_x, float anchor_y)
{
    GL_surface_blit_sr(surface, position, sheet->atlas, sheet->cells[cell_id], scale_x, scale_y, rotation, anchor_x, anchor_y);
}

void GL_sheet_tile(const GL_Sheet_t *sheet, const GL_Surface_t *surface, GL_Point_t position, size_t cell_id, GL_Point_t offset)
{
    GL_surface_tile(surface, position, sheet->atlas, sheet->cells[cell_id], offset);
}

void GL_sheet_tile_s(const GL_Sheet_t *sheet, const GL_Surface_t *surface, GL_Point_t position, size_t cell_id, GL_Point_t offset, int scale_x, int scale_y)
{
    GL_surface_tile_s(surface, position, sheet->atlas, sheet->cells[cell_id], offset, scale_x, scale_y);
}
