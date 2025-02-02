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
 * Copyright (c) 2019-2025 Marco Lizza
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

static int _stbi_io_read(void *user_data, char *data, int size)
{
    GL_Callbacks_Closure_t *closure = (GL_Callbacks_Closure_t *)user_data;
    return (int)closure->callbacks->read(closure->user_data, data, size);
}

static void _stbi_io_skip(void *user_data, int n)
{
    GL_Callbacks_Closure_t *closure = (GL_Callbacks_Closure_t *)user_data;
    closure->callbacks->seek(closure->user_data, n, SEEK_CUR);
}

static int _stbi_io_eof(void *user_data)
{
    GL_Callbacks_Closure_t *closure = (GL_Callbacks_Closure_t *)user_data;
    return closure->callbacks->eof(closure->user_data);
}

static const stbi_io_callbacks _stbi_io_callbacks = {
    _stbi_io_read,
    _stbi_io_skip,
    _stbi_io_eof,
};

// TODO: pass `callback` by pointer, as well!!!
GL_Surface_t *GL_surface_decode_from_callbacks(const GL_Callbacks_t *io_callbacks, void *io_user_data, GL_Surface_Callback_t callback, void *user_data)
{
    int width, height, components;
    void *pixels = stbi_load_from_callbacks(&_stbi_io_callbacks,
        &(GL_Callbacks_Closure_t){
            .callbacks = io_callbacks,
            .user_data = io_user_data
        }, &width, &height, &components, STBI_rgb_alpha);
    if (!pixels) {
        LOG_E("can't decode surface (%s)", stbi_failure_reason());
        goto error_exit;
    }
    LOG_D("loaded %dx%d image", width, height);

    GL_Surface_t *surface = GL_surface_create(width, height);
    if (!surface) {
        goto error_free_pixels;
    }

    callback(user_data, surface, pixels);
    LOG_D("surface decoded at %p (%dx%d)", surface->data, width, height);

    free(pixels);

    return surface;

error_free_pixels:
    free(pixels);
error_exit:
    return NULL;
}

GL_Surface_t *GL_surface_create(size_t width, size_t height)
{
    GL_Pixel_t *data = malloc(sizeof(GL_Pixel_t) * width * height);
    if (!data) {
        LOG_E("can't allocate `%dx%d` pixel-data", width, height);
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

    LOG_D("surface %p created with size `%dx%d`", data, width, height);

    return surface;

error_free_data:
    free(data);
error_exit:
    return NULL;
}

void GL_surface_destroy(GL_Surface_t *surface)
{
    LOG_D("destroying surface %p", surface);

    free(surface->data);
    LOG_D("surface data at %p freed", surface->data);

    free(surface);
    LOG_D("surface freed");
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
