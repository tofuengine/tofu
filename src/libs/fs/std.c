/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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
#include <libs/stb.h>

#include <sys/stat.h>
#include <unistd.h>

#define LOG_CONTEXT "fs-std"

typedef struct _Std_Mount_t {
    Mount_VTable_t vtable; // Matches `_FS_Mount_t` structure.
    char base_path[FILE_PATH_MAX];
} Std_Mount_t;

typedef struct _Std_Handle_t {
    Handle_VTable_t vtable; // Matches `_FS_Handle_t` structure.
    FILE *stream;
} Std_Handle_t;

static void _std_mount_ctor(FS_Mount_t *mount, const char *base_path);
static void _std_mount_dtor(FS_Mount_t *mount);
static bool _std_mount_contains(const FS_Mount_t *mount, const char *file);
static FS_Handle_t *_std_mount_open(const FS_Mount_t *mount, const char *file);

static void _std_handle_ctor(FS_Handle_t *handle, FILE *stream);
static void _std_handle_dtor(FS_Handle_t *handle);
static size_t _std_handle_size(FS_Handle_t *handle);
static size_t _std_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested);
static bool _std_handle_seek(FS_Handle_t *handle, long offset, int whence);
static long _std_handle_tell(FS_Handle_t *handle);
static bool _std_handle_eof(FS_Handle_t *handle);

bool FS_std_is_valid(const char *path)
{
    struct stat path_stat;
    int result = stat(path, &path_stat);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get stats for `%s`", path);
        return false;
    }

    return S_ISDIR(path_stat.st_mode);
}

FS_Mount_t *FS_std_mount(const char *path)
{
    FS_Mount_t *mount = malloc(sizeof(Std_Mount_t));
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate mount for folder `%s`", path);
        return NULL;
    }

    _std_mount_ctor(mount, path);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "mount %p initialized at folder `%s`", mount, path);

    return mount;
}

static void _std_mount_ctor(FS_Mount_t *mount, const char *base_path)
{
    Std_Mount_t *std_mount = (Std_Mount_t *)mount;

    *std_mount = (Std_Mount_t){
            .vtable = (Mount_VTable_t){
                .dtor = _std_mount_dtor,
                .contains = _std_mount_contains,
                .open = _std_mount_open
            },
            .base_path = { 0 }
        };

    strcpy(std_mount->base_path, base_path);
}

static void _std_mount_dtor(FS_Mount_t *mount)
{
    Std_Mount_t *std_mount = (Std_Mount_t *)mount;

    *std_mount = (Std_Mount_t){ 0 };
}

static bool _std_mount_contains(const FS_Mount_t *mount, const char *file)
{
    const Std_Mount_t *std_mount = (const Std_Mount_t *)mount;

    char full_path[FILE_PATH_MAX];
    strcpy(full_path, std_mount->base_path);
    strcat(full_path, FILE_PATH_SEPARATOR_SZ);
    strcat(full_path, file);
    for (size_t i = 0; full_path[i] != '\0'; ++i) { // Replace virtual file-system separtor `/` with the actual one.
        if (full_path[i] == FS_PATH_SEPARATOR) {
            full_path[i] = FILE_PATH_SEPARATOR;
        }
    } // FIXME: better organize name normalization.

    bool exists = access(full_path, R_OK) != -1;
    Log_assert(!exists, LOG_LEVELS_DEBUG, LOG_CONTEXT, "file `%s` found in mount %p", file, mount);
    return exists;
}

static FS_Handle_t *_std_mount_open(const FS_Mount_t *mount, const char *file)
{
    const Std_Mount_t *std_mount = (const Std_Mount_t *)mount;

    char full_path[FILE_PATH_MAX];
    strcpy(full_path, std_mount->base_path);
    strcat(full_path, FILE_PATH_SEPARATOR_SZ);
    strcat(full_path, file);
    for (size_t i = 0; full_path[i] != '\0'; ++i) { // Replace virtual file-system separtor `/` with the actual one.
        if (full_path[i] == FS_PATH_SEPARATOR) {
            full_path[i] = FILE_PATH_SEPARATOR;
        }
    }

    FILE *stream = fopen(full_path, "rb");
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", full_path);
        return NULL;
    }

    FS_Handle_t *handle = malloc(sizeof(Std_Handle_t));
    if (!handle) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate handle for file `%s`", file);
        fclose(stream);
        return NULL;
    }

    _std_handle_ctor(handle, stream);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "file `%s` opened w/ handle %p", file, handle);

    return handle;
}

static void _std_handle_ctor(FS_Handle_t *handle, FILE *stream)
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
            .stream = stream
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

    struct stat stat;
    int result = fstat(fileno(std_handle->stream), &stat);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get stats for handle %p", handle);
        return 0;
    }

//    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p is", std_handle);

    return (size_t)stat.st_size;
}

static size_t _std_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    size_t bytes_read = fread(buffer, sizeof(char), bytes_requested, std_handle->stream);

#ifdef __DEBUG_FS_CALLS__
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes read for handle %p", bytes_read, handle);
#endif
    return bytes_read;
}

static bool _std_handle_seek(FS_Handle_t *handle, long offset, int whence)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    bool seeked = fseek(std_handle->stream, offset, whence) == 0;
#ifdef __DEBUG_FS_CALLS__
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes seeked w/ mode %d for handle %p w/ result %d", offset, whence, handle, seeked);
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
    Log_assert(!end_of_file, LOG_LEVELS_DEBUG, LOG_CONTEXT, "end-of-file reached for handle %p", handle);
#endif
    return end_of_file;
}
