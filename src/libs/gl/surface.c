/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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
#include <libs/mumalloc.h>

#define LOG_CONTEXT "gl-surface"

static inline bool _is_power_of_two(int n)
{
    return n && !(n & (n - 1));
}

GL_Surface_t *GL_surface_decode(size_t width, size_t height, const void *pixels, GL_Surface_Callback_t callback, void *user_data)
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
    GL_Pixel_t *data = mu_malloc(sizeof(GL_Pixel_t) * width * height);
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate (%dx%d) pixel-data", width, height);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface created at %p (%dx%d)", data, width, height);

    GL_Surface_t *surface = mu_malloc(sizeof(GL_Surface_t));
    if (!surface) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate surface");
        mu_free(data);
        return NULL;
    }

    *surface = (GL_Surface_t){
            .width = width,
            .height = height,
            .data = data,
            .data_size = width * height,
            .is_power_of_two = _is_power_of_two((int)width) && _is_power_of_two((int)height)
        };

    return surface;
}

void GL_surface_destroy(GL_Surface_t *surface)
{
    mu_free(surface->data);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface data at %p freed", surface->data);

    mu_free(surface);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface %p freed", surface);
}

void GL_surface_clear(const GL_Surface_t *surface, GL_Pixel_t index)
{
#ifdef __NO_MEMSET_MEMCPY__
    GL_Pixel_t *dst = surface->data;
    for (size_t i = surface->data_size; i; --i) {
        *(dst++) = index;
    }
#else
    mu_memset(surface->data, index, surface->data_size);
#endif
}

GL_Pixel_t GL_surface_peek(const GL_Surface_t *surface, GL_Point_t position)
{
    return surface->data[position.y * surface->width + position.x];
}

void GL_surface_poke(const GL_Surface_t *surface, GL_Point_t position, GL_Pixel_t index)
{
    surface->data[position.y * surface->width + position.x] = index;
}
