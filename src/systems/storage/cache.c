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

#include "cache.h"

#include <config.h>
#include <libs/ascii85.h>
#include <libs/base64.h>
#include <libs/fs/fs.h>
#include <libs/log.h>
#include <libs/stb.h>
#include <systems/storage.h>

#include <stdbool.h>

#define LOG_CONTEXT "storage-cache"

static void _cache_scan(void *user_data, FS_Scan_Callback_t callback, void *callback_user_data)
{
    Storage_Cache_t *cache = (Storage_Cache_t *)user_data;
    Storage_Cache_Entry_t *entries = cache->entries;

    for (size_t i = 0; i < shlenu(entries); ++i) {
        const char *name = entries[i].key;
        callback(callback_user_data, name);
    }
}

static bool _cache_contains(void *user_data, const char *name)
{
    Storage_Cache_t *cache = (Storage_Cache_t *)user_data;
    Storage_Cache_Entry_t *entries = cache->entries;

    return shgeti(entries, name) != -1;
}

static void *_cache_open(void *user_data, const char *name)
{
    Storage_Cache_t *cache = (Storage_Cache_t *)user_data;
    Storage_Cache_Entry_t *entries = cache->entries;

    int index = shgeti(entries, name);
    if (index == -1) {
        return NULL;
    }

    const Storage_Cache_Entry_Value_t *value = &entries[index].value;

    Storage_Cache_Stream_t *stream = malloc(sizeof(Storage_Cache_Stream_t));
    if (!stream) {
        return NULL;
    }

    *stream = (Storage_Cache_Stream_t){
        .ptr = (const uint8_t *)value->data,
        .size = value->size,
        .position = 0
    };

    return stream;
}

static void _cache_close(void *stream)
{
    Storage_Cache_Stream_t *cache_stream = (Storage_Cache_Stream_t *)stream;

    free(cache_stream);
}

static size_t _cache_size(void *stream)
{
    Storage_Cache_Stream_t *cache_stream = (Storage_Cache_Stream_t *)stream;

    return cache_stream->size;
}

static size_t _cache_read(void *stream, void *buffer, size_t bytes_requested)
{
    Storage_Cache_Stream_t *cache_stream = (Storage_Cache_Stream_t *)stream;

    size_t bytes_available = cache_stream->size - cache_stream->position;

    size_t bytes_to_copy = bytes_available > bytes_requested ? bytes_requested : bytes_available;

    memcpy(buffer, cache_stream->ptr + cache_stream->position, bytes_to_copy);

    cache_stream->position += bytes_to_copy;

    return bytes_to_copy;
}

static bool _cache_seek(void *stream, long offset, int whence)
{
    Storage_Cache_Stream_t *cache_stream = (Storage_Cache_Stream_t *)stream;

    long position;

    switch (whence) {
        default:
        case SEEK_SET:
            position = offset;
            break;
        case SEEK_CUR:
            position = (long)cache_stream->position + offset;
            break;
        case SEEK_END:
            position = (long)(cache_stream->size - 1) + offset;
            break;
    }

    if (position < 0 || (size_t)position >= cache_stream->size) {
        return false;
    }

    cache_stream->position = (size_t)position;

    return true;
}

static long _cache_tell(void *stream)
{
    Storage_Cache_Stream_t *cache_stream = (Storage_Cache_Stream_t *)stream;

    return (long)cache_stream->position;
}

static bool _cache_eof(void *stream)
{
    Storage_Cache_Stream_t *cache_stream = (Storage_Cache_Stream_t *)stream;

    return cache_stream->position >= cache_stream->size;
}

Storage_Cache_t *Storage_Cache_create(FS_Context_t *context)
{
    Storage_Cache_t *cache = malloc(sizeof(Storage_Cache_t));
    if (!cache) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate storage cache");
        return NULL;
    }

    *cache = (Storage_Cache_t){ 0 };

    sh_new_arena(cache->entries); // Use `sh_new_arena()` for string hashmaps that you never delete from.
    if (!cache->entries) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate storage cache entries");
        goto error_free;
    }

    bool attached = FS_attach_from_callbacks(context, (FS_Callbacks_t){
            .scan = _cache_scan,
            .contains = _cache_contains,
            .open = _cache_open,
            .close = _cache_close,
            .size = _cache_size,
            .read = _cache_read,
            .seek = _cache_seek,
            .tell = _cache_tell,
            .eof = _cache_eof
        }, cache);
    if (!attached) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't attach storage cache callbacks");
        goto error_free_entries;
    }

    return cache;

error_free_entries:
    shfree(cache->entries);
error_free:
    free(cache);
    return NULL;
}

void Storage_Cache_destroy(Storage_Cache_t *cache)
{
    for (size_t i = 0; i < shlenu(cache->entries); ++i) {
        free(cache->entries[i].value.data);
    }
    shfree(cache->entries);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "storage cache entries freed");

    free(cache);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "storage cache freed");
}

bool Storage_Cache_inject_base64(Storage_Cache_t *cache, const char *name, const char *encoded_data, size_t length)
{
    bool valid = base64_is_valid(encoded_data);
    if (!valid) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "data `%.16s` is not Base64 encoded", encoded_data);
        return false;
    }

    size_t size = base64_decoded_size(encoded_data);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "Base64 data `%.32s` is %d byte(s) long", encoded_data, size);

    void *data = malloc(sizeof(char) * size);
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d byte(s) buffer for decoding data `%.16s`", size, encoded_data);
        return false;
    }
    
    base64_decode(data, size, encoded_data);

    Storage_Cache_Entry_Value_t value = (Storage_Cache_Entry_Value_t){ .data = data, .size = size };
    shput(cache->entries, name, value);

    return true;
}

bool Storage_Cache_inject_ascii85(Storage_Cache_t *cache, const char *name, const char *encoded_data, size_t length)
{
    int32_t max_size = ascii85_get_max_decoded_length(strlen(encoded_data));
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "Ascii85 data `%.32s` is %d byte(s) long", encoded_data, max_size);

    void *data = malloc(sizeof(char) * max_size);
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d byte(s) buffer for decoding data `%.16s`", max_size, encoded_data);
        return false;
    }

    int32_t size = ascii85_decode(encoded_data, length, data, max_size);
    if (size < 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "data `%.16s` can't be decoded as Ascii85", encoded_data);
        free(data);
        return false;
    }

    Storage_Cache_Entry_Value_t value = (Storage_Cache_Entry_Value_t){ .data = data, .size = (size_t)size };
    shput(cache->entries, name, value);

    return true;
}

bool Storage_Cache_inject_raw(Storage_Cache_t *cache, const char *name, const void *raw_data, size_t size)
{
    void *data = stb_memdup(raw_data, size);
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d byte(s) buffer for data %p", size, data);
        return false;
    }
    
    Storage_Cache_Entry_Value_t value = (Storage_Cache_Entry_Value_t){ .data = data, .size = size };
    shput(cache->entries, name, value);

    return true;
}
