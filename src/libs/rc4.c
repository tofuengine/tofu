/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "rc4.h"

void rc4_schedule(rc4_context_t *context, const uint8_t *key, size_t key_size)
{
    uint8_t *S = context->S;
    for (size_t k = 0; k < 256; ++k) {
        S[k] = k;
    }
    uint8_t i = 0, j = 0;
    for (size_t k = 0; k < 256; ++k) {
        const uint8_t Si = S[i];
        j += Si + key[i % key_size];
        S[i] = S[j]; // Swap!
        S[j] = Si;
        i += 1;
    }
    context->i = 0;
    context->j = 0;
}

void rc4_process(rc4_context_t *context, uint8_t *data, size_t data_size)
{
    uint8_t i = context->i, j = context->j;
    uint8_t *S = context->S;
    for (size_t k = 0; k < data_size; ++k) {
        i += 1;
        const uint8_t Si = S[i];
        j += Si;
        const uint8_t Sj = S[i] = S[j]; // Swap!
        S[j] = Si;
        data[k] ^= S[(uint8_t)(Si + Sj)];
    }
    context->i = i;
    context->j = j;
}
