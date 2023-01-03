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

#include "std.h"

#include "internal.h"

#include <core/platform.h>
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>

#include <dirent.h>
#include <sys/stat.h>

#define LOG_CONTEXT "fs-std"

typedef struct Std_Mount_s {
    Mount_VTable_t vtable; // Matches `_FS_Mount_t` structure.
    char path[PLATFORM_PATH_MAX];
} Std_Mount_t;

typedef struct Std_Handle_s {
    Handle_VTable_t vtable; // Matches `_FS_Handle_t` structure.
    FILE *stream;
    size_t size;
} Std_Handle_t;

static void _std_mount_ctor(FS_Mount_t *mount, const char *path);
static void _std_mount_dtor(FS_Mount_t *mount);
static void _std_mount_scan(const FS_Mount_t *mount, FS_Scan_Callback_t callback, void *user_data);
static bool _std_mount_contains(const FS_Mount_t *mount, const char *name);
static FS_Handle_t *_std_mount_open(const FS_Mount_t *mount, const char *name);

static void _std_handle_ctor(FS_Handle_t *handle, FILE *stream, size_t size);
static void _std_handle_dtor(FS_Handle_t *handle);
static size_t _std_handle_size(FS_Handle_t *handle);
static size_t _std_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested);
static bool _std_handle_seek(FS_Handle_t *handle, long offset, int whence);
static long _std_handle_tell(FS_Handle_t *handle);
static bool _std_handle_eof(FS_Handle_t *handle);

bool FS_std_is_valid(const char *path)
{
    return path_is_folder(path);
}

// Precondition: the path need to be pre-validated as being a folder.
FS_Mount_t *FS_std_mount(const char *path)
{
    FS_Mount_t *mount = malloc(sizeof(Std_Mount_t));
    if (!mount) {
        LOG_E(LOG_CONTEXT, "can't allocate mount for folder `%s`", path);
        return NULL;
    }

    _std_mount_ctor(mount, path);

    LOG_D(LOG_CONTEXT, "mount %p initialized at folder `%s`", mount, path);

    return mount;
}

static void _std_mount_ctor(FS_Mount_t *mount, const char *path)
{
    Std_Mount_t *std_mount = (Std_Mount_t *)mount;

    *std_mount = (Std_Mount_t){
            .vtable = (Mount_VTable_t){
                .dtor = _std_mount_dtor,
                .scan = _std_mount_scan,
                .contains = _std_mount_contains,
                .open = _std_mount_open
            },
            .path = { 0 }
        };

    strncpy(std_mount->path, path, PLATFORM_PATH_MAX - 1);
}

static void _std_mount_dtor(FS_Mount_t *mount)
{
    Std_Mount_t *std_mount = (Std_Mount_t *)mount;

    *std_mount = (Std_Mount_t){ 0 };
}

static void _read_directory(const char *path, size_t path_length, FS_Scan_Callback_t callback, void *user_data)
{
    DIR *dp = opendir(path);
    if (!dp) {
        return;
    }

    for (struct dirent *entry = readdir(dp); entry; entry = readdir(dp)) {
        if (strcasecmp(entry->d_name, "..") == 0 || strcasecmp(entry->d_name, ".") == 0) {
            continue;
        }

        char subpath[PLATFORM_PATH_MAX] = { 0 };
        path_join(subpath, path, entry->d_name);

        struct stat sb;
        int result = stat(subpath, &sb);
        if (result == -1) {
            LOG_E(LOG_CONTEXT, "can't stat file `%s`", subpath);
            continue;
        }

        if (S_ISDIR(sb.st_mode)) {
            _read_directory(subpath, path_length, callback, user_data);
        } else {
            callback(user_data, subpath + path_length + 1); // Skip base path and separator.
        }
    }

    closedir(dp);
}

static void _std_mount_scan(const FS_Mount_t *mount, FS_Scan_Callback_t callback, void *user_data)
{
    const Std_Mount_t *std_mount = (const Std_Mount_t *)mount;

    _read_directory(std_mount->path, strlen(std_mount->path), callback, user_data);
}

static bool _std_mount_contains(const FS_Mount_t *mount, const char *name)
{
    const Std_Mount_t *std_mount = (const Std_Mount_t *)mount;

    char path[PLATFORM_PATH_MAX] = { 0 };
    path_join(path, std_mount->path, name);

    bool exists = path_exists(path);
    LOG_IF_D(exists, LOG_CONTEXT, "file `%s` found in mount %p", name, mount);
    return exists;
}

static size_t _size(FILE *stream)
{
    fseek(stream, 0L, SEEK_END);
    size_t size = (size_t)ftell(stream);
#ifdef __DEBUG_FS_CALLS__
    LOG_D(LOG_CONTEXT, "handle %p is %d bytes long", handle, size);
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
        LOG_E(LOG_CONTEXT, "can't access file `%s`", path);
        return NULL;
    }

    FS_Handle_t *handle = malloc(sizeof(Std_Handle_t));
    if (!handle) {
        LOG_E(LOG_CONTEXT, "can't allocate handle for file `%s`", name);
        goto error_close;
    }

    _std_handle_ctor(handle, stream, _size(stream));

    LOG_D(LOG_CONTEXT, "file `%s` opened w/ handle %p", name, handle);

    return handle;

error_close:
    fclose(stream);
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
}

static void _std_handle_dtor(FS_Handle_t *handle)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    fclose(std_handle->stream);
}

static size_t _std_handle_size(FS_Handle_t *handle)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    return std_handle->size;
}

static size_t _std_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    size_t bytes_read = fread(buffer, sizeof(char), bytes_requested, std_handle->stream);
#ifdef __DEBUG_FS_CALLS__
    LOG_D(LOG_CONTEXT, "%d bytes read for handle %p", bytes_read, handle);
#endif
    return bytes_read;
}

static bool _std_handle_seek(FS_Handle_t *handle, long offset, int whence)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    bool seeked = fseek(std_handle->stream, offset, whence) == 0;
#ifdef __DEBUG_FS_CALLS__
    LOG_D(LOG_CONTEXT, "%d bytes seeked w/ mode %d for handle %p w/ result %d", offset, whence, handle, seeked);
#endif
    return seeked;
}

static long _std_handle_tell(FS_Handle_t *handle)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    return ftell(std_handle->stream);
}

static bool _std_handle_eof(FS_Handle_t *handle)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    bool end_of_file = feof(std_handle->stream) != 0;
#ifdef __DEBUG_FS_CALLS__
    LOG_IF_D(end_of_file, LOG_CONTEXT, "end-of-file reached for handle %p", handle);
#endif
    return end_of_file;
}
