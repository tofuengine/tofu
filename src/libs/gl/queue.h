/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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

#ifndef TOFU_LIBS_GL_QUEUE_H
#define TOFU_LIBS_GL_QUEUE_H

#include "common.h"
#include "sheet.h"
#include "surface.h"

#include <stdbool.h>

typedef struct GL_Queue_Sprite_s {
    GL_Cell_t cell_id;
    GL_Point_t position;
    float scale_x, scale_y;
    int rotation;
    float anchor_x, anchor_y;
} GL_Queue_Sprite_t;

typedef struct GL_Queue_s {
    const GL_Sheet_t *sheet;
    GL_Queue_Sprite_t *sprites;
} GL_Queue_t;

extern GL_Queue_t *GL_queue_create(const GL_Sheet_t *sheet, size_t capacity);
extern void GL_queue_destroy(GL_Queue_t *queue);

extern bool GL_queue_resize(GL_Queue_t *queue, size_t capacity);
extern bool GL_queue_grow(GL_Queue_t *queue, size_t amount); // Can't shrink or references would be lost.
extern void GL_queue_clear(GL_Queue_t *queue);
extern void GL_queue_add(GL_Queue_t *queue, GL_Queue_Sprite_t sprite);

//extern GL_Queue_Sprite_t *GL_queue_get_sprite(const GL_Queue_t *queue, size_t index);

void GL_queue_blit(const GL_Queue_t *queue, const GL_Context_t *context); // FIXME: rename to `flush()`
void GL_queue_blit_s(const GL_Queue_t *queue, const GL_Context_t *context);
void GL_queue_blit_sr(const GL_Queue_t *queue, const GL_Context_t *context);

#endif  /* TOFU_LIBS_GL_QUEUE_H */
