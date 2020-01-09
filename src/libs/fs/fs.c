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

#include "pak.h"
#include "std.h"

#include <libs/log.h>
#include <libs/stb.h>

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>

#define LOG_CONTEXT "fs"

typedef struct _Mount_t {
    // v-table
    void  (*unmount)(void *mount);
    bool  (*exists)(void *mount, const char *file);
    void *(*open)  (void *mount, const char *file);
} Mount_t;

typedef struct _Handle_t {
    // v-table
    void   (*close)(void *handle);
    size_t (*size) (void *handle);
    size_t (*read) (void *handle, void *buffer, size_t bytes_requested);
    void   (*skip) (void *handle, int offset);
    bool   (*eof)  (void *handle);
} Handle_t;

static bool _mount(File_System_t *file_system, const char *path)
{
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "adding mount-point `%s`", path);

    struct stat path_stat;
    int result = stat(path, &path_stat);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get stats for `%s`", path);
        return NULL;
    }

    File_System_Mount_t *mount;
    if (S_ISDIR(path_stat.st_mode)) {
        mount = stdio_mount(path);
    } else
    if (S_ISREG(path_stat.st_mode) && pakio_is_archive(path)) {
        mount = pakio_mount(path);
    } else {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't detect type for mount `%s`", path);
        return false;
    }

    arrpush(file_system->mounts, mount);

    return true;
}

#if PLATFORM_ID == PLATFORM_WINDOWS
  #define realpath(N,R) _fullpath((R),(N),PATH_MAX)
#endif

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
    if (dp) { // Path is a folder, ensure trailing separator, then scan and mount valid archives.
        if (resolved[strlen(resolved) - 1] != '/') {
            strcat(resolved, FILE_PATH_SEPARATOR_SZ);
        }

        for (struct dirent *entry = readdir(dp); entry; entry = readdir(dp)) {
            char full_path[FILE_PATH_MAX];
            strcpy(full_path, resolved);
            strcat(full_path, entry->d_name);

            if (entry->d_type != DT_REG || !pakio_is_archive(full_path)) {
                Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "skipping file `%s` (not an archive)", full_path);
                continue;
            }

            _mount(file_system, full_path);

            // TODO: add also possible "archive.pa0", ..., "archive.p99" file
            // overriding "archive.pak".
    //        for (int i = 0; i < 100; ++i) {
    //            sprintf(&entry->d_name[length - 2], "%02d", i);
    //            _mount(file_system, full_path);
    //        }
        }

        closedir(dp);
    }

    _mount(file_system, resolved);

    return true;
}

void FS_terminate(File_System_t *file_system)
{
    size_t count = arrlen(file_system->mounts);
    for (int i = count - 1; i >= 0; --i) {
        Mount_t *mount = (Mount_t *)file_system->mounts[i];
        mount->unmount(mount);
    }
    arrfree(file_system->mounts);
}

bool FS_exists(const File_System_t *file_system, const char *file)
{
    size_t count = arrlen(file_system->mounts);
    for (int i = count - 1; i >= 0; --i) {
        Mount_t *mount = (Mount_t *)file_system->mounts[i];
        if (mount->exists(mount, file)) {
            return true;
        }
    }

    return false;
}

File_System_Handle_t *FS_open(const File_System_t *file_system, const char *file)
{
    size_t count = arrlen(file_system->mounts); // Backward search to enable resource override in multi-archives.
    for (int i = count - 1; i >= 0; --i) {
        Mount_t *mount = (Mount_t *)file_system->mounts[i];
        if (!mount->exists(mount, file)) {
            continue;
        }
        return ((Mount_t *)mount)->open(mount, file);
    }

    return NULL;
}

void FS_close(File_System_Handle_t *handle)
{
    ((Handle_t *)handle)->close(handle);
}

size_t FS_size(File_System_Handle_t *handle)
{
    return ((Handle_t *)handle)->size(handle);
}

size_t FS_read(File_System_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    return ((Handle_t *)handle)->read(handle, buffer, bytes_requested);
}

void FS_skip(File_System_Handle_t *handle, int offset)
{
    ((Handle_t *)handle)->skip(handle, offset);
}

bool FS_eof(File_System_Handle_t *handle)
{
    return ((Handle_t *)handle)->eof(handle);
}
