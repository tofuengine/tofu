/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
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

#include "hex.h"

#include <stddef.h>

static inline uint32_t _from_hex(const char *hex, size_t length)
{
    uint8_t value = 0;
    for (size_t i = 0; i < length; ++i) {
        char c = hex[i];
        if (c == '\0') {
            break;
        }
        value <<= 4;
        if (c >= '0' && c <= '9') {
            value |= (uint8_t)(c - '0');
        } else
        if (c >= 'a' && c <= 'f') {
            value |= (uint8_t)(c - 'a' + 10);
        } else
        if (c >= 'A' && c <= 'F') {
            value |= (uint8_t)(c - 'A' + 10);
        }
    }
    return value;
}

uint8_t hex_to_uint8(const char *hex)
{
    return _from_hex(hex, 2);
}

uint16_t hex_to_uint16(const char *hex)
{
    return _from_hex(hex, 4);
}

uint32_t hex_to_uint32(const char *hex)
{
    return _from_hex(hex, 8);
}
