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
 * Copyright (c) 2019-2026 Marco Lizza
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
#include <libs/image.h>
#define _LOG_TAG "gl-surface"
#include <libs/log.h>
#include <libs/stb.h>

#include <memory.h>
#include <stdlib.h>

static size_t _read(void *user_data, void *buffer, size_t bytes_to_read)
{
    GL_IO_Callbacks_Closure_t *closure = (GL_IO_Callbacks_Closure_t *)user_data;
    return closure->callbacks->read(closure->user_data, buffer, bytes_to_read);
}

static bool _seek(void *user_data, long offset, int whence)
{
    GL_IO_Callbacks_Closure_t *closure = (GL_IO_Callbacks_Closure_t *)user_data;
    return closure->callbacks->seek(closure->user_data, offset, whence);
}

static long _tell(void *user_data)
{
    GL_IO_Callbacks_Closure_t *closure = (GL_IO_Callbacks_Closure_t *)user_data;
    return closure->callbacks->tell(closure->user_data);
}

static bool _eof(void *user_data)
{
    GL_IO_Callbacks_Closure_t *closure = (GL_IO_Callbacks_Closure_t *)user_data;
    return closure->callbacks->eof(closure->user_data);
}

static const image_io_callbacks_t _image_io_callbacks = {
    .read = _read,
    .seek = _seek,
    .tell = _tell,
    .eof  = _eof
};

typedef struct _decode_callbacks_closure_s {
    GL_Surface_t *surface;
    const GL_Surface_Callbacks_t *callbacks;
    void *user_data;
} _decode_callbacks_closure_t;

static bool _on_allocate(void *user_data, size_t width, size_t height, const uint8_t *palette, size_t palette_length)
{
    _decode_callbacks_closure_t *closure = (_decode_callbacks_closure_t *)user_data;

    GL_Surface_t *surface = GL_surface_create(width, height);
    if (!surface) {
        LOG_E("can't allocate surface");
        return false;
    }
    LOG_D("surface %p allocated with size `%dx%d`", surface, width, height);

    closure->surface = surface;

    closure->callbacks->on_start_of_data(closure->user_data, closure->surface);

    return true;
}

static bool _on_scanline(void *user_data, size_t index, const void *pixels)
{
    _decode_callbacks_closure_t *closure = (_decode_callbacks_closure_t *)user_data;

    closure->callbacks->on_scanline(closure->user_data, closure->surface, (int)index, (const GL_Pixel_t *)pixels);

    return true; // Continue decoding.
}

static void _on_free(void *user_data, bool success)
{
    _decode_callbacks_closure_t *closure = (_decode_callbacks_closure_t *)user_data;

    closure->callbacks->on_end_of_data(closure->user_data, closure->surface, success);

    if (!success) {
        LOG_E("decoding failed, destroying surface");
        GL_surface_destroy(closure->surface);
    } else {
        LOG_D("surface %p decoded", closure->surface);
    }
}

static const image_decode_callbacks_t _image_decode_callbacks = {
    .on_allocate = _on_allocate,
    .on_scanline = _on_scanline,
    .on_free     = _on_free
};

GL_Surface_t *GL_surface_decode_from_callbacks(const GL_IO_Callbacks_t *io_callbacks, void *io_user_data,
                                               const GL_Surface_Callbacks_t *callbacks, void *user_data)
{
    _decode_callbacks_closure_t decode_callbacks_closure = {
        .surface = NULL,
        .callbacks = callbacks,
        .user_data = user_data
    };

    bool decoded = image_decode_from_callbacks(&_image_io_callbacks, &(GL_IO_Callbacks_Closure_t){
            .callbacks = io_callbacks,
            .user_data = io_user_data
        },
        &_image_decode_callbacks, &decode_callbacks_closure);

    if (!decoded) {
        LOG_E("can't decode image from callbacks");
        return NULL;
    }

    return decode_callbacks_closure.surface;
}

static inline bool _is_power_of_two(int n)
{
    return n && !(n & (n - 1));
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
#if defined(_NO_MEMSET_MEMCPY)
    GL_Pixel_t *dst = surface->data;
    for (size_t i = surface->data_size; i; --i) {
        *(dst++) = index;
    }
#else   /* defined(_NO_MEMSET_MEMCPY) */
    memset(surface->data, index, surface->data_size);
#endif  /* defined(_NO_MEMSET_MEMCPY) */
}

GL_Pixel_t GL_surface_peek(const GL_Surface_t *surface, GL_Point_t position)
{
    return surface->data[position.y * surface->width + position.x];
}

void GL_surface_poke(const GL_Surface_t *surface, GL_Point_t position, GL_Pixel_t index)
{
    surface->data[position.y * surface->width + position.x] = index;
}

void GL_surface_remap(const GL_Surface_t *surface, const GL_Pixel_t *shifting)
{
    GL_Pixel_t *dst = surface->data;
    for (size_t i = surface->data_size; i; --i) {
        GL_Pixel_t pixel = *dst;
        GL_Pixel_t index = shifting[GL_PIXEL_GET_INDEX(pixel)];
        *(dst++) = index | (pixel & GL_PIXEL_TRANSPARENCY_MASK); // Preserve transparency bit.
    }
}
