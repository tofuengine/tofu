
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

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// We are using `miniaudio`'s ring-buffer, but we could also go for an in-house implementation
// https://embedjournal.com/implementing-circular-buffer-embedded-c/
// https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/

typedef struct _Buffer_t {
    void *data;
    size_t index;
    size_t size;
} Buffer_t;

extern bool buffer_init(Buffer_t *buffer, size_t size);
extern void buffer_uninit(Buffer_t *buffer);

extern void buffer_reset(Buffer_t *buffer);

extern size_t buffer_available(const Buffer_t *buffer);
extern void *buffer_lock(Buffer_t *buffer, size_t *requested);
extern void buffer_unlock(Buffer_t *buffer, void *ptr, size_t used);

#endif  /* __BUFFER_H__ */