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

#include "fsauxlib.h"

#include <libs/log.h>
#include <libs/stb.h>

#include <stdint.h>

#define LOG_CONTEXT "fs-aux"

// FIXME: convert bool argument to flags.
static void *_load(void *mount, const char *file, bool null_terminate, size_t *size)
{
    size_t bytes_to_read;
    File_System_Handle_t *handle = FS_open(mount, file, &bytes_to_read);
    if (!handle) {
        return NULL;
    }
    size_t bytes_to_allocate = bytes_to_read + (null_terminate ? 1 : 0);
    void *data = malloc(bytes_to_allocate * sizeof(uint8_t)); // Add null terminator for the string.
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d bytes of memory", bytes_to_allocate);
        FS_close(handle);
        return NULL;
    }
    size_t read_bytes = FS_read(handle, data, bytes_to_read);
    FS_close(handle);
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

static File_System_Chunk_t load_as_string(void *mount, const char *file)
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

static File_System_Chunk_t load_as_binary(File_System_Mount_t *mount, const char *file)
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

static int stbi_io_read(void *user, char *data, int size)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user;
    return (int)FS_read(handle, data, (size_t)size);
}

static void stbi_io_skip(void *user, int n)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user;
    FS_skip(handle, n);
}

static int stbi_io_eof(void *user)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user;
    return FS_eof(handle) ? -1 : 0;
}

static const stbi_io_callbacks _io_callbacks = {
    stbi_io_read,
    stbi_io_skip,
    stbi_io_eof,
};

static File_System_Chunk_t load_as_image(void *mount, const char *file)
{
    size_t byte_to_read;
    File_System_Handle_t *handle = FS_open(mount, file, &byte_to_read);
    if (!handle) {
        return (File_System_Chunk_t){ .type = FILE_SYSTEM_CHUNK_NULL };
    }

    int width, height, components;
    void *pixels = stbi_load_from_callbacks(&_io_callbacks, handle, &width, &height, &components, STBI_rgb_alpha);
    FS_close(handle);
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

File_System_Chunk_t FS_load(const File_System_t *file_system, const char *file, File_System_Chunk_Types_t type)
{
    File_System_Chunk_t chunk = (File_System_Chunk_t){ .type = FILE_SYSTEM_CHUNK_NULL };

    size_t count = arrlen(file_system->mounts);
    for (int i = count - 1; i >= 0; --i) { // Backward search to enable resource override in multi-archives.
        void *mount = file_system->mounts[i];

        if (!FS_exists(mount, file)) {
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
