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

#include "std.h"

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

#define LOG_CONTEXT "fs_std"

typedef struct _Std_Context_t {
    char base_path[FILE_PATH_MAX];
} Std_Context_t;

static void *stdio_init(const char *path)
{
    Std_Context_t *std_context = malloc(sizeof(Std_Context_t));
    *std_context = (Std_Context_t){ 0 };

    strcpy(std_context->base_path, path);

    return std_context;
}

static void stdio_deinit(void *context)
{
    Std_Context_t *std_context = (Std_Context_t *)context;

    free(std_context);
}

static void *stdio_open(const void *context, const char *file, File_System_Modes_t mode, size_t *size_in_bytes)
{
    Std_Context_t *std_context = (Std_Context_t *)context;

    char full_path[FILE_PATH_MAX];
    strcpy(full_path, std_context->base_path);
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

static size_t stdio_read(void *handle, char *buffer, size_t bytes_to_read)
{
    return fread(buffer, sizeof(char), bytes_to_read, (FILE *)handle);
}

static void stdio_skip(void *handle, int offset)
{
    fseek((FILE *)handle, offset, SEEK_CUR);
}

static bool stdio_eof(void *handle)
{
    return feof((FILE *)handle) != 0;
}

static void stdio_close(void *handle)
{
    fclose((FILE *)handle);
}

const File_System_Modes_IO_Callbacks_t *std_callbacks = &(File_System_Modes_IO_Callbacks_t){
    stdio_init,
    stdio_deinit,
    stdio_open,
    stdio_read,
    stdio_skip,
    stdio_eof,
    stdio_close,
};
