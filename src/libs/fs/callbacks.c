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

#include "callbacks.h"

#include "internal.h"

#include <core/config.h>
#define _LOG_TAG "fs-callbacks"
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>

typedef struct Cache_Mount_s {
    Mount_VTable_t vtable; // Matches `FS_Mount_t` structure.
    FS_Callbacks_t callbacks;
    void *user_data;
} Cache_Mount_t;

typedef struct Cache_Handle_s {
    Handle_VTable_t vtable; // Matches `FS_Handle_t` structure.
    FS_Callbacks_t callbacks;
    void *stream;
} Cache_Handle_t;

static void _callbacks_mount_ctor(FS_Mount_t *mount, FS_Callbacks_t callbacks, void *user_data);
static void _callbacks_mount_dtor(FS_Mount_t *mount);
static bool _callbacks_mount_contains(const FS_Mount_t *mount, const char *name);
static FS_Handle_t *_callbacks_mount_open(const FS_Mount_t *mount, const char *name);

static void _callbacks_handle_ctor(FS_Handle_t *handle, FS_Callbacks_t callbacks, void *stream);
static void _callbacks_handle_dtor(FS_Handle_t *handle);
static size_t _callbacks_handle_size(const FS_Handle_t *handle);
static size_t _callbacks_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested);
static bool _callbacks_handle_seek(FS_Handle_t *handle, long offset, int whence);
static long _callbacks_handle_tell(const FS_Handle_t *handle);
static bool _callbacks_handle_eof(const FS_Handle_t *handle);

FS_Mount_t *FS_callbacks_mount(FS_Callbacks_t callbacks, void *user_data)
{
    FS_Mount_t *mount = malloc(sizeof(Cache_Mount_t));
    if (!mount) {
        LOG_E("can't allocate mount for cache w/ user-data %p", user_data);
        return NULL;
    }

    _callbacks_mount_ctor(mount, callbacks, user_data);

    return mount;
}

static void _callbacks_mount_ctor(FS_Mount_t *mount, FS_Callbacks_t callbacks, void *user_data)
{
    Cache_Mount_t *cache_mount = (Cache_Mount_t *)mount;

    *cache_mount = (Cache_Mount_t){
            .vtable = (Mount_VTable_t){
                .dtor = _callbacks_mount_dtor,
                .contains = _callbacks_mount_contains,
                .open = _callbacks_mount_open
            },
            .callbacks = callbacks,
            .user_data = user_data
        };

    LOG_T("mount %p initialized as cache w/ user-data %p", mount, user_data);
}

static void _callbacks_mount_dtor(FS_Mount_t *mount)
{
    Cache_Mount_t *cache_mount = (Cache_Mount_t *)mount;

    *cache_mount = (Cache_Mount_t){ 0 };

    LOG_T("mount %p uninitialized", mount);
}

static bool _callbacks_mount_contains(const FS_Mount_t *mount, const char *name)
{
    Cache_Mount_t *cache_mount = (Cache_Mount_t *)mount;

    return cache_mount->callbacks.contains(cache_mount->user_data, name);
}

static FS_Handle_t *_callbacks_mount_open(const FS_Mount_t *mount, const char *name)
{
    Cache_Mount_t *cache_mount = (Cache_Mount_t *)mount;

    FS_Handle_t *handle = malloc(sizeof(Cache_Handle_t));
    if (!handle) {
        LOG_E("can't allocate handle for file `%s`", name);
        return NULL;
    }

    void *stream = cache_mount->callbacks.open(cache_mount->user_data, name);

    _callbacks_handle_ctor(handle, cache_mount->callbacks, stream);

    return handle;
}

static void _callbacks_handle_ctor(FS_Handle_t *handle, FS_Callbacks_t callbacks, void *stream)
{
    Cache_Handle_t *cache_handle = (Cache_Handle_t *)handle;

    *cache_handle = (Cache_Handle_t){
            .vtable = (Handle_VTable_t){
                .dtor = _callbacks_handle_dtor,
                .size = _callbacks_handle_size,
                .read = _callbacks_handle_read,
                .seek = _callbacks_handle_seek,
                .tell = _callbacks_handle_tell,
                .eof = _callbacks_handle_eof
            },
            .callbacks = callbacks,
            .stream = stream
        };

    LOG_T("handle %p initialized", handle);
}

static void _callbacks_handle_dtor(FS_Handle_t *handle)
{
    Cache_Handle_t *cache_handle = (Cache_Handle_t *)handle;

    cache_handle->callbacks.close(cache_handle->stream);

    *cache_handle = (Cache_Handle_t){ 0 };

    LOG_T("handle %p uninitialized", handle);
}

static size_t _callbacks_handle_size(const FS_Handle_t *handle)
{
    const Cache_Handle_t *cache_handle = (const Cache_Handle_t *)handle;

    return cache_handle->callbacks.size(cache_handle->stream);
}

static size_t _callbacks_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    Cache_Handle_t *cache_handle = (Cache_Handle_t *)handle;

    size_t bytes_read = cache_handle->callbacks.read(cache_handle->stream, buffer, bytes_requested);
#if defined(TOFU_FILE_DEBUG_ENABLED)
    LOG_D("%d bytes read for handle %p", bytes_read, handle);
#endif
    return bytes_read;
}

static bool _callbacks_handle_seek(FS_Handle_t *handle, long offset, int whence)
{
    Cache_Handle_t *cache_handle = (Cache_Handle_t *)handle;

    bool sought = cache_handle->callbacks.seek(cache_handle->stream, offset, whence);
#if defined(TOFU_FILE_DEBUG_ENABLED)
    LOG_D("%d bytes sought w/ mode %d for handle %p w/ result %d", offset, whence, handle, sought);
#endif
    return sought;
}

static long _callbacks_handle_tell(const FS_Handle_t *handle)
{
    const Cache_Handle_t *cache_handle = (const Cache_Handle_t *)handle;

    return cache_handle->callbacks.tell(cache_handle->stream);
}

static bool _callbacks_handle_eof(const FS_Handle_t *handle)
{
    const Cache_Handle_t *cache_handle = (const Cache_Handle_t *)handle;

    bool end_of_file =  cache_handle->callbacks.eof(cache_handle->stream);
#if defined(TOFU_FILE_DEBUG_ENABLED)
    LOG_IF_D(end_of_file, "end-of-file reached for handle %p", handle);
#endif
    return end_of_file;
}
