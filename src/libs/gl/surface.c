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

#include "surface.h"

#include <config.h>
#include <libs/log.h>
#include <libs/gl/gl.h>
#include <libs/stb.h>

#define LOG_CONTEXT "gl"

bool GL_surface_decode(GL_Surface_t *surface, const void *buffer, size_t buffer_size, const GL_Surface_Callback_t callback, void *parameters)
{
    int width, height, components;
    void *data = stbi_load_from_memory(buffer, buffer_size, &width, &height, &components, STBI_rgb_alpha); //STBI_default);
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't decode surface from #%p: %s", data, stbi_failure_reason());
        return false;
    }
    GL_surface_create(surface, width, height);
    if (callback != NULL) {
        callback(parameters, surface, data);
    }
    stbi_image_free(data);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface decoded at #%p (%dx%d w/ %d bpp)", surface->data, width, height, components);

    return true;
}

bool GL_surface_create(GL_Surface_t *surface, size_t width, size_t height)
{
    GL_Pixel_t *data = malloc(width * height * sizeof(GL_Pixel_t));
    if (!data) {
        return false;
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface created at #%p (%dx%d)", data, width, height);

    *surface = (GL_Surface_t){
            .width = width,
            .height = height,
            .data = data,
            .data_size = width * height
        };

    return true;
}

void GL_surface_delete(GL_Surface_t *surface)
{
    free(surface->data);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface at #%p deleted", surface->data);
}

void GL_surface_to_rgba(const GL_Surface_t *surface, const GL_Palette_t *palette, GL_Color_t *vram)
{
    const int data_size = surface->data_size;
    const GL_Color_t *colors = palette->colors;
#ifdef __DEBUG_GRAPHICS__
    int count = palette->count;
#endif
    const GL_Pixel_t *src = surface->data;
    GL_Color_t *dst = vram;
    for (int i = data_size; i; --i) {
        GL_Pixel_t index = *src++;
#ifdef __DEBUG_GRAPHICS__
        GL_Color_t color;
        if (index >= count) {
            int y = (index - 240) * 8;
            color = (GL_Color_t){ 0, 63 + y, 0, 255 };
        } else {
            color = colors[index];
        }
        *(dst++) = color;
#else
        *(dst++) = colors[index];
#endif
    }
}
