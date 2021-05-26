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

#ifndef __GL_BATCH_H__
#define __GL_BATCH_H__

#include "common.h"
#include "sheet.h"
#include "surface.h"
#include "state.h"

#include <stdbool.h>

typedef struct _GL_Batch_Sprite_t {
    GL_Cell_t cell_id;
    GL_Point_t position;
    float sx, sy;
    int rotation;
    float ax, ay;
} GL_Batch_Sprite_t;

typedef struct _GL_Batch_t {
    const GL_Sheet_t *sheet;
    GL_Batch_Sprite_t *sprites;
} GL_Batch_t;

extern GL_Batch_t *GL_batch_create(const GL_Sheet_t *sheet, size_t capacity);
extern void GL_batch_destroy(GL_Batch_t *batch);

extern bool GL_batch_resize(GL_Batch_t *batch, size_t capacity);
extern bool GL_batch_grow(GL_Batch_t *batch, size_t amount); // Can't shrink or references would be lost.
extern void GL_batch_clear(GL_Batch_t *batch);
extern void GL_batch_add(GL_Batch_t *batch, GL_Batch_Sprite_t sprite);

//extern GL_Batch_Sprite_t *GL_batch_get_sprite(const GL_Batch_t *batch, size_t index);

void GL_batch_blit(const GL_Batch_t *batch, const GL_Surface_t *surface, const GL_State_t state); // FIXME: rename to `flush()`
void GL_batch_blit_s(const GL_Batch_t *batch, const GL_Surface_t *surface, const GL_State_t state);
void GL_batch_blit_sr(const GL_Batch_t *batch, const GL_Surface_t *surface, const GL_State_t state);

#endif  /* __GL_BATCH_H__ */
