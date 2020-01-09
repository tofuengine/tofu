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

#include <libs/log.h>
#include <libs/stb.h>

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#define LOG_CONTEXT "fs-std"

typedef struct _Std_Mount_t {
    // v-table
    void  (*dtor)                (File_System_Mount_t *mount);
    bool  (*contains)            (File_System_Mount_t *mount, const char *file);
    File_System_Handle_t *(*open)(File_System_Mount_t *mount, const char *file);
    // data
    char base_path[FILE_PATH_MAX];
} Std_Mount_t;

typedef struct _Std_Handle_t {
    // v-table
    void   (*dtor)(File_System_Handle_t *handle);
    size_t (*size)(File_System_Handle_t *handle);
    size_t (*read)(File_System_Handle_t *handle, void *buffer, size_t bytes_requested);
    void   (*skip)(File_System_Handle_t *handle, int offset);
    bool   (*eof) (File_System_Handle_t *handle);
    // data
    FILE *stream;
} Std_Handle_t;

static void _std_handle_dtor(File_System_Handle_t *handle)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    fclose(std_handle->stream);

    *std_handle = (Std_Handle_t){ 0 };

    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "handle %p deinitialized", handle);
}

static size_t _std_handle_size(File_System_Handle_t *handle)
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

static size_t _std_handle_read(File_System_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    size_t bytes_read = fread(buffer, sizeof(char), bytes_requested, std_handle->stream);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes read for handle %p", bytes_read, handle);
    return bytes_read;
}

static void _std_handle_skip(File_System_Handle_t *handle, int offset)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    fseek(std_handle->stream, offset, SEEK_CUR);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes seeked for handle %p", offset, handle);
}

static bool _std_handle_eof(File_System_Handle_t *handle)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    bool end_of_file = feof(std_handle->stream) != 0;
    Log_assert(!end_of_file, LOG_LEVELS_DEBUG, LOG_CONTEXT, "end-of-file reached for handle %p", handle);
    return end_of_file;
}

static void _std_handle_ctor(File_System_Handle_t *handle, FILE *stream)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    *std_handle = (Std_Handle_t){
            .dtor = _std_handle_dtor,
            .size = _std_handle_size,
            .read = _std_handle_read,
            .skip = _std_handle_skip,
            .eof = _std_handle_eof
        };

    std_handle->stream = stream;

    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "handle %p initialized", handle);
}

static void _std_mount_dtor(File_System_Mount_t *mount)
{
    Std_Mount_t *std_mount = (Std_Mount_t *)mount;

    *std_mount = (Std_Mount_t){ 0 };

    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "mount %p deinitialized", mount);
}

static bool _std_mount_contains(File_System_Mount_t *mount, const char *file)
{
    Std_Mount_t *std_mount = (Std_Mount_t *)mount;

    char full_path[FILE_PATH_MAX];
    strcpy(full_path, std_mount->base_path);
    strcat(full_path, file);

    bool exists = access(full_path, R_OK) != -1;
    Log_assert(!exists, LOG_LEVELS_DEBUG, LOG_CONTEXT, "file `%s` found in mount %p", file, mount);
    return exists;
}

static File_System_Handle_t *_std_mount_open(File_System_Mount_t *mount, const char *file)
{
    Std_Mount_t *std_mount = (Std_Mount_t *)mount;

    char full_path[FILE_PATH_MAX];
    strcpy(full_path, std_mount->base_path);
    strcat(full_path, file);

    FILE *stream = fopen(full_path, "rb");
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", full_path);
        return NULL;
    }

    File_System_Handle_t *handle = malloc(sizeof(Std_Handle_t));
    if (!handle) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate handle for file `%s`", file);
        fclose(stream);
        return NULL;
    }

    _std_handle_ctor(handle, stream);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "file `%s` opened w/ handle %p", file, handle);

    return handle;
}

static void _std_mount_ctor(File_System_Mount_t *mount, const char *base_path)
{
    Std_Mount_t *std_mount = (Std_Mount_t *)mount;

    *std_mount = (Std_Mount_t){
            .dtor = _std_mount_dtor,
            .contains = _std_mount_contains,
            .open = _std_mount_open
        };

    strcpy(std_mount->base_path, base_path); // The path *need* to be terminated with the file path-separator!!!

    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "mount %p initialized", mount);
}

bool std_is_valid(const char *path)
{
    struct stat path_stat;
    int result = stat(path, &path_stat);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get stats for `%s`", path);
        return false;
    }

    return S_ISDIR(path_stat.st_mode);
}

File_System_Mount_t *std_mount(const char *path)
{
    File_System_Mount_t *mount = malloc(sizeof(Std_Mount_t));
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate mount for folder `%s`", path);
        return NULL;
    }

    _std_mount_ctor(mount, path);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "mount initialized at folder `%s`", path);

    return mount;
}
