/*
 * Copyright (c) 2019-2020 by Marco Lizza (marco.lizza@gmail.com)
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
 **/

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

const File_System_Callbacks_t *_detect(const char *path)
{
    struct stat path_stat;
    int result = stat(path, &path_stat);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get stats for `%s`", path);
        return false;
    }

    return S_ISDIR(path_stat.st_mode) ? std_callbacks: pak_callbacks;
}

static bool _mount(File_System_t *file_system, const char *base_path)
{
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "adding mount-point `%s`", base_path);

    const File_System_Callbacks_t *callbacks = _detect(base_path);

    void *context = callbacks->init(base_path);
    if (!context) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize mount-point for path `%s`", base_path);
        return false;
    }

    File_System_Mount_t mount_point = (File_System_Mount_t){
            .callbacks = callbacks,
            .context = context
        };
    arrpush(file_system->mount_points, mount_point);

    return true;
}

// FIXME: convert bool argument to flags.
static void *_load(const File_System_Callbacks_t *callbacks, const void *context, const char *file, bool null_terminate, size_t *size)
{
    size_t bytes_to_read;
    void *handle = callbacks->open(context, file, &bytes_to_read);
    if (!handle) {
        return NULL;
    }
    size_t bytes_to_allocate = bytes_to_read + (null_terminate ? 1 : 0);
    void *data = malloc(bytes_to_allocate * sizeof(uint8_t)); // Add null terminator for the string.
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d bytes of memory", bytes_to_allocate);
        callbacks->close(handle);
        return NULL;
    }
    size_t read_bytes = callbacks->read(handle, data, bytes_to_read);
    callbacks->close(handle);
    if (read_bytes < bytes_to_read) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read %d bytes of data (%d available)", bytes_to_read, read_bytes);
        free(data);
        return NULL;
    }
    if (null_terminate) {
        ((char *)data)[read_bytes] = '\0';
    }
    *size = read_bytes;
    return data;
}

static File_System_Chunk_t load_as_string(const File_System_Callbacks_t *callbacks, const void *context, const char *file)
{
    size_t length;
    void *chars = _load(callbacks, context, file, true, &length);
    if (!chars) {
        return (File_System_Chunk_t){ .type = FILE_SYSTEM_CHUNK_NULL };
    }
    return (File_System_Chunk_t){
            .type = FILE_SYSTEM_CHUNK_STRING,
            .var = {
                .string = {
                        .chars = (char *)chars,
                        .length = chars ? length : 0
                    }
            }
        };
}

static File_System_Chunk_t load_as_binary(const File_System_Callbacks_t *callbacks, const void *context, const char *file)
{
    size_t size;
    void *ptr = _load(callbacks, context, file, false, &size);
    if (!ptr) {
        return (File_System_Chunk_t){ .type = FILE_SYSTEM_CHUNK_NULL };
    }
    return (File_System_Chunk_t){
            .type = FILE_SYSTEM_CHUNK_BLOB,
            .var = {
                .blob = {
                        .ptr = ptr,
                        .size = ptr ? size : 0
                    }
            }
        };
}

typedef struct _stbi_context_t {
    const File_System_Callbacks_t *callbacks;
    void *handle;
} stbi_context_t;

static int stb_stdio_read(void *user, char *data, int size)
{
    const stbi_context_t *stbi_context = (const stbi_context_t *)user;
    return (int)stbi_context->callbacks->read(stbi_context->handle, data, (size_t)size);
}

static void stb_stdio_skip(void *user, int n)
{
    const stbi_context_t *stbi_context = (const stbi_context_t *)user;
    stbi_context->callbacks->skip(stbi_context->handle, n);
}

static int stb_stdio_eof(void *user)
{
    const stbi_context_t *stbi_context = (const stbi_context_t *)user;
    return stbi_context->callbacks->eof(stbi_context->handle) ? -1 : 0;
}

static const stbi_io_callbacks _io_callbacks = {
    stb_stdio_read,
    stb_stdio_skip,
    stb_stdio_eof,
};

static File_System_Chunk_t load_as_image(const File_System_Callbacks_t *callbacks, const void *context, const char *file)
{
    size_t byte_to_read;
    void *handle = callbacks->open(context, file, &byte_to_read);
    if (!handle) {
        return (File_System_Chunk_t){ .type = FILE_SYSTEM_CHUNK_NULL };
    }

    stbi_context_t stbi_context = {
            .callbacks = callbacks, .handle = handle
        };

    int width, height, components;
    void *pixels = stbi_load_from_callbacks(&_io_callbacks, &stbi_context, &width, &height, &components, STBI_rgb_alpha);
    callbacks->close(handle);
    if (!pixels) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't decode surface from file `%s` (%s)", file, stbi_failure_reason());
        return (File_System_Chunk_t){ .type = FILE_SYSTEM_CHUNK_NULL };
    }

    return (File_System_Chunk_t){
            .type = FILE_SYSTEM_CHUNK_IMAGE,
            .var = {
                .image = {
                        .width = width,
                        .height = height,
                        .pixels = pixels
                    }
                }
        };
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
    if (resolved[strlen(resolved) - 1] != '/') {
        strcat(resolved, FILE_PATH_SEPARATOR_SZ);
    }

    DIR *dp = opendir(resolved);
    if (!dp) {
        fprintf(stderr,"cannot open directory: %s\n", resolved);
        return false;
    }

    for (struct dirent *entry = readdir(dp); entry; entry = readdir(dp)) {
        char full_path[FILE_PATH_MAX];
        strcpy(full_path, resolved);
        strcat(full_path, entry->d_name);

        struct stat statbuf;
        int result = stat(full_path, &statbuf);
        if (result != 0) {
            continue;
        }
        if (!S_ISREG(statbuf.st_mode)) {
            continue;
        }
        size_t length = strlen(entry->d_name);
        if (length < 4) {
            continue;
        }
        if (!strcmp(&entry->d_name[length - 4], ".pak") == 0) {
            continue;
        }

        _mount(file_system, full_path);

        // TODO: add also possibile "archive.pa0", ..., "archive.p99" file
        // overriding "archive.pak".
//        for (int i = 0; i < 100; ++i) {
//            sprintf(&entry->d_name[length - 2], "%02d", i);
//            _mount(file_system, full_path);
//        }
    }

    closedir(dp);

    _mount(file_system, resolved);

    return true;
}

void FS_terminate(File_System_t *file_system)
{
    size_t count = arrlen(file_system->mount_points);
    for (size_t i = 0; i < count; ++i) {
        File_System_Mount_t *mount_point = &file_system->mount_points[i];
        mount_point->callbacks->deinit(mount_point->context);
    }
    arrfree(file_system->mount_points);
}

File_System_Chunk_t FS_load(const File_System_t *file_system, const char *file, File_System_Chunk_Types_t type)
{
    File_System_Chunk_t chunk = (File_System_Chunk_t){ .type = FILE_SYSTEM_CHUNK_NULL };

    size_t count = arrlen(file_system->mount_points);
    for (int i = count - 1; i >= 0; --i) { // Backward search to enable resource override in multi-archives.
        File_System_Mount_t *mount_point = &file_system->mount_points[i];

        if (!mount_point->callbacks->exists(mount_point->context, file)) {
            continue;
        }

        if (type == FILE_SYSTEM_CHUNK_STRING) {
            chunk = load_as_string(mount_point->callbacks, mount_point->context, file);
        } else
        if (type == FILE_SYSTEM_CHUNK_BLOB) {
            chunk = load_as_binary(mount_point->callbacks, mount_point->context, file);
        } else
        if (type == FILE_SYSTEM_CHUNK_IMAGE) {
            chunk = load_as_image(mount_point->callbacks, mount_point->context, file);
        }

        break;
    }

    return chunk;
}

void FS_release(File_System_Chunk_t chunk)
{
    if (chunk.type == FILE_SYSTEM_CHUNK_STRING) {
        free(chunk.var.string.chars);
    } else
    if (chunk.type == FILE_SYSTEM_CHUNK_BLOB) {
        free(chunk.var.blob.ptr);
    } else
    if (chunk.type == FILE_SYSTEM_CHUNK_IMAGE) {
        stbi_image_free(chunk.var.image.pixels);
    }
}
