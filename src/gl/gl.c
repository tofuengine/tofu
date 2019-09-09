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

bool GL_initialize(GL_t *gl, size_t width, size_t height, size_t colors)
{
    uint8_t *buffer = malloc(width * height * sizeof(uint8_t));
    if (!buffer) {
        return false;
    }

    uint8_t **rows = malloc(height * sizeof(uint8_t *));
    if (!rows) {
        free(buffer);
        return false;
    }
    for (size_t i = 0; i < height; ++i) {
        rows[i] = width * i;
    }

    uint8_t *vram = malloc(width * height * sizeof(uint8_t));
    if (!vram) {
        free(buffer);
        free(rows);
        return false;
    }

    GLuint texture;
    glGenTextures(1, &texture); //allocate the memory for texture
    glBindTexture(GL_TEXTURE_2D, texture); //Binding the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0); // Disable mip-mapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, vram);

    *gl = (GL_t){
            .width = width,
            .height = height,
            .buffer = buffer,
            .rows = rows,
            .vram = vram,
            .texture = texture,
            .context = (GL_Context_t){}
        };

    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        gl->context.shifting[i] = i;
        gl->context.transparent[i] = 1;
    }
    gl->context.transparent[0] = 0;

    GL_palette_greyscale(&gl->context.palette, colors);
    Log_write(LOG_LEVELS_DEBUG, "<GL> calculating greyscale palette of #%d entries", colors);

    return true;
}

void GL_terminate(GL_t *gl)
{
    free(gl->buffer);
    free(gl->rows);
    free(gl->vram);

    glDeleteBuffers(1, &gl->texture);
    Log_write(LOG_LEVELS_DEBUG, "<GL> texture w/ id #%d deleted", gl->texture);

    *gl = (GL_t){};
}

void GL_push(GL_t *gl)
{
}

void GL_pop(GL_t *gl)
{
}

void GL_prepare(const GL_t *gl)
{
    const GL_Color_t *colors = &gl->context.palette;

    uint8_t *src = gl->buffer;
    GL_Color_t *dst = gl->vram;
    for (size_t i = 0; i < gl->height; ++i) {
        for (size_t j = 0; j < gl->width; ++j) {
            dst[j] = colors[src[j]];
        }
        src += gl->width;
        dst += gl->width;
    }

    glBindTexture(GL_TEXTURE_2D, gl->texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gl->width, gl->height, GL_RGBA, GL_UNSIGNED_BYTE, gl->vram);
//    glBindTexture(GL_TEXTURE_2D, 0);
}
