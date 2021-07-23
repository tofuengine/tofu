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

#include "decoder.h"

#include <libs/base64.h>
#include <libs/imath.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <stdint.h>

#define LOG_CONTEXT "decode"

bool decoder_is_valid(const char *encoded_data)
{
    return base64_is_valid(encoded_data);
}

Blob_t decoder_as_blob(const char *encoded_data)
{
    size_t size = base64_decoded_size(encoded_data);

    uint8_t *ptr = malloc(size);
    if (!ptr) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d byte(s)", size);
        return (Blob_t){ 0 };
    }

    base64_decode(ptr, size, encoded_data);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "decoded %d byte(s)", size);

    return (Blob_t){
            .ptr = ptr,
            .size = size
        };
}

typedef struct _Cursor_t {
    uint8_t *data;
    size_t size;
    size_t index;
} Cursor_t;

static int _stbi_io_read(void *user_data, char *data, int size)
{
    Cursor_t *cursor = (Cursor_t *)user_data;
    int count = imin(size, cursor->size - cursor->index);
    memcpy(data, cursor->data + cursor->index, count);
    cursor->index += count;
    return count;
}

static void _stbi_io_skip(void *user_data, int n)
{
    Cursor_t *cursor = (Cursor_t *)user_data;
    cursor->index += n;
}

static int _stbi_io_eof(void *user_data)
{
    Cursor_t *cursor = (Cursor_t *)user_data;
    return cursor->index >= cursor->size ? -1 : 0;
}

static const stbi_io_callbacks _stbi_io_callbacks = {
    _stbi_io_read,
    _stbi_io_skip,
    _stbi_io_eof,
};

Image_t decoder_as_image(const char *encoded_data)
{
    size_t size = base64_decoded_size(encoded_data);

    uint8_t *data = malloc(size);
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d byte(s)", size);
        return (Image_t){ 0 };
    }

    base64_decode(data, size, encoded_data);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "decoded %d byte(s)", size);

    int width, height, components;
    void *pixels = stbi_load_from_callbacks(&_stbi_io_callbacks, &(Cursor_t){ .data = data, .size = size, .index = 0 }, &width, &height, &components, STBI_rgb_alpha);
    if (!pixels) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't decode image from data `%p` (%s)", data, stbi_failure_reason());
        free(data);
        return (Image_t){ 0 };
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "decoded %dx%d image", width, height);

    free(data);

    return (Image_t){
            .width = width,
            .height = height,
            .pixels = pixels
        };
}
