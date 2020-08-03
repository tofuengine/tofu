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

#include "batch.h"

#include "blit.h"

#include <config.h>
#include <libs/log.h>
#include <libs/stb.h>

#define LOG_CONTEXT "gl-batch"

GL_Batch_t *GL_batch_create(GL_Sheet_t *sheet, size_t count)
{
    GL_Batch_t *batch = malloc(sizeof(GL_Batch_t));
    if (!batch) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate batch");
        return NULL;
    }
    *batch = (GL_Batch_t){
            .sheet = sheet,
            .sprites = NULL
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch %p attached", batch);
    return batch;
}

void GL_batch_destroy(GL_Batch_t *batch)
{
    arrfree(batch->sprites);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch sprites freed");

    free(batch);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch %p freed", batch);
}

void GL_batch_grow(GL_Batch_t *batch, size_t count)
{
    size_t capacity = arrcap(batch->sprites);
    capacity += count;
    arrsetcap(batch->sprites, capacity);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch %p capacity grown by %d to %d", count, capacity);
}

void GL_batch_clear(GL_Batch_t *batch)
{
    arrfree(batch->sprites);
    batch->sprites = NULL;
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "batch cleared");
}

void GL_batch_add(GL_Batch_t *batch, GL_Batch_Sprite_t sprite)
{
    arrpush(batch->sprites, sprite);
}

//extern GL_Batch_Sprite_t *GL_batch_get_sprite(GL_Batch_t *batch, size_t index);

void GL_batch_blit(const GL_Batch_t *batch, GL_Context_t *context)
{
    const GL_Sheet_t *sheet = batch->sheet;
    const GL_Rectangle_t *cells = sheet->cells;

    GL_Batch_Sprite_t *current = batch->sprites;
    for (int count = arrlen(batch->sprites); count; --count) {
        GL_Batch_Sprite_t *sprite = current++;
        GL_context_blit(context, sheet->atlas, cells[sprite->cell_id], sprite->position);
    }
}

void GL_batch_blit_s(const GL_Batch_t *batch, GL_Context_t *context)
{
    const GL_Sheet_t *sheet = batch->sheet;
    const GL_Rectangle_t *cells = sheet->cells;

    GL_Batch_Sprite_t *current = batch->sprites;
    for (int count = arrlen(batch->sprites); count; --count) {
        GL_Batch_Sprite_t *sprite = current++;
        GL_context_blit_s(context, sheet->atlas, cells[sprite->cell_id], sprite->position, sprite->sx, sprite->sy);
    }
}

void GL_batch_blit_sr(const GL_Batch_t *batch, GL_Context_t *context)
{
    const GL_Sheet_t *sheet = batch->sheet;
    const GL_Rectangle_t *cells = sheet->cells;

    GL_Batch_Sprite_t *current = batch->sprites;
    for (int count = arrlen(batch->sprites); count; --count) {
        GL_Batch_Sprite_t *sprite = current++;
        GL_context_blit_sr(context, sheet->atlas, cells[sprite->cell_id], sprite->position, sprite->sx, sprite->sy, sprite->rotation, sprite->ax, sprite->ay);
    }
}
