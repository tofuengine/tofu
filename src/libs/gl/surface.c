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

#include "surface.h"

#include <config.h>
#include <libs/log.h>
#include <libs/stb.h>

#define LOG_CONTEXT "gl-surface"

GL_Surface_t *GL_surface_decode(size_t width, size_t height, const void *pixels, const GL_Surface_Callback_t callback, void *user_data)
{
    GL_Surface_t *surface = GL_surface_create(width, height);
    if (!surface) {
        return NULL;
    }

    callback(user_data, surface, pixels);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface decoded at %p (%dx%d)", surface->data, width, height);

    return surface;
}

GL_Surface_t *GL_surface_create(size_t width, size_t height)
{
    GL_Pixel_t *data = malloc(sizeof(GL_Pixel_t) * width * height);
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "cant' allocate (%dx%d) pixel-data", width, height);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface created at %p (%dx%d)", data, width, height);

    GL_Surface_t *surface = malloc(sizeof(GL_Surface_t));
    if (!surface) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "cant' allocate surface");
        free(data);
        return NULL;
    }

    *surface = (GL_Surface_t){
            .width = width,
            .height = height,
            .data = data,
            .data_size = width * height
        };

    return surface;
}

void GL_surface_destroy(GL_Surface_t *surface)
{
    free(surface->data);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface-data at %p freed", surface->data);

    free(surface);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface %p freed", surface);
}
