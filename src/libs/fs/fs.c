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

#include "fs.h"

#include "internals.h"
#include "pak.h"
#include "std.h"

#include <libs/log.h>
#include <libs/stb.h>

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>

#define LOG_CONTEXT "fs"

#if PLATFORM_ID == PLATFORM_WINDOWS
  #define realpath(N,R) _fullpath((R),(N),PATH_MAX)
#endif

static File_System_Mount_t *_mount(const char *path)
{
    if (std_is_valid(path)) {
        return std_mount(path);
    } else
    if (pak_is_valid(path)) {
        return pak_mount(path);
    } else {
        return NULL;
    }
}

static void _unmount(File_System_Mount_t *mount)
{
    ((Mount_t *)mount)->vtable.dtor(mount);
    free(mount);
}

static bool _attach(File_System_t *file_system, const char *path)
{
    File_System_Mount_t *mount = _mount(path); // Path need to be already resolved.
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't attach mount-point `%s`", path);
        return false;
    }

    arrpush(file_system->mounts, mount);

    return true;
}

bool FS_initialize(File_System_t *file_system, const char *base_path)
{
    *file_system = (File_System_t){ 0 };

    char resolved[FILE_PATH_MAX]; // Using local buffer to avoid un-tracked `malloc()` for the syscall.
    char *ptr = realpath(base_path ? base_path : FILE_PATH_CURRENT_SZ, resolved);
    if (!ptr) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't resolve `%s`", base_path);
        return false;
    }

    DIR *dp = opendir(resolved);
    if (dp) { // Path is a folder, scan and mount valid archives.
        for (struct dirent *entry = readdir(dp); entry; entry = readdir(dp)) {
            char full_path[FILE_PATH_MAX];
            strcpy(full_path, resolved);
            strcat(full_path, FILE_PATH_SEPARATOR_SZ);
            strcat(full_path, entry->d_name);

            if (!pak_is_valid(full_path)) {
                continue;
            }

            _attach(file_system, full_path);
        }

        closedir(dp);
    }

    _attach(file_system, resolved);

    return true;
}

void FS_terminate(File_System_t *file_system)
{
    size_t count = arrlen(file_system->mounts);
    for (int i = count - 1; i >= 0; --i) {
        File_System_Mount_t *mount = file_system->mounts[i];
        _unmount(mount);
    }
    arrfree(file_system->mounts);
}

bool FS_attach(File_System_t *file_system, const char *path)
{
    char resolved[FILE_PATH_MAX]; // Using local buffer to avoid un-tracked `malloc()` for the syscall.
    char *ptr = realpath(path ? path : FILE_PATH_CURRENT_SZ, resolved);
    if (!ptr) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't resolve `%s`", path);
        return false;
    }

    return _attach(file_system, resolved);
}

File_System_Mount_t *FS_locate(const File_System_t *file_system, const char *file)
{
    size_t count = arrlen(file_system->mounts);
    for (int i = count - 1; i >= 0; --i) {
        File_System_Mount_t *mount = file_system->mounts[i];
        if (((Mount_t *)mount)->vtable.contains(mount, file)) {
            return mount;
        }
    }

    return NULL;
}

File_System_Handle_t *FS_open(File_System_Mount_t *mount, const char *file)
{
    return ((Mount_t *)mount)->vtable.open(mount, file);
}

void FS_close(File_System_Handle_t *handle)
{
    ((Handle_t *)handle)->vtable.dtor(handle);
    free(handle);
}

size_t FS_size(File_System_Handle_t *handle)
{
    return ((Handle_t *)handle)->vtable.size(handle);
}

size_t FS_read(File_System_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    return ((Handle_t *)handle)->vtable.read(handle, buffer, bytes_requested);
}

void FS_skip(File_System_Handle_t *handle, int offset)
{
    ((Handle_t *)handle)->vtable.skip(handle, offset);
}

bool FS_eof(File_System_Handle_t *handle)
{
    return ((Handle_t *)handle)->vtable.eof(handle);
}
