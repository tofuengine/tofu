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

#include <stdio.h>

#define LOG_CONTEXT "fs_pak"

#pragma pack(push, 4)
typedef struct _Pak_Header_t {
    char signature[4];
    size_t entries;
} Pak_Header_t;

typedef struct _Pak_Entry_t {
    char name[128];
    long offset;
    size_t size;
} Pak_Entry_t;
#pragma pack(pop)

typedef struct _Entry_t {
    long offset;
    size_t size;
} Entry_t;

typedef struct _Directory_Entry_t {
    char *key;
    Entry_t value;
} Directory_Entry_t;

typedef struct _Pak_Context_t {
    char base_path[FILE_PATH_MAX];
    Directory_Entry_t *directory;
} Pak_Context_t;

typedef struct _Pak_Handle_t {
    FILE *stream;
    long eof;
} Pak_Handle_t;

static void *pakio_init(const char *path)
{
    Pak_Context_t *pak_context = malloc(sizeof(Pak_Context_t));
    *pak_context = (Pak_Context_t){ 0 };

    strcpy(pak_context->base_path, path);

    FILE *stream = fopen(path, "rb");
    if (!stream) {
        return NULL;
    }

    Pak_Header_t header;
    int bytes_read = fread(&header, sizeof(Pak_Header_t), 1, stream);
    if (bytes_read != sizeof(Pak_Header_t)) {
        fclose(stream);
        return NULL;
    }
    if (strncmp(header.signature, "PAK!", 4) != 0) {
        fclose(stream);
        return NULL;
    }

    long offset = sizeof(Pak_Entry_t) * header.entries;
    fseek(stream, offset, SEEK_END);

    for (size_t i = 0; i < header.entries; ++i) {
        Pak_Entry_t entry;
        int bytes_read = fread(&entry, sizeof(Pak_Entry_t), 1, stream);
        if (bytes_read != sizeof(Pak_Entry_t)) {
            break;
        }

        const Entry_t pair = (Entry_t){ .offset = entry.offset, .size = entry.size };
        shput(pak_context->directory, entry.name, pair);
    }

    const Entry_t null = (Entry_t){ .offset = -1L, .size = 0 };
    shdefault(pak_context->directory, null);

    fclose(stream);

    return pak_context;
}

static void pakio_deinit(void *context)
{
    Pak_Context_t *pak_context = (Pak_Context_t *)context;

    shfree(pak_context->directory);

    free(pak_context);
}

static void *pakio_open(const void *context, const char *file, char mode, size_t *size_in_bytes)
{
    Pak_Context_t *pak_context = (Pak_Context_t *)context;

    const Entry_t entry = shget(pak_context->directory, file);
    if (entry.offset == -1L) {
        return NULL;
    }

    Pak_Handle_t *handle = malloc(sizeof(Pak_Handle_t));
    if (!handle) {
        return NULL;
    }

    FILE *stream = fopen(pak_context->base_path, mode == 'b' ? "rb" :"rt");
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access entry `%s`", file);
        free(handle);
        return NULL;
    }

    fseek(stream, entry.offset, SEEK_SET);

    *size_in_bytes = entry.size;

    *handle = (Pak_Handle_t){ .stream = stream, .eof = entry.offset + entry.size };

    return handle;
}

static size_t pakio_read(void *handle, char *buffer, size_t bytes_to_read)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long position = ftell(pak_handle->stream);
    if (position == -1) {
        return 0;
    }

    size_t bytes_to_eof = pak_handle->eof < position;
    if (bytes_to_eof > bytes_to_read) {
        bytes_to_eof = bytes_to_read;
    }

    return fread(buffer, sizeof(uint8_t), bytes_to_eof, pak_handle->stream);
}

static void pakio_skip(void *handle, int offset)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    fseek(pak_handle->stream, offset, SEEK_CUR);
}

static bool pakio_eof(void *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long position = ftell(pak_handle->stream);
    if (position == -1) {
        return true;
    }

    return position >= pak_handle->eof;
}

static void pakio_close(void *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    fclose(pak_handle->stream);
    free(pak_handle);
}

const File_System_Callbacks_t *pak_callbacks = &(File_System_Callbacks_t){
    pakio_init,
    pakio_deinit,
    pakio_open,
    pakio_read,
    pakio_skip,
    pakio_eof,
    pakio_close,
};
