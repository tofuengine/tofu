/*
 * MIT License
 *
 * Copyright (c) 2019-2023 Marco Lizza
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

#include "base64.h"

#include <stdbool.h>
#include <string.h>

// https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/

size_t base64_encoded_size(size_t in_size)
{
	size_t out_size = in_size;
	if (in_size % 3 != 0)
		out_size += 3 - (in_size % 3);
	out_size /= 3;
	out_size *= 4;
	return out_size + 1;
}

static const char _b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void base64_encode(char *out, const uint8_t *in, size_t in_size)
{
	for (size_t i=0, j=0; i < in_size; i+=3, j+=4) {
		uint32_t v = in[i];
		v = i + 1 < in_size ? v << 8 | in[i+ 1] : v << 8;
		v = i + 2 < in_size ? v << 8 | in[i+ 2] : v << 8;

		out[j]     = _b64_table[(v >> 18) & 0x3F];
		out[j + 1] = _b64_table[(v >> 12) & 0x3F];
		if (i + 1 < in_size) {
			out[j + 2] = _b64_table[(v >> 6) & 0x3F];
		} else {
			out[j + 2] = '=';
		}
		if (i + 2 < in_size) {
			out[j + 3] = _b64_table[v & 0x3F];
		} else {
			out[j + 3] = '=';
		}
	}
}

static inline bool _is_valid_char(char c)
{
    if (c >= '0' && c <= '9')
        return 1;
    if (c >= 'A' && c <= 'Z')
        return 1;
    if (c >= 'a' && c <= 'z')
        return 1;
    if (c == '+' || c == '/' || c == '=')
        return 1;
    return 0;
}

bool base64_is_valid(const char *in)
{
    size_t length = strlen(in);

    if (length % 4 != 0) {
        return false;
    }

    for (size_t i = 0; i < length; ++i) {
        if (!_is_valid_char(in[i])) {
            return false;
        }
    }
    return true;
}

size_t base64_decoded_size(const char *in)
{
    size_t length = strlen(in);

    size_t size = length / 4 * 3;

    for (size_t i = length - 1; i >= length  - 2; --i) {
        if (in[i] == '=') {
            size -= 1;
        } else {
            break;
        }
    }

    return size;
}

static const int _decode_table[] = {
    62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1,
    -1, -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

void base64_decode(uint8_t *out, size_t out_size, const char *in)
{
    for (size_t i=0, j=0; in[i] != '\0'; i += 4, j += 3) {
        uint32_t v = _decode_table[in[i] - 43];
        v = (v << 6) | _decode_table[in[i + 1] - 43];
        v = in[i + 2]=='=' ? v << 6 : (v << 6) | _decode_table[in[i + 2] - 43];
        v = in[i + 3]=='=' ? v << 6 : (v << 6) | _decode_table[in[i + 3] - 43];

        out[j] = (v >> 16) & 0xFF;
        if (in[i + 2] != '=')
            out[j + 1] = (v >> 8) & 0xFF;
        if (in[i + 3] != '=')
            out[j + 2] = v & 0xFF;
    }
}

