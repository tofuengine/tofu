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

#ifndef __GL_BATCH_H__
#define __GL_BATCH_H__

#include "common.h"
#include "context.h"
#include "sheet.h"

#include <stdbool.h>

typedef struct _GL_Batch_Sprite_t {
    int cell_id;

    GL_Point_t position;
    float sx, sy;
    int rotation;
    float ax, ay;

    bool used;
} GL_Batch_Sprite_t;

typedef struct _GL_Batch_t {
    GL_Sheet_t *sheet;
    GL_Batch_Sprite_t *sprites;
} GL_Batch_t;

extern GL_Batch_t *GL_batch_create(GL_Sheet_t *sheet, size_t count);
extern void GL_batch_destroy(GL_Batch_t *batch);

extern void GL_batch_grow(GL_Batch_t *batch, size_t count); // Can't shrink or references would be lost.
extern void GL_batch_clear(GL_Batch_t *batch);

extern GL_Batch_Sprite_t *GL_batch_get_sprite(GL_Batch_t *batch, size_t index);

extern void GL_batch_blit(GL_Batch_t *batch, GL_Context_t *context);

#endif  /* __GL_BATCH_H__ */