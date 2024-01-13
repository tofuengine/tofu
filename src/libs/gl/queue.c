/*
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "queue.h"

#include "blit.h"

#include <core/config.h>
#define _LOG_TAG "gl-queue"
#include <libs/log.h>
#include <libs/stb.h>

GL_Queue_t *GL_queue_create(const GL_Sheet_t *sheet, size_t capacity)
{
    GL_Queue_t *queue = malloc(sizeof(GL_Queue_t));
    if (!queue) {
        LOG_E("can't allocate queue");
        return NULL;
    }

    GL_Queue_Sprite_t *sprites = NULL;
    if (capacity > 0) {
        bool allocated = arrsetcap(sprites, capacity); // FIXME: should be `!!`?
        if (!allocated) {
            LOG_E("can't allocate queue sprites");
            goto error_free;
        }
    }

    *queue = (GL_Queue_t){
            .sheet = sheet,
            .sprites = sprites
        };
    LOG_D("queue %p attached", queue);

    return queue;

error_free:
    free(queue);
    return NULL;
}

void GL_queue_destroy(GL_Queue_t *queue)
{
    arrfree(queue->sprites);
    LOG_D("queue sprites freed");

    free(queue);
    LOG_D("queue %p freed", queue);
}

bool GL_queue_resize(GL_Queue_t *queue, size_t capacity)
{
    bool allocated = arrsetcap(queue->sprites, capacity); // FIXME: should be `!!`?
    if (!allocated) {
        LOG_E("can't resize queue slots");
        return false;
    }
    LOG_D("queue %p capacity reset to %d", queue, capacity);
    return true;
}

bool GL_queue_grow(GL_Queue_t *queue, size_t amount)
{
    size_t capacity = arrcap(queue->sprites);
    capacity += amount;
    bool allocated = arrsetcap(queue->sprites, capacity); // FIXME: should be `!!`?
    if (!allocated) {
        LOG_E("can't grow queue slots");
        return false;
    }
    LOG_D("queue %p capacity grown by %d slots to %d", queue, amount, capacity);
    return true;
}

void GL_queue_clear(GL_Queue_t *queue)
{
    static const size_t zero = 0; // Note: we don't pass the immediate `0` to avoid a "type-limit" warning from the compiler.
    arrsetlen(queue->sprites, zero);
}

void GL_queue_add(GL_Queue_t *queue, GL_Queue_Sprite_t sprite)
{
    arrpush(queue->sprites, sprite);
}

void GL_queue_blit(const GL_Queue_t *queue, const GL_Context_t *context)
{
    const GL_Sheet_t *sheet = queue->sheet;

    GL_Queue_Sprite_t *current = queue->sprites;
    for (size_t count = arrlenu(queue->sprites); count; --count) {
        GL_Queue_Sprite_t *sprite = current++;
        GL_sheet_blit(sheet, context, sprite->position, sprite->cell_id);
    }
}

void GL_queue_blit_s(const GL_Queue_t *queue, const GL_Context_t *context)
{
    const GL_Sheet_t *sheet = queue->sheet;

    GL_Queue_Sprite_t *current = queue->sprites;
    for (size_t count = arrlenu(queue->sprites); count; --count) {
        GL_Queue_Sprite_t *sprite = current++;
        GL_sheet_blit_s(sheet, context, sprite->position, sprite->cell_id, sprite->scale_x, sprite->scale_y);
    }
}

void GL_queue_blit_sr(const GL_Queue_t *queue, const GL_Context_t *context)
{
    const GL_Sheet_t *sheet = queue->sheet;

    GL_Queue_Sprite_t *current = queue->sprites;
    for (size_t count = arrlenu(queue->sprites); count; --count) {
        GL_Queue_Sprite_t *sprite = current++;
        GL_sheet_blit_sr(sheet, context, sprite->position, sprite->cell_id, sprite->scale_x, sprite->scale_y, sprite->rotation, sprite->anchor_x, sprite->anchor_y);
    }
}
