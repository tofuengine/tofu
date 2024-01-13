/*
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#ifndef TOFU_LIBS_FS_PAK_H
#define TOFU_LIBS_FS_PAK_H

#include "fs.h"

/*
+---------+
| HEADER  | sizeof(Pak_Header_t)
+---------+
| ENTRY 0 | sizeof(Pak_Entry_t)
+---------+
| ENTRY 1 |     "         "
+---------+
    ...
    ...
    ...
+---------+
| ENTRY n |     "         "
+---------+
| DATA 0  | sizeof(Entry) * sizeof(uint8_t)
+---------+
| DATA 1  |     "                     "
+---------+
    ...
    ...
    ...
+---------+
| DATA n  |     "                     "
+---------+

NOTE: `uint16_t` and `uint32_t` data are explicitly stored in little-endian.
*/

#define PAK_SIGNATURE        "TOFUPAK!"
#define PAK_SIGNATURE_LENGTH 8

#define PAK_VERSION          0

#define PAK_ID_LENGTH    MD5_SIZE
#define PAK_ID_LENGTH_SZ (PAK_ID_LENGTH * 2 + 1)

#define PAK_KEY_LENGTH   PAK_ID_LENGTH

extern bool FS_pak_is_valid(const char *path);
extern FS_Mount_t *FS_pak_mount(const char *path);

#endif /* TOFU_LIBS_FS_PAK_H */
