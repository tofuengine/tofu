/*
 * MIT License
 * 
 * Copyright (c) 2019-2021 Marco Lizza
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

#include "std.h"

#include "internals.h"

#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>

#define LOG_CONTEXT "fs-cache"

typedef struct Std_Mount_s {
    Mount_VTable_t vtable; // Matches `_FS_Mount_t` structure.
    FS_Cache_Callbacks_t callbacks;
    void *user_data;
} Cache_Mount_t;

typedef struct Std_Handle_s {
    Handle_VTable_t vtable; // Matches `_FS_Handle_t` structure.
    FS_Cache_Callbacks_t callbacks;
    void *stream;
} Cache_Handle_t;

static void _cache_mount_ctor(FS_Mount_t *mount, FS_Cache_Callbacks_t callbacks, void *user_data);
static void _cache_mount_dtor(FS_Mount_t *mount);
static void _cache_mount_scan(const FS_Mount_t *mount, FS_Scan_Callback_t callback, void *user_data);
static bool _cache_mount_contains(const FS_Mount_t *mount, const char *name);
static FS_Handle_t *_cache_mount_open(const FS_Mount_t *mount, const char *name);

static void _cache_handle_ctor(FS_Handle_t *handle, FS_Cache_Callbacks_t callbacks, void *stream);
static void _cache_handle_dtor(FS_Handle_t *handle);
static size_t _cache_handle_size(FS_Handle_t *handle);
static size_t _cache_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested);
static bool _cache_handle_seek(FS_Handle_t *handle, long offset, int whence);
static long _cache_handle_tell(FS_Handle_t *handle);
static bool _cache_handle_eof(FS_Handle_t *handle);

FS_Mount_t *FS_cache_mount(FS_Cache_Callbacks_t callbacks, void *user_data)
{
    FS_Mount_t *mount = malloc(sizeof(Cache_Mount_t));
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate mount for cache w/ user-data %p", user_data);
        return NULL;
    }

    _cache_mount_ctor(mount, callbacks, user_data);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "mount %p initialized as cache w/ user-data %p", mount, user_data);

    return mount;
}

static void _cache_mount_ctor(FS_Mount_t *mount, FS_Cache_Callbacks_t callbacks, void *user_data)
{
    Cache_Mount_t *cache_mount = (Cache_Mount_t *)mount;

    *cache_mount = (Cache_Mount_t){
            .vtable = (Mount_VTable_t){
                .dtor = _cache_mount_dtor,
                .scan = _cache_mount_scan,
                .contains = _cache_mount_contains,
                .open = _cache_mount_open
            },
            .callbacks = callbacks,
            .user_data = user_data
        };
}

static void _cache_mount_dtor(FS_Mount_t *mount)
{
    Cache_Mount_t *cache_mount = (Cache_Mount_t *)mount;

    *cache_mount = (Cache_Mount_t){ 0 };
}

static void _cache_mount_scan(const FS_Mount_t *mount, FS_Scan_Callback_t callback, void *user_data)
{
    Cache_Mount_t *cache_mount = (Cache_Mount_t *)mount;

    cache_mount->callbacks.scan(cache_mount->user_data, callback, user_data);
}

static bool _cache_mount_contains(const FS_Mount_t *mount, const char *name)
{
    Cache_Mount_t *cache_mount = (Cache_Mount_t *)mount;

    return cache_mount->callbacks.contains(cache_mount->user_data, name);
}

static FS_Handle_t *_cache_mount_open(const FS_Mount_t *mount, const char *name)
{
    Cache_Mount_t *cache_mount = (Cache_Mount_t *)mount;

    FS_Handle_t *handle = malloc(sizeof(Cache_Handle_t));
    if (!handle) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate handle for file `%s`", name);
        return NULL;
    }

    void *stream = cache_mount->callbacks.open(cache_mount->user_data, name);

    _cache_handle_ctor(handle, cache_mount->callbacks, stream);

    return handle;
}

static void _cache_handle_ctor(FS_Handle_t *handle, FS_Cache_Callbacks_t callbacks, void *stream)
{
    Cache_Handle_t *cache_handle = (Cache_Handle_t *)handle;

    *cache_handle = (Cache_Handle_t){
            .vtable = (Handle_VTable_t){
                .dtor = _cache_handle_dtor,
                .size = _cache_handle_size,
                .read = _cache_handle_read,
                .seek = _cache_handle_seek,
                .tell = _cache_handle_tell,
                .eof = _cache_handle_eof
            },
            .callbacks = callbacks,
            .stream = stream
        };
}

static void _cache_handle_dtor(FS_Handle_t *handle)
{
    Cache_Handle_t *cache_handle = (Cache_Handle_t *)handle;

    cache_handle->callbacks.close(cache_handle->stream);

    *cache_handle = (Cache_Handle_t){ 0 };
}

static size_t _cache_handle_size(FS_Handle_t *handle)
{
    Cache_Handle_t *cache_handle = (Cache_Handle_t *)handle;

    return cache_handle->callbacks.size(cache_handle->stream);
}

static size_t _cache_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    Cache_Handle_t *cache_handle = (Cache_Handle_t *)handle;

    size_t bytes_read = cache_handle->callbacks.read(cache_handle->stream, buffer, bytes_requested);
#ifdef __DEBUG_FS_CALLS__
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes read for handle %p", bytes_read, handle);
#endif
    return bytes_read;
}

static bool _cache_handle_seek(FS_Handle_t *handle, long offset, int whence)
{
    Cache_Handle_t *cache_handle = (Cache_Handle_t *)handle;

    bool seeked = cache_handle->callbacks.seek(cache_handle->stream, offset, whence);
#ifdef __DEBUG_FS_CALLS__
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes seeked w/ mode %d for handle %p w/ result %d", offset, whence, handle, seeked);
#endif
    return seeked;
}

static long _cache_handle_tell(FS_Handle_t *handle)
{
    Cache_Handle_t *cache_handle = (Cache_Handle_t *)handle;

    return cache_handle->callbacks.tell(cache_handle->stream);
}

static bool _cache_handle_eof(FS_Handle_t *handle)
{
    Cache_Handle_t *cache_handle = (Cache_Handle_t *)handle;

    bool end_of_file =  cache_handle->callbacks.eof(cache_handle->stream);
#ifdef __DEBUG_FS_CALLS__
    Log_assert(!end_of_file, LOG_LEVELS_DEBUG, LOG_CONTEXT, "end-of-file reached for handle %p", handle);
#endif
    return end_of_file;
}
