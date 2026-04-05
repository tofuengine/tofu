/*
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

#include "image.h"

#include <core/config.h>
#define _LOG_TAG "image"
#include <libs/log.h>
#include <libs/stb.h>

#include <memory.h>

// Note: initially, the image format of choice was PNG (TGA would be another
//       candidate). The decoding time wasn't that bad, but it required the
//       inclusion of at least two libraries(`libspng` and `miniz`) for a clean
//       memory-efficient implementation.
//       Also, supporting runtime palette remapping complicated quite a bit the
//       final game code. So, we devised a custom image format, optimized for
//       loading times and memory usage. The color remapping need to be done
//       "offline" with `imagen`. This is realistic, as in a game typically we
//       don't want really that much level of freedom (the palette is chosen
//       carefully by the pixel-artist).

#define IMG_MAGIC "TOFUIMG!"
#define IMG_MAGIC_SIZE 8

#pragma pack(push, 1)
typedef struct _img_header_s {
    uint8_t magic[IMG_MAGIC_SIZE]; // "TOFUIMG!"
    uint16_t width; // Width of the image in pixels (0-65535)
    uint16_t height; // Height of the image in pixels (0-65535)
    uint8_t palette[IMG_PALETTE_MAX_COLORS * 3]; // Max 128 RGB entries
} _img_header_t;
#pragma pack(pop)

bool image_decode_from_callbacks(const image_io_callbacks_t *io_callbacks, void *io_user_data,
                                 const image_decode_callbacks_t *decode_callbacks, void *decode_user_data)
{
    _img_header_t header;
    size_t header_size = io_callbacks->read(io_user_data, &header, sizeof(header));
    if (header_size != sizeof(header)) {
        LOG_E("can't read image header");
        goto error_exit;
    }
    if (memcmp(header.magic, IMG_MAGIC, IMG_MAGIC_SIZE) != 0) {
        LOG_E("invalid image header");
        goto error_exit;
    }

    size_t row_buffer_size = header.width;
    void *row_buffer = malloc(row_buffer_size);
    if (!row_buffer) {
        LOG_E("can't allocate row buffer");
        goto error_exit;
    }

    bool allocated = decode_callbacks->on_allocate(decode_user_data, header.width, header.height, header.palette, IMG_PALETTE_MAX_COLORS);
    if (!allocated) {
        LOG_E("can't allocate target buffer");
        goto error_free_row_buffer;
    }

    bool success = true;
    for (size_t row_index = 0; success && row_index < header.height; ++row_index) {
        success = io_callbacks->read(io_user_data, row_buffer, row_buffer_size) == row_buffer_size;
        success = success && decode_callbacks->on_scanline(decode_user_data, row_index, row_buffer);
    }

    LOG_IF_D(success, "image decoded");

    decode_callbacks->on_free(decode_user_data, success);

    if (!success) {
        LOG_E("can't decode image");
        goto error_free_row_buffer;
    }

    free(row_buffer);

    return success;

error_free_row_buffer:
    free(row_buffer);
error_exit:
    return false;
}

bool image_encode_to_callbacks(const image_io_callbacks_t *io_callbacks, void *io_user_data,
                               const image_encode_callbacks_t *encode_callbacks, void *encode_user_data)
{
    size_t width, height;
    uint8_t palette[IMG_PALETTE_MAX_COLORS * 3] = { 0 };
    size_t palette_length = IMG_PALETTE_MAX_COLORS;
    bool initialized = encode_callbacks->on_initialize(encode_user_data, &width, &height, palette, &palette_length);
    if (!initialized) {
        LOG_E("can't initialize encoding");
        goto error_exit;
    }

    _img_header_t header = (_img_header_t){
            .magic = IMG_MAGIC,
            .width = (uint16_t)width,
            .height = (uint16_t)height,
            .palette = { 0 }
        };
    memcpy(header.palette, palette, IMG_PALETTE_MAX_COLORS * 3);

    size_t bytes_written = io_callbacks->write(io_user_data, &header, sizeof(header)) == sizeof(header);
    if (bytes_written != sizeof(header)) {
        LOG_E("can't write image header");
        goto error_exit;
    }

    size_t row_buffer_size = width;
    void *row_buffer = malloc(row_buffer_size);
    if (!row_buffer) {
        LOG_E("can't allocate row buffer");
        goto error_exit;
    }

    bool success = true;
    for (size_t row_index = 0; success && row_index < height; ++row_index) {
        success = encode_callbacks->on_scanline(encode_user_data, row_index, row_buffer);
        success = success && io_callbacks->write(io_user_data, row_buffer, row_buffer_size) == row_buffer_size;
    }

    LOG_IF_D(success, "image encoded");

    encode_callbacks->on_deinitialize(encode_user_data, success);

    if (!success) {
        LOG_E("can't encode image");
        goto error_free_row_buffer;
    }

    free(row_buffer);

    return success;

error_free_row_buffer:
    free(row_buffer);
error_exit:
    return false;
}
