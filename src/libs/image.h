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

#ifndef TOFU_LIBS_IMAGE_H
#define TOFU_LIBS_IMAGE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct image_decode_callbacks_s {
    size_t (*read)(void *user_data, void *buffer, size_t bytes_to_read);
    bool   (*seek)(void *user_data, long offset, int whence);
    long   (*tell)(void *user_data);
    int    (*eof)(void *user_data);

    bool (*on_allocate)(void *user_data, size_t width, size_t height);
    bool (*on_scanline)(void *user_data, size_t index, const void *pixels);
    void (*on_free)(void *user_data, bool success);
} image_decode_callbacks_t;

typedef struct image_decode_callbacks_closure_s {
    const image_decode_callbacks_t *callbacks;
    void *user_data;
} image_decode_callbacks_closure_t;

extern bool image_decode_from_callbacks(const image_decode_callbacks_t *callbacks, void *user_data);

#endif  /* TOFU_LIBS_IMAGE_H */
