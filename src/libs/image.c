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

#define _BYTES_PER_PIXEL 4

#define _IMG_PALETTE_MAX_LENGTH 128
#define _IMG_PIXEL_TO_PALETTE_INDEX(pixel) ((pixel) & 0x7F)
#define _IMG_PIXEL_IS_TRANSPARENT(pixel) (((pixel) & 0x80) != 0)


// "TOFUIMG!" is the proprietary image format designed for Tofu Engine.
//
// At it's core is a simple indexed-color format where each pixel occupies
// a single byte.
//
// Each pixel represents as index into a palette of up to 128 colors. This
// is due to the fact that the high bit of the pixel byte is reserved to
// indicate transparency. So, basically
//
//              +++++++---> palette index (0-127)
//              |||||||
//     pixel = 76543210
//             |
//             +--> transparency flag (0 or 1)
//
// The maximum palette size is 128 colors, which are more that enough for the
// kind of pixel-art graphics that Tofu Engine is designed to handle.
//
// The file format includes a header that limits the maximum image size to
// 65535x65535 pixels, which is more than enough for any scenario we can
// imagine.

#pragma pack(push, 1)
typedef struct _img_header_s {
    uint8_t magic[8]; // "TOFUIMG!"
    uint16_t width; // Width of the image in pixels (0-65535)
    uint16_t height; // Height of the image in pixels (0-65535)
    uint8_t palette[_IMG_PALETTE_MAX_LENGTH * 3]; // Max 128 RGB entries
} _img_header_t;
#pragma pack(pop)

static const char *_img_magic = "TOFUIMG!";

bool image_decode_from_callbacks(const image_io_callbacks_t *io_callbacks, void *io_user_data,
                                 const image_decode_callbacks_t *decode_callbacks, void *decode_user_data)
{
    _img_header_t header;
    size_t header_size = io_callbacks->read(io_user_data, &header, sizeof(header));
    if (header_size != sizeof(header)) {
        LOG_E("can't read image header");
        goto error_exit;
    }
    if (memcmp(header.magic, _img_magic, sizeof(header.magic)) != 0) {
        LOG_E("invalid image header");
        goto error_exit;
    }

    size_t row_buffer_size = header.width;
    void *row_buffer = malloc(row_buffer_size);
    if (!row_buffer) {
        LOG_E("can't allocate row buffer");
        goto error_exit;
    }

    bool allocated = decode_callbacks->on_allocate(decode_user_data, header.width, header.height, header.palette, _IMG_PALETTE_MAX_LENGTH);
    if (!allocated) {
        LOG_E("can't allocate target buffer");
        goto error_free_row_buffer;
    }

    bool success = true;
    for (size_t row_index = 0; success && row_index < header.height; ++row_index) {
        size_t bytes_read = io_callbacks->read(io_user_data, row_buffer, row_buffer_size);
        success = bytes_read == row_buffer_size;
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
    return false;
}
