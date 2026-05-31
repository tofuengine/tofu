/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2026 Marco Lizza
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

#include <core/config.h>
#include <libs/bytes.h>
#if defined(TOFU_GRAPHICS_BLIT_FAST_PATH)
#include <libs/fmath.h>
#endif  /* defined(TOFU_GRAPHICS_BLIT_FAST_PATH) */
#include <libs/imath.h>
#define _LOG_TAG "gl-sheet"
#include <libs/log.h>
#include <libs/stb.h>
#if defined(TOFU_GRAPHICS_BLIT_FAST_PATH)
#include <libs/sincos.h>
#endif  /* defined(TOFU_GRAPHICS_BLIT_FAST_PATH) */

#include <math.h>

static GL_Rectangle_t *_parse_cells(const GL_Rectangle32_t *rectangles, size_t count)
{
    GL_Rectangle_t *cells = malloc(sizeof(GL_Rectangle_t) * count);
    if (!cells) {
        return NULL;
    }

    for (size_t i = 0; i < count; ++i) {
        cells[i] = (GL_Rectangle_t){
                .x = bytes_i32le(rectangles[i].x),
                .y = bytes_i32le(rectangles[i].y),
                .width = bytes_ui32le(rectangles[i].width),
                .height = bytes_ui32le(rectangles[i].height)
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
        LOG_E("can't allocate %d cells", amount);
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

static GL_Sheet_t *_allocate(const GL_Surface_t *atlas, GL_Rectangle_t *cells, size_t count)
{
    GL_Sheet_t *sheet = malloc(sizeof(GL_Sheet_t));
    if (!sheet) {
        LOG_E("can't allocate sheet for atlas %p", atlas);
        return NULL;
    }

    *sheet = (GL_Sheet_t){
            .atlas = atlas,
            .cells = cells,
            .count = count
        };

    return sheet;
}

GL_Sheet_t *GL_sheet_create_fixed(const GL_Surface_t *atlas, GL_Size_t cell_size)
{
    size_t count;
    GL_Rectangle_t *cells = _generate_cells((GL_Size_t){ .width = atlas->width, .height = atlas->height }, cell_size, &count);
    if (!cells) {
        goto error_exit;
    }

    GL_Sheet_t *sheet = _allocate(atlas, cells, count);
    if (!sheet) {
        goto error_free_cells;
    }

    LOG_D("sheet %p created (fixed) for atlas %p", sheet, atlas);

    return sheet;

error_free_cells:
    free(cells);
error_exit:
    return NULL;
}

GL_Sheet_t *GL_sheet_create(const GL_Surface_t *atlas, const GL_Rectangle32_t *rectangles, size_t count)
{
    GL_Rectangle_t *cells = _parse_cells(rectangles, count);
    if (!cells) {
        goto error_exit;
    }

    GL_Sheet_t *sheet = _allocate(atlas, cells, count);
    if (!sheet) {
        goto error_free_cells;
    }

    LOG_D("sheet %p created for atlas %p", sheet, atlas);

    return sheet;

error_free_cells:
    free(cells);
error_exit:
    return NULL;
}

void GL_sheet_destroy(GL_Sheet_t *sheet)
{
    LOG_D("destroying sheet %p", sheet);

    free(sheet->cells);
    LOG_D("sheet cells %p freed", sheet->cells);

    free(sheet);
    LOG_D("sheet freed");
}

GL_Size_t GL_sheet_size(const GL_Sheet_t *sheet, size_t cell_id, float scale_x, float scale_y)
{
    const GL_Rectangle_t *cell = &sheet->cells[cell_id];
    return (GL_Size_t){
            .width = (size_t)ITRUNC(cell->width * fabsf(scale_x)),
            .height = (size_t)ITRUNC(cell->height * fabsf(scale_y))
        };
}

// Note:
//   - Non-rotated blits use top-left destination coordinates.
//   - Rotated blits use anchor/pivot destination coordinates.
void GL_sheet_blit(const GL_Sheet_t *sheet, const GL_Context_t *context, GL_Point_t position, size_t cell_id)
{
    GL_context_blit(context, position, sheet->atlas, sheet->cells[cell_id]);
}

void GL_sheet_blit_s(const GL_Sheet_t *sheet, const GL_Context_t *context, GL_Point_t position, size_t cell_id, float scale_x, float scale_y)
{
    const GL_Surface_t *atlas = sheet->atlas;
    const GL_Rectangle_t cell = sheet->cells[cell_id];

#if defined(TOFU_GRAPHICS_BLIT_FAST_PATH)
    if (scale_x == 1.0f && scale_y == 1.0f) {
        GL_context_blit(context, position, atlas, cell);
    } else {
        GL_context_blit_s(context, position, atlas, cell, scale_x, scale_y);
    }
#else   /* defined(TOFU_GRAPHICS_BLIT_FAST_PATH) */
    GL_context_blit_s(context, position, atlas, cell, scale_x, scale_y);
#endif  /* defined(TOFU_GRAPHICS_BLIT_FAST_PATH) */
}

// TODO: Validate the pivot-to-top-left rounding with anchors like 0, 0.5, 1, odd/even sprite sizes, and negative scale.
void GL_sheet_blit_sr(const GL_Sheet_t *sheet, const GL_Context_t *context, GL_Point_t position, size_t cell_id, float scale_x, float scale_y, int rotation, float anchor_x, float anchor_y)
{
    const GL_Surface_t *atlas = sheet->atlas;
    const GL_Rectangle_t cell = sheet->cells[cell_id];

#if defined(TOFU_GRAPHICS_BLIT_FAST_PATH)
    // Note: fractional-scale zero-rotation fast path can still differ slightly
    //       from the general case due to the pivot-to-top-left rounding, but
    //       it's a good trade-off for the performance boost.
    if (rotation % SINCOS_PERIOD == 0) {
        const int dw = ITRUNC(cell.width * FABS(scale_x));
        const int dh = ITRUNC(cell.height * FABS(scale_y));

        const float left = (float)position.x + 0.5f - (float)dw * anchor_x;
        const float top  = (float)position.y + 0.5f - (float)dh * anchor_y;

        GL_Point_t left_top = { .x = IFLOORF(left), .y = IFLOORF(top) };

        if (scale_x == 1.0f && scale_y == 1.0f) {
            GL_context_blit(context, left_top, atlas, cell);
        } else {
            GL_context_blit_s(context, left_top, atlas, cell, scale_x, scale_y);
        }
    } else {
        GL_context_blit_sr(context, position, atlas, cell, scale_x, scale_y, rotation, anchor_x, anchor_y);
    }
#else   /* defined(TOFU_GRAPHICS_BLIT_FAST_PATH) */
    GL_context_blit_sr(context, position, atlas, cell, scale_x, scale_y, rotation, anchor_x, anchor_y);
#endif  /* defined(TOFU_GRAPHICS_BLIT_FAST_PATH) */
}

void GL_sheet_tile(const GL_Sheet_t *sheet, const GL_Context_t *context, GL_Point_t position, size_t cell_id, GL_Point_t offset)
{
    GL_context_tile(context, position, sheet->atlas, sheet->cells[cell_id], offset);
}

void GL_sheet_tile_s(const GL_Sheet_t *sheet, const GL_Context_t *context, GL_Point_t position, size_t cell_id, GL_Point_t offset, int scale_x, int scale_y)
{
    GL_context_tile_s(context, position, sheet->atlas, sheet->cells[cell_id], offset, scale_x, scale_y);
}
