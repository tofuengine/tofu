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

typedef struct _Std_Context_t {
    char base_path[FILE_PATH_MAX];
} Std_Context_t;

typedef struct _Std_Handle_t {
    FILE *stream;
} Std_Handle_t;

static void *stdio_init(const char *path)
{
    Std_Context_t *std_context = malloc(sizeof(Std_Context_t));
    *std_context = (Std_Context_t){ 0 };

    strcpy(std_context->base_path, path); // The path *need* to be terminated with the file path-separator!!!

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "I/O initialized at folder `%s`", path);

    return std_context;
}

static void stdio_deinit(void *context)
{
    Std_Context_t *std_context = (Std_Context_t *)context;

    free(std_context);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "I/O deinitialized");
}

static bool stdio_exists(const void *context, const char *file)
{
    const Std_Context_t *std_context = (const Std_Context_t *)context;

    char full_path[FILE_PATH_MAX];
    strcpy(full_path, std_context->base_path);
    strcat(full_path, file);

    bool exists = access(full_path, R_OK) != -1;
    Log_assert(!exists, LOG_LEVELS_DEBUG, LOG_CONTEXT, "file `%s` found in context %p", file, context);
    return exists;
}

static void *stdio_open(const void *context, const char *file, size_t *size_in_bytes)
{
    const Std_Context_t *std_context = (const Std_Context_t *)context;

    char full_path[FILE_PATH_MAX];
    strcpy(full_path, std_context->base_path);
    strcat(full_path, file);

    FILE *stream = fopen(full_path, "rb");
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", full_path);
        return NULL;
    }

    struct stat stat;
    int result = fstat(fileno(stream), &stat);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get file `%s` stats", full_path);
        fclose(stream);
        return NULL;
    }

    *size_in_bytes = stat.st_size;

    Std_Handle_t *std_handle = malloc(sizeof(Std_Handle_t));
    if (!std_handle) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate handle for file `%s`", file);
        fclose(stream);
        return NULL;
    }

    *std_handle = (Std_Handle_t){ .stream = stream };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "file `%s` opened w/ handle %p (%d bytes)", file, std_handle, stat.st_size);

    return std_handle;
}

static size_t stdio_read(void *handle, void *buffer, size_t bytes_requested)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    size_t bytes_read = fread(buffer, sizeof(char), bytes_requested, std_handle->stream);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes read for handle %p", bytes_read, handle);
    return bytes_read;
}

static void stdio_skip(void *handle, int offset)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    fseek(std_handle->stream, offset, SEEK_CUR);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes seeked for handle %p", offset, handle);
}

static bool stdio_eof(void *handle)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    bool end_of_file = feof(std_handle->stream) != 0;
    Log_assert(!end_of_file, LOG_LEVELS_DEBUG, LOG_CONTEXT, "end-of-file reached for handle %p", handle);
    return end_of_file;
}

static void stdio_close(void *handle)
{
    Std_Handle_t *std_handle = (Std_Handle_t *)handle;

    fclose(std_handle->stream);
    free(std_handle);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p closed", std_handle);
}

const File_System_Callbacks_t *stdio_callbacks = &(File_System_Callbacks_t){
    stdio_init,
    stdio_deinit,
    stdio_exists,
    stdio_open,
    stdio_read,
    stdio_skip,
    stdio_eof,
    stdio_close,
};
