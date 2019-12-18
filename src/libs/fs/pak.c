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

#include "pak.h"

#include <libs/log.h>
#include <libs/stb.h>

#include <lz4/lz4.h>

#define LOG_CONTEXT "fs_pak"

typedef struct _Pak_Context_t {
    char base_path[PATH_MAX];
} Pak_Context_t;

static void *pakio_init(const char *path)
{
    Pak_Context_t *pak_context = malloc(sizeof(Pak_Context_t));
    *pak_context = (Pak_Context_t){ 0 };

    return pak_context;
}

static void pakio_deinit(void *context)
{
    Pak_Context_t *pak_context = (Pak_Context_t *)context;

    free(pak_context->base_path);
    free(pak_context);
}

static void *pakio_open(const void *context, const char *file, File_System_Modes_t mode, size_t *size_in_bytes)
{
    return NULL;
}

static size_t pakio_read(void *handle, char *buffer, size_t bytes_to_read)
{
    return 0;
}

static void pakio_skip(void *handle, int offset)
{
}

static bool pakio_eof(void *handle)
{
    return false;
}

static void pakio_close(void *handle)
{
}

const File_System_Modes_IO_Callbacks_t *pak_callbacks = &(File_System_Modes_IO_Callbacks_t){
    pakio_init,
    pakio_deinit,
    pakio_open,
    pakio_read,
    pakio_skip,
    pakio_eof,
    pakio_close,
};
