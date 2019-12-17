/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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

#include <libs/log.h>
#include <libs/stb.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define LOG_CONTEXT "fs"

typedef enum _File_System_Modes_t {
    FILE_SYSTEM_MODE_BINARY,
    FILE_SYSTEM_MODE_TEXT,
    File_System_Modes_t_CountOf
} File_System_Modes_t;

static void *fs_open(const File_System_t *fs, const char *file, File_System_Modes_t mode, size_t *size_in_bytes)
{
    char full_path[PATH_FILE_MAX];
    strcpy(full_path, fs->base_path);
    strcat(full_path, file);

    FILE *stream = fopen(full_path, mode == FILE_SYSTEM_MODE_BINARY ? "rb" :"rt");
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", full_path);
    }

    struct stat stat;
    int result = fstat(fileno(stream), &stat);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get file `%s` stats", full_path);
        fclose(stream);
        return NULL;
    }

    *size_in_bytes = stat.st_size;

    return (void *)stream;
}

static size_t fs_read(const File_System_t *fs, void *handle, void *buffer, size_t bytes_to_read)
{
    return (size_t)fread(buffer, sizeof(char), bytes_to_read, (FILE *)handle);
}

static void fs_close(const File_System_t *fs, void *handle)
{
    fclose((FILE *)handle);
}

static void *fs_load(const File_System_t *fs, const char *file, File_System_Modes_t mode, size_t *size)
{
    size_t byte_to_read;
    void *handle = fs_open(fs, file, mode, &byte_to_read);
    if (!handle) {
        return NULL;
    }
    size_t bytes_to_allocate = byte_to_read + (mode == FILE_SYSTEM_MODE_TEXT ? 1 : 0);
    void *data = malloc(bytes_to_allocate * sizeof(uint8_t)); // Add null terminator for the string.
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d bytes of memory", bytes_to_allocate);
        fs_close(fs, handle);
        return NULL;
    }
    size_t read_bytes = fs_read(fs, handle, data, byte_to_read);
    fs_close(fs, handle);
    if (read_bytes < byte_to_read) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read %d bytes of data (%d available)", byte_to_read, read_bytes);
        free(data);
        return NULL;
    }
    if (mode == FILE_SYSTEM_MODE_TEXT) {
        ((char *)data)[read_bytes] = '\0';
    }
    *size = read_bytes;
    return data;
}

static File_System_Modes_IO_Callbacks_t _callbacks = { 0 };

static File_System_Chunk_t load_as_string(const File_System_t *fs, const char *file)
{
    size_t length;
    char *chars = fs_load(fs, file, FILE_SYSTEM_MODE_TEXT, &length);
    return (File_System_Chunk_t){
            .type = FILE_SYSTEM_CHUNK_STRING,
            .var = {
                .string = {
                        .chars = chars,
                        .length = length
                    }
            }
        };
}

static File_System_Chunk_t load_as_binary(const File_System_t *fs, const char *file)
{
    size_t size;
    void *ptr = fs_load(fs, file, FILE_SYSTEM_MODE_BINARY, &size);
    return (File_System_Chunk_t){
            .type = FILE_SYSTEM_CHUNK_BLOB,
            .var = {
                .blob = {
                        .ptr = ptr,
                        .size = size
                    }
            }
        };
}

static int stb_stdin_read(void *user, char *data, int size)
{
    return (int) fread(data,1,size,(FILE*) user);
}

static void stb_stdin_skip(void *user, int n)
{
    fseek((FILE*) user, n, SEEK_CUR);
}

static int stb_stdin_eof(void *user)
{
    return feof((FILE*) user);
}

static const stbi_io_callbacks _io_callbacks = {
    stb_stdin_read,
    stb_stdin_skip,
    stb_stdin_eof,
};

static File_System_Chunk_t load_as_image(const File_System_t *fs, const char *file)
{
    size_t byte_to_read;
    void *handle = fs_open(fs, file, FILE_SYSTEM_MODE_BINARY, &byte_to_read);
    if (!handle) {
        return (File_System_Chunk_t){ .type = FILE_SYSTEM_CHUNK_NULL };
    }

    int width, height, components;
    void *pixels = stbi_load_from_callbacks(&_io_callbacks, handle, &width, &height, &components, STBI_rgb_alpha);
    fs_close(fs, handle);
    if (!pixels) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't decode surface from file: %s", stbi_failure_reason());
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

// TODO: implement a generic resource-loader, for later use with bundled applications.
void FS_initialize(File_System_t *fs, const char *base_path)
{
    *fs = (File_System_t){ 0 };

    char resolved[PATH_FILE_MAX]; // Using local buffer to avoid un-tracked `malloc()` for the syscall.
    char *ptr = realpath(base_path ? base_path : FILE_PATH_CURRENT_SZ, resolved);
    if (!ptr) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't resolve path `%s`", base_path);
        return;
    }

    size_t length = strlen(resolved);
    if (resolved[length - 1] != '/') {
        strcat(resolved, FILE_PATH_SEPARATOR_SZ);
        length += 1;
    }

    fs->base_path = malloc((length + 1) * sizeof(char));
    strcpy(fs->base_path, resolved);
}

void FS_terminate(File_System_t *fs)
{
    free(fs->base_path);
}

File_System_Chunk_t FS_load(const File_System_t *fs, const char *file, File_System_Chunk_Types_t type)
{
    if (type == FILE_SYSTEM_CHUNK_STRING) {
        return load_as_string(fs, file);
    } else
    if (type == FILE_SYSTEM_CHUNK_BLOB) {
        return load_as_binary(fs, file);
    } else
    if (type == FILE_SYSTEM_CHUNK_IMAGE) {
        return load_as_image(fs, file);
    }
    return (File_System_Chunk_t){ .type = FILE_SYSTEM_CHUNK_NULL };
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
