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

#ifndef __SYSTEMS_STORAGE_CACHE_H__
#define __SYSTEMS_STORAGE_CACHE_H__

#include <libs/fs/fs.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Storage_Cache_Entry_Value_s {
    void *data;
    size_t size;
} Storage_Cache_Entry_Value_t;

typedef struct Storage_Cache_Entry_s {
    char *key;
    Storage_Cache_Entry_Value_t value;
} Storage_Cache_Entry_t;

typedef struct Storage_Cache_Stream_s {
    const uint8_t *ptr;
    size_t size;
    size_t position;
} Storage_Cache_Stream_t;

typedef struct Storage_Cache_s {
    FS_Context_t *context;

    Storage_Cache_Entry_t *entries;
} Storage_Cache_t;

extern Storage_Cache_t *Storage_Cache_create(FS_Context_t *context);
extern void Storage_Cache_destroy(Storage_Cache_t *cache);

extern bool Storage_Cache_inject_base64(Storage_Cache_t *cache, const char *name, const char *encoded_data, size_t length);
extern bool Storage_Cache_inject_ascii85(Storage_Cache_t *cache, const char *name, const char *encoded_data, size_t length);
extern bool Storage_Cache_inject_raw(Storage_Cache_t *cache, const char *name, const void *raw_data, size_t size);

#endif  /* __SYSTEMS_STORAGE_CACHE_H__ */
