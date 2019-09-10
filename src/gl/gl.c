/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "gl.h"

#include "../log.h"

#include <memory.h>
#include <stdio.h>

bool GL_initialize(GL_t *gl, size_t width, size_t height)
{
    *gl = (GL_t){};

    if (!GL_surface_create(&gl->surface, width, height)) {
        return false;
    }

    gl->context.background = 0;
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        gl->context.shifting[i] = i;
        gl->context.transparent[i] = GL_BOOL_FALSE;
    }
    gl->context.transparent[0] = GL_BOOL_TRUE;

    GL_palette_greyscale(&gl->context.palette, GL_MAX_PALETTE_COLORS);
    Log_write(LOG_LEVELS_DEBUG, "<GL> calculating greyscale palette of #%d entries", GL_MAX_PALETTE_COLORS);

    return true;
}

void GL_terminate(GL_t *gl)
{
    GL_surface_delete(&gl->surface);

    *gl = (GL_t){};
}

void GL_push(GL_t *gl)
{
}

void GL_pop(GL_t *gl)
{
}

void GL_clear(GL_t *gl)
{
    memset(gl->surface.data, gl->context.background, gl->surface.data_size);
}

void GL_prepare(const GL_t *gl, void *vram)
{
    const GL_Color_t *colors = gl->context.palette.colors;

#if 1
    const GL_Pixel_t *src = (const GL_Pixel_t *)gl->surface.data;
    GL_Color_t *dst = (GL_Color_t *)vram;
    for (size_t i = 0; i < gl->surface.data_size; ++i) {
        *(dst++) = colors[*(src++)];
    }
#else
    const GL_Pixel_t *src = (const GL_Pixel_t *)gl->surface.data;
    GL_Color_t *dst = (GL_Color_t *)vram;
    for (size_t i = 0; i < gl->surface.height; ++i) {
        for (size_t j = 0; j < gl->surface.width; ++j) {
            dst[j] = colors[src[j]];
        }
        src += gl->surface.width;
        dst += gl->surface.width;
    }
#endif
}
