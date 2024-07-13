/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#include <core/config.h>
#define _LOG_TAG "gl-surface"
#include <libs/log.h>
#include <libs/stb.h>

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
    LOG_D("surface decoded at %p (%dx%d)", surface->data, width, height);

    return surface;
}

GL_Surface_t *GL_surface_create(size_t width, size_t height)
{
    GL_Pixel_t *data = malloc(sizeof(GL_Pixel_t) * width * height);
    if (!data) {
        LOG_E("can't allocate (%dx%d) pixel-data", width, height);
        goto error_exit;
    }

    GL_Surface_t *surface = malloc(sizeof(GL_Surface_t));
    if (!surface) {
        LOG_E("can't allocate surface");
        goto error_free_data;
    }

    *surface = (GL_Surface_t){
            .width = width,
            .height = height,
            .data = data,
            .data_size = width * height,
            .is_power_of_two = _is_power_of_two((int)width) && _is_power_of_two((int)height)
        };

    LOG_D("surface created at %p (%dx%d)", data, width, height);

    return surface;

error_free_data:
    free(data);
error_exit:
    return NULL;
}

void GL_surface_destroy(GL_Surface_t *surface)
{
    free(surface->data);
    LOG_D("surface data at %p freed", surface->data);

    free(surface);
    LOG_D("surface %p freed", surface);
}

void GL_surface_clear(const GL_Surface_t *surface, GL_Pixel_t index)
{
#if defined(__NO_MEMSET_MEMCPY__)
    GL_Pixel_t *dst = surface->data;
    for (size_t i = surface->data_size; i; --i) {
        *(dst++) = index;
    }
#else
    memset(surface->data, index, surface->data_size);
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
