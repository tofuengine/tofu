/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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

#include "xor.h"

void xor_schedule(xor_context_t *context, const uint8_t *key, size_t size)
{
    *context = (xor_context_t){ 
            .n = size < XOR_MAX_KEY_LENGTH ? size : XOR_MAX_KEY_LENGTH
        };

    for (size_t i = 0; i < context->n; ++i) {
        context->K[i] = key[i];
    }
}

void xor_process(xor_context_t *context, uint8_t *out, const uint8_t *in, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        out[i] = in[i] ^ context->K[context->i];
        context->i = (context->i + 1) % context->n;
    }
}

void xor_seek(xor_context_t *context, size_t index)
{
    context->i = index % context->n;
}
