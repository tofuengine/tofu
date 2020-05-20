
/*
 * MIT License
 *
 * Copyright (c) 2019-2020 Marco Lizza
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

#include "buffer.h"

#include <libs/stb.h>

#include <stdlib.h>

bool buffer_init(Buffer_t *buffer, size_t length, size_t bytes_per_frame)
{
    void *frames = malloc(length * bytes_per_frame);
    if (!frames) {
        return false;
    }

    *buffer = (Buffer_t){
            .frames = frames,
            .bytes_per_frame = bytes_per_frame,
            .length = length,
            .index = 0
        };

    return true;
}

void buffer_uninit(Buffer_t *buffer)
{
    free(buffer->frames);
}

void buffer_reset(Buffer_t *buffer)
{
    buffer->index = 0;
}

size_t buffer_available(const Buffer_t *buffer)
{
    return (buffer->length - buffer->index) / buffer->bytes_per_frame;
}

void *buffer_lock(Buffer_t *buffer, size_t *requested)
{
    size_t available = (buffer->length - buffer->index) / buffer->bytes_per_frame;
    if (*requested > available) {
        *requested = available;
    }
    return (uint8_t *)buffer->frames + buffer->index;
}

void buffer_unlock(Buffer_t *buffer, void *ptr, size_t used)
{
    void *cursor = (uint8_t *)buffer->frames + buffer->index;
    if (ptr != cursor) {
        return;
    }
    buffer->index += used * buffer->bytes_per_frame;
//    ASSERT(buffer->index <= buffer->size);
}
