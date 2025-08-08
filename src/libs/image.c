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

#define _BYTES_PER_PIXEL 4

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

    bool allocated = decode_callbacks->on_allocate(decode_user_data, ihdr.width, ihdr.height, _BYTES_PER_PIXEL);
    if (!allocated) {
        LOG_E("can't allocate target buffer");
        goto error_free_row_buffer;
    }

    result = spng_decode_image(ctx, NULL, 0, SPNG_FMT_RGBA8, SPNG_DECODE_PROGRESSIVE);
    if (result != SPNG_OK) {
        LOG_E("can't start decoding (%s)", spng_strerror(result));
        goto error_free_row_buffer;
    }

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

static int _spng_write(spng_ctx *ctx, void *user, void *buffer, size_t bytes_to_write)
{
    _io_callbacks_closure_t *closure = (_io_callbacks_closure_t *)user;

    size_t bytes_written = closure->callbacks->write(closure->user_data, buffer, bytes_to_write);
    if (bytes_written != bytes_to_write) {
        return SPNG_IO_ERROR;
    }

    return SPNG_OK;
}

bool image_encode_to_callbacks(const image_io_callbacks_t *io_callbacks, void *io_user_data,
                               const image_encode_callbacks_t *encode_callbacks, void *encode_user_data)
{
#if defined(DEBUG) && !defined(SANITIZE)
    spng_ctx *ctx = spng_ctx_new2(&_spng_alloc, SPNG_CTX_ENCODER);
#else
    spng_ctx *ctx = spng_ctx_new(SPNG_CTX_ENCODER);
#endif
    if (!ctx) {
        LOG_E("can't allocate context");
        goto error_exit;
    }

    // TODO: handle errors!
    spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);
    spng_set_png_stream(ctx, _spng_write, &(_io_callbacks_closure_t){
            .callbacks = io_callbacks,
            .user_data = io_user_data
        });

    /* Set image properties, this determines the destination image format */
    /* Valid color type, bit depth combinations: https://www.w3.org/TR/2003/REC-PNG-20031110/#table111 */
    size_t width, height;

    bool initialized = encode_callbacks->on_initialize(encode_user_data, &width, &height);
    if (!initialized) {
        LOG_E("can't initialize encoder");
        goto error_free_context;
    }

    struct spng_ihdr ihdr = {
            .width = width,
            .height = height,
            .bit_depth = 8, // Default to 8 bits per channel
            .color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA // Default to RGBA
        };
    int result = spng_set_ihdr(ctx, &ihdr);
    if (result != SPNG_OK) {
        LOG_E("can't set PNG header (%s)", spng_strerror(result));
        goto error_free_context;
    }

#if defined(IMAGE_HIGHEST_COMPRESSION_LEVEL)
    spng_set_option(ctx, SPNG_IMG_COMPRESSION_LEVEL, 9);
#endif 

    size_t image_size = width * height * _BYTES_PER_PIXEL;
    size_t row_buffer_size = image_size / height;

    void *row_buffer = malloc(row_buffer_size);
    if (!row_buffer) {
        LOG_E("can't allocate row buffer");
        goto error_free_context;
    }

    result = spng_encode_image(ctx, NULL, 0, SPNG_FMT_PNG, SPNG_ENCODE_PROGRESSIVE);
    if (result != SPNG_OK) {
        LOG_E("can't start encoding (%s)", spng_strerror(result));
        goto error_free_row_buffer;
    }

    for (size_t row_index = 0; row_index < height; ++row_index) {
        encode_callbacks->on_scanline(encode_user_data, row_index, row_buffer, row_buffer_size);

        result = spng_encode_row(ctx, row_buffer, row_buffer_size);
        if (result != SPNG_OK) {
            break;
        }
    }

    bool success = result == SPNG_EOI;
    LOG_IF_D(success, "image encoded");

    encode_callbacks->on_deinitialize(encode_user_data, success);

    free(row_buffer);

    spng_ctx_free(ctx); // A call to `spng_encode_chunks()` is not needed.

    return success;

error_free_row_buffer:
    free(row_buffer);
error_free_context:
    spng_ctx_free(ctx);
error_exit:
    return false;
}
