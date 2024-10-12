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

#include "std.h"

#include "internal.h"

#include <core/config.h>
#include <core/platform.h>
#define _LOG_TAG "fs-std"
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>

#include <dirent.h>
#include <sys/stat.h>

typedef struct Std_Mount_s {
    Mount_VTable_t vtable; // Matches `FS_Mount_t` structure.
    char path[PLATFORM_PATH_MAX];
} Std_Mount_t;

typedef struct Std_Handle_s {
    Handle_VTable_t vtable; // Matches `FS_Handle_t` structure.
    FILE *stream;
    size_t size;
} Std_Handle_t;

static void _std_mount_ctor(FS_Mount_t *mount, const char *path);
static void _std_mount_dtor(FS_Mount_t *mount);
static bool _std_mount_contains(const FS_Mount_t *mount, const char *name);
static FS_Handle_t *_std_mount_open(const FS_Mount_t *mount, const char *name);

static void _std_handle_ctor(FS_Handle_t *handle, FILE *stream, size_t size);
static void _std_handle_dtor(FS_Handle_t *handle);
static size_t _std_handle_size(const FS_Handle_t *handle);
static size_t _std_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested);
static bool _std_handle_seek(FS_Handle_t *handle, long offset, int whence);
static long _std_handle_tell(const FS_Handle_t *handle);
static bool _std_handle_eof(const FS_Handle_t *handle);

bool FS_std_is_valid(const char *path)
{
    return path_is_folder(path);
}

// Precondition: the path need to be pre-validated as being a folder.
FS_Mount_t *FS_std_mount(const char *path)
{
    FS_Mount_t *mount = malloc(sizeof(Std_Mount_t));
    if (!mount) {
        LOG_E("can't allocate mount for folder `%s`", path);
        return NULL;
    }

    _std_mount_ctor(mount, path);

    return mount;
}

static void _std_mount_ctor(FS_Mount_t *mount, const char *path)
{
    Std_Mount_t *std_mount = (Std_Mount_t *)mount;

    *std_mount = (Std_Mount_t){
            .vtable = (Mount_VTable_t){
                .dtor = _std_mount_dtor,
                .contains = _std_mount_contains,
                .open = _std_mount_open
            },
            .path = { 0 }
        };

    strncpy(std_mount->path, path, PLATFORM_PATH_MAX - 1);

    LOG_T("mount %p initialized at folder `%s`", mount, path);
}

static void _std_mount_dtor(FS_Mount_t *mount)
{
    Std_Mount_t *std_mount = (Std_Mount_t *)mount;

    *std_mount = (Std_Mount_t){ 0 };

    LOG_T("mount %p uninitialized", mount);
}

static bool _std_mount_contains(const FS_Mount_t *mount, const char *name)
{
    const Std_Mount_t *std_mount = (const Std_Mount_t *)mount;

    char path[PLATFORM_PATH_MAX] = { 0 };
    path_join(path, std_mount->path, name);

    bool exists = path_exists(path);
    LOG_IF_D(exists, "file `%s` found in mount %p", name, mount);
    return exists;
}

static size_t _size(FILE *stream)
{
    fseek(stream, 0L, SEEK_END);
    size_t size = (size_t)ftell(stream);
#if defined(TOFU_FILE_DEBUG_ENABLED)
    LOG_D("stream %p is %d bytes long", stream, size);
#endif
    fseek(stream, 0L, SEEK_SET);

    return size;
}

static FS_Handle_t *_std_mount_open(const FS_Mount_t *mount, const char *name)
{
    const Std_Mount_t *std_mount = (const Std_Mount_t *)mount;

    char path[PLATFORM_PATH_MAX] = { 0 };
    path_join(path, std_mount->path, name);

    FILE *stream = fopen(path, "rb");
    if (!stream) {
        LOG_E("can't access file `%s`", path);
        goto error_exit;
    }

    FS_Handle_t *handle = malloc(sizeof(Std_Handle_t));
    if (!handle) {
        LOG_E("can't allocate handle for file `%s`", name);
        goto error_close_stream;
    }

    _std_handle_ctor(handle, stream, _size(stream));

    LOG_D("file `%s` opened w/ handle %p", name, handle);

    return handle;

error_close_stream:
    fclose(stream);
error_exit:
    return NULL;
}

static void _std_handle_ctor(FS_Handle_t *handle, FILE *stream, size_t size)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    *std_handle = (Std_Handle_t){
            .vtable = (Handle_VTable_t){
                .dtor = _std_handle_dtor,
                .size = _std_handle_size,
                .read = _std_handle_read,
                .seek = _std_handle_seek,
                .tell = _std_handle_tell,
                .eof = _std_handle_eof
            },
            .stream = stream,
            .size = size
        };

    LOG_T("handle %p initialized (size is %u bytes)", handle, size);
}

static void _std_handle_dtor(FS_Handle_t *handle)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    fclose(std_handle->stream);

    LOG_T("handle %p uninitialized", handle);
}

static size_t _std_handle_size(const FS_Handle_t *handle)
{
    const Std_Handle_t *std_handle = (const Std_Handle_t *)handle;

    return std_handle->size;
}

static size_t _std_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    size_t bytes_read = fread(buffer, sizeof(char), bytes_requested, std_handle->stream);
#if defined(TOFU_FILE_DEBUG_ENABLED)
    LOG_D("%d bytes read for handle %p", bytes_read, handle);
#endif
    return bytes_read;
}

static bool _std_handle_seek(FS_Handle_t *handle, long offset, int whence)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    bool sought = fseek(std_handle->stream, offset, whence) == 0;
#if defined(TOFU_FILE_DEBUG_ENABLED)
    LOG_D("%d bytes sought w/ mode %d for handle %p w/ result %d", offset, whence, handle, sought);
#endif
    return sought;
}

static long _std_handle_tell(const FS_Handle_t *handle)
{
    const Std_Handle_t *std_handle = (const Std_Handle_t *)handle;

    return ftell(std_handle->stream);
}

static bool _std_handle_eof(const FS_Handle_t *handle)
{
    const Std_Handle_t *std_handle = (const Std_Handle_t *)handle;

    bool end_of_file = feof(std_handle->stream) != 0;
#if defined(TOFU_FILE_DEBUG_ENABLED)
    LOG_IF_D(end_of_file, "end-of-file reached for handle %p", handle);
#endif
    return end_of_file;
}
