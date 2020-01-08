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

#define READER_BUFFER_SIZE  2048

typedef struct _Reader_Context_t {
    File_System_Handle_t *handle;
    char buffer[READER_BUFFER_SIZE];
} Reader_Context_t;

const File_System_Mount_Callbacks_t *_detect(const char *path)
{
    struct stat path_stat;
    int result = stat(path, &path_stat);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get stats for `%s`", path);
        return false;
    }

    if (S_ISDIR(path_stat.st_mode)) {
        return stdio_callbacks;
    } else
    if (S_ISREG(path_stat.st_mode) && pakio_is_archive(path)) {
        return pakio_callbacks;
    }

    return NULL;
}

static bool _mount(File_System_t *file_system, const char *base_path)
{
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "adding mount-point `%s`", base_path);

    const File_System_Mount_Callbacks_t *callbacks = _detect(base_path);
    if (!callbacks) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't detect type for mount-point `%s`", base_path);
        return false;
    }

    void *context = callbacks->init(base_path);
    if (!context) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize mount-point `%s`", base_path);
        return false;
    }

    File_System_Mount_t mount = (File_System_Mount_t){
            .callbacks = callbacks,
            .context = context
        };
    arrpush(file_system->mounts, mount);

    return true;
}

// FIXME: convert bool argument to flags.
static void *_load(const File_System_Mount_t *mount, const char *file, bool null_terminate, size_t *size)
{
    size_t bytes_to_read;
    File_System_Handle_t *handle = mount->callbacks->open(mount->context, file, &bytes_to_read);
    if (!handle) {
        return NULL;
    }
    size_t bytes_to_allocate = bytes_to_read + (null_terminate ? 1 : 0);
    void *data = malloc(bytes_to_allocate * sizeof(uint8_t)); // Add null terminator for the string.
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d bytes of memory", bytes_to_allocate);
        handle->callbacks->close(handle);
        return NULL;
    }
    size_t read_bytes = handle->callbacks->read(handle, data, bytes_to_read);
    handle->callbacks->close(handle);
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

static File_System_Chunk_t load_as_string(const File_System_Mount_t *mount, const char *file)
{
    size_t length;
    void *chars = _load(mount, file, true, &length);
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

static File_System_Chunk_t load_as_binary(const File_System_Mount_t *mount, const char *file)
{
    size_t size;
    void *ptr = _load(mount, file, false, &size);
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

static int stb_stdio_read(void *user, char *data, int size)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user;
    return (int)handle->callbacks->read(handle, data, (size_t)size);
}

static void stb_stdio_skip(void *user, int n)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user;
    handle->callbacks->skip(handle, n);
}

static int stb_stdio_eof(void *user)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user;
    return handle->callbacks->eof(handle) ? -1 : 0;
}

static const stbi_io_callbacks _io_callbacks = {
    stb_stdio_read,
    stb_stdio_skip,
    stb_stdio_eof,
};

static File_System_Chunk_t load_as_image(const File_System_Mount_t *mount, const char *file)
{
    size_t byte_to_read;
    File_System_Handle_t *handle = mount->callbacks->open(mount->context, file, &byte_to_read);
    if (!handle) {
        return (File_System_Chunk_t){ .type = FILE_SYSTEM_CHUNK_NULL };
    }

    int width, height, components;
    void *pixels = stbi_load_from_callbacks(&_io_callbacks, handle, &width, &height, &components, STBI_rgb_alpha);
    handle->callbacks->close(handle);
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

static const char *_reader(lua_State *L, void *ud, size_t *size)
{
    Reader_Context_t *context = (Reader_Context_t *)ud;
    File_System_Handle_t *handle = context->handle;

    if (handle->callbacks->eof(handle)) {
        return NULL;
    }

    *size = handle->callbacks->read(handle, context->buffer, READER_BUFFER_SIZE);

    return context->buffer;
}

static int load_as_script(const File_System_Mount_t *mount, const char *file, lua_State *L)
{
    size_t bytes_to_read;
    File_System_Handle_t *handle = mount->callbacks->open(mount->context, file, &bytes_to_read);
    if (!handle) {
        return LUA_ERRFILE;
    }

    char name[FILE_PATH_MAX];
    sprintf(name, "@%s", file);

    // nor `text` nor `binary`, autodetect.
    int result = lua_load(L, _reader, &(Reader_Context_t){ .handle = handle }, name, NULL);

    handle->callbacks->close(handle);

    return result;
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
        if (!pakio_is_archive(full_path)) {
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
    const size_t count = arrlen(file_system->mounts);
    for (size_t i = 0; i < count; ++i) {
        File_System_Mount_t *mount = &file_system->mounts[i];
        mount->callbacks->deinit(mount->context);
    }
    arrfree(file_system->mounts);
}

bool FS_exists(const File_System_t *file_system, const char *file)
{
    const size_t count = arrlen(file_system->mounts);
    for (size_t i = 0; i < count; ++i) {
        File_System_Mount_t *mount = &file_system->mounts[i];

        if (mount->callbacks->exists(mount->context, file)) {
            return true;
        }
    }

    return false;
}

File_System_Handle_t *FS_open(const File_System_t *file_system, const char *file, size_t *size_in_bytes)
{
    const size_t count = arrlen(file_system->mounts);
    for (int i = count - 1; i >= 0; --i) { // Backward search to enable resource override in multi-archives.
        File_System_Mount_t *mount = &file_system->mounts[i];

        return mount->callbacks->open(mount->context, file, size_in_bytes);
    }

    return NULL;
}

size_t FS_read(File_System_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    return handle->callbacks->read(handle, buffer, bytes_requested);
}

void FS_skip(File_System_Handle_t *handle, int offset)
{
    handle->callbacks->skip(handle, offset);
}

bool FS_eof(File_System_Handle_t *handle)
{
    return handle->callbacks->eof(handle);
}

void FS_close(File_System_Handle_t *handle)
{
    handle->callbacks->close(handle);
}

int FS_load_script(const File_System_t *file_system, const char *file, lua_State *L)
{
    size_t count = arrlen(file_system->mounts);
    for (int i = count - 1; i >= 0; --i) { // Backward search to enable resource override in multi-archives.
        const File_System_Mount_t *mount = &file_system->mounts[i];

        if (!mount->callbacks->exists(mount->context, file)) {
            continue;
        }

        return load_as_script(mount, file, L);
    }

    return LUA_ERRFILE;
}

File_System_Chunk_t FS_load(const File_System_t *file_system, const char *file, File_System_Chunk_Types_t type)
{
    File_System_Chunk_t chunk = (File_System_Chunk_t){ .type = FILE_SYSTEM_CHUNK_NULL };

    size_t count = arrlen(file_system->mounts);
    for (int i = count - 1; i >= 0; --i) { // Backward search to enable resource override in multi-archives.
        const File_System_Mount_t *mount = &file_system->mounts[i];

        if (!mount->callbacks->exists(mount->context, file)) {
            continue;
        }

        if (type == FILE_SYSTEM_CHUNK_STRING) {
            chunk = load_as_string(mount, file);
        } else
        if (type == FILE_SYSTEM_CHUNK_BLOB) {
            chunk = load_as_binary(mount, file);
        } else
        if (type == FILE_SYSTEM_CHUNK_IMAGE) {
            chunk = load_as_image(mount, file);
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
