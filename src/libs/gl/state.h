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

#ifndef __GL_STATE_H__
#define __GL_STATE_H__

#include "common.h"
#include "palette.h"

typedef struct _GL_State_t { // FIXME: rename to `GL_State_s`
    GL_Quad_t clipping_region;
    GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS];
    GL_Bool_t transparent[GL_MAX_PALETTE_COLORS];
} GL_State_t;

extern void GL_state_reset(GL_State_t *state, GL_Size_t size);

extern void GL_state_set_clipping(GL_State_t *state, GL_Size_t size, const GL_Rectangle_t *region);
extern void GL_state_set_shifting(GL_State_t *state, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count);
extern void GL_state_set_transparent(GL_State_t *state, const GL_Pixel_t *indexes, const GL_Bool_t *transparent, size_t count);

#endif  /* __GL_STATE_H__ */
