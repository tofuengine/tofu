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
    GL_Pixel_t *data = malloc(width * height * sizeof(GL_Pixel_t));
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

void GL_surface_to_rgba(const GL_Surface_t *surface, const GL_Palette_t *palette, GL_Color_t *vram)
{
    const size_t data_size = surface->data_size;
    const GL_Color_t *colors = palette->colors;
#ifdef __DEBUG_GRAPHICS__
    int count = palette->count;
#endif
    const GL_Pixel_t *src = surface->data;
    GL_Color_t *dst = vram;
    for (size_t i = data_size; i; --i) {
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

typedef struct _copper_state_t {
    GL_Palette_t palette;
    int modulo;
} copper_state_t;

void GL_surface_to_rgba_run(const GL_Surface_t *surface, const GL_Palette_t *palette, const copper_list_entry_t *copper_list, size_t copper_list_entries, GL_Color_t *vram)
{
    copper_state_t state = {
            .palette = *palette,
            .modulo = 0
        };
    const copper_list_entry_t *entry = copper_list;
    const copper_list_entry_t *end_of_list = copper_list + copper_list_entries;
    size_t wait_y = 0, wait_x = 0;

    GL_Color_t *colors = state.palette.colors;
#ifdef __DEBUG_GRAPHICS__
    int count = palette->count;
#endif
    const GL_Pixel_t *src = surface->data;
    GL_Color_t *dst = vram;

    size_t y = 0;
    for (size_t i = surface->height; i; --i) {
        size_t x = 0;
        for (size_t j = surface->width; j; --j) {
            while (y >= wait_y && x >= wait_x && entry < end_of_list) {
#if 1
                switch (entry->command) {
                    case WAIT: {
                        wait_x = entry->args[0].u;
                        wait_y = entry->args[1].u;
                        break;
                    }
                    case PALETTE: {
                        colors[entry->args[0].u8] = GL_palette_unpack_color(entry->args[1].u32);
                        break;
                    }
                    case MODULO: {
                        state.modulo = entry->args[0].i;
                        break;
                    }
                }
#else
                if (entry->command == WAIT) {
                    wait_x = entry->args[0].u;
                    wait_y = entry->args[1].u;
                } else
                if (entry->command == PALETTE) {
                    colors[entry->args[0].u8] = GL_palette_unpack_color(entry->args[1].u32);
                } else
                if (entry->command == MODULO) {
                    state.modulo = entry->args[0].i;
                }
#endif
                ++entry;
            }

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
            ++x;
        }
        ++y;

        src += state.modulo;
    }
}
