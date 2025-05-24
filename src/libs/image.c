/*
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

#include "image.h"

#include <core/config.h>
#define _LOG_TAG "image"
#include <libs/log.h>
#include <libs/stb.h>

#include <spng/spng.h>

#include <memory.h>

typedef struct _io_callbacks_closure_s {
    const image_io_callbacks_t *callbacks;
    void *user_data;
} _io_callbacks_closure_t;

static int _spng_read(spng_ctx *ctx, void *user, void *buffer, size_t bytes_to_read)
{
    _io_callbacks_closure_t *closure = (_io_callbacks_closure_t *)user;

    size_t bytes_read = closure->callbacks->read(closure->user_data, buffer, bytes_to_read);
    if (bytes_read != bytes_to_read) {
        if (closure->callbacks->eof(closure->user_data)) {
            return SPNG_IO_EOF;
        }
        return SPNG_IO_ERROR;
    }

    return SPNG_OK;
}

#if defined(DEBUG) && !defined(SANITIZE)
static void *_spng_malloc(size_t size)
{
    return malloc(size);
}

static void *_spng_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

static void *_spng_calloc(size_t count, size_t size)
{
    void *ptr = malloc(count * size);
    if (ptr) {
        memset(ptr, 0x00, count * size);
    }
    return ptr;
}

static void _spng_free(void *ptr)
{
    free(ptr);
}

static struct spng_alloc _spng_alloc = { // Can't declare this struct as `const` due to SPNG API.
    .malloc_fn = _spng_malloc,
    .realloc_fn = _spng_realloc,
    .calloc_fn = _spng_calloc,
    .free_fn = _spng_free
};
#endif

bool image_decode_from_callbacks(const image_io_callbacks_t *io_callbacks, void *io_user_data,
                                 const image_decode_callbacks_t *decode_callbacks, void *decode_user_data)
{
#if defined(DEBUG) && !defined(SANITIZE)
    spng_ctx *ctx = spng_ctx_new2(&_spng_alloc, 0);
#else
    spng_ctx *ctx = spng_ctx_new(0);
#endif
    if (!ctx) {
        LOG_E("can't allocate context");
        goto error_exit;
    }

    spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);
    spng_set_png_stream(ctx, _spng_read, &(_io_callbacks_closure_t){
            .callbacks = io_callbacks,
            .user_data = io_user_data
        });

    struct spng_ihdr ihdr;
    int result = spng_get_ihdr(ctx, &ihdr);
    if (result) {
        LOG_E("can't decode image (%s)", spng_strerror(result));
        goto error_free_context;
    }
    LOG_D("image size is %dx%d", ihdr.width, ihdr.height);

    size_t image_size;
    spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &image_size);
    size_t row_buffer_size = image_size / ihdr.height;

    void *row_buffer = malloc(row_buffer_size);
    if (!row_buffer) {
        LOG_E("can't allocate row buffer");
        goto error_free_context;
    }

    bool allocated = decode_callbacks->on_allocate(decode_user_data, ihdr.width, ihdr.height);
    if (!allocated) {
        LOG_E("can't allocate target buffer");
        goto error_free_row_buffer;
    }

    spng_decode_image(ctx, NULL, 0, SPNG_FMT_RGBA8, SPNG_DECODE_PROGRESSIVE);
    do {
        struct spng_row_info row_info = { 0 };

        result = spng_get_row_info(ctx, &row_info);
        if (result) {
            break;
        }

        result = spng_decode_row(ctx, row_buffer, row_buffer_size);
        decode_callbacks->on_scanline(decode_user_data, row_info.row_num, row_buffer);
    } while (!result);

    bool success = result == SPNG_EOI;
    LOG_IF_D(success, "image decoded");

    decode_callbacks->on_free(decode_user_data, success);

    if (!success) {
        LOG_E("can't decode image (%s)", spng_strerror(result));
        goto error_free_row_buffer;
    }

    free(row_buffer);
    spng_ctx_free(ctx);

    return success;

error_free_row_buffer:
    free(row_buffer);
error_free_context:
    spng_ctx_free(ctx);
error_exit:
    return false;
}
