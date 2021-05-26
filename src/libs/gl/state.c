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

#include "state.h"

#include <libs/imath.h>

void GL_state_reset(GL_State_t *state, GL_Size_t size)
{
    *state = (GL_State_t){
            .clipping_region = (GL_Quad_t){ .x0 = 0, .y0 = 0, .x1 = size.width - 1, .y1 = size.height - 1 },
            .shifting = { 0 },
            .transparent = { 0 }
        };
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        state->shifting[i] = (GL_Pixel_t)i;
        state->transparent[i] = GL_BOOL_FALSE;
    }
    state->transparent[0] = GL_BOOL_TRUE;
}

void GL_state_set_clipping(GL_State_t *state, GL_Size_t size, const GL_Rectangle_t *region)
{
    if (!region) {
        state->clipping_region = (GL_Quad_t){
                .x0 = 0,
                .y0 = 0,
                .x1 = (int)size.width - 1,
                .y1 = (int)size.height - 1
            };
    } else {
        state->clipping_region = (GL_Quad_t){
                .x0 = imax(0, region->x),
                .y0 = imax(0, region->y),
                .x1 = imin((int)size.width, region->x + (int)region->width) - 1,
                .y1 = imin((int)size.height, region->y + (int)region->height) - 1
            };
    }
}

void GL_state_set_shifting(GL_State_t *state, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count)
{
    if (!from) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            state->shifting[i] = (GL_Pixel_t)i;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            state->shifting[from[i]] = to[i];
        }
    }
}

void GL_state_set_transparent(GL_State_t *state, const GL_Pixel_t *indexes, const GL_Bool_t *transparent, size_t count)
{
    if (!indexes) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            state->transparent[i] = GL_BOOL_FALSE;
        }
        state->transparent[0] = GL_BOOL_TRUE;
    } else {
        for (size_t i = 0; i < count; ++i) {
            state->transparent[indexes[i]] = transparent[i];
        }
    }
}
