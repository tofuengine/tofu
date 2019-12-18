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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define LOG_CONTEXT "fs_pak"

#pragma pack(push, 1)
typedef struct _Pak_Header_t {
    char signature[8];
    uint32_t version;
    uint32_t flags;
    uint32_t entries;
} Pak_Header_t;

typedef struct _Pak_Entry_Header_t {
    int32_t offset;
    uint32_t size;
    uint32_t name_length; // The entry header is followed by `name_length` chars.
} Pak_Entry_Header_t;
#pragma pack(pop)

typedef struct _Pak_Entry_t {
    char *name;
    long offset;
    size_t size;
} Pak_Entry_t;

typedef struct _Pak_Context_t {
    char base_path[FILE_PATH_MAX];
    size_t entries;
    Pak_Entry_t *directory;
} Pak_Context_t;

typedef struct _Pak_Handle_t {
    FILE *stream;
    long eof;
} Pak_Handle_t;

#define PAK_SIGNATURE   "TOFUPAK!"

static int pak_entry_compare(const void *lhs, const void *rhs)
{
    const Pak_Entry_t *l = (const Pak_Entry_t *)lhs;
    const Pak_Entry_t *r = (const Pak_Entry_t *)rhs;
    return strcmp(l->name, r->name);
}

static void *pakio_init(const char *path)
{
    FILE *stream = fopen(path, "rb");
    if (!stream) {
        return NULL;
    }

    Pak_Header_t header;
    int headers_read = fread(&header, sizeof(Pak_Header_t), 1, stream);
    if (headers_read != 1) {
        fclose(stream);
        return NULL;
    }
    if (strncmp(header.signature, PAK_SIGNATURE, sizeof(header.signature)) != 0) {
        fclose(stream);
        return NULL;
    }

    Pak_Entry_t *directory = malloc(sizeof(Pak_Entry_t) * header.entries);
    if (!directory) {
        fclose(stream);
        return NULL;
    }
    memset(directory, 0x00, sizeof(Pak_Entry_t) * header.entries);

    for (size_t i = 0; i < header.entries; ++i) {
        Pak_Entry_Header_t entry_header;
        size_t entries_read = fread(&entry_header, sizeof(Pak_Entry_Header_t), 1, stream);
        if (entries_read != 1) {
            break;
        }

        char *entry_name = malloc((entry_header.name_length + 1) * sizeof(char));
        if (!entry_name) {
            break;
        }
        size_t chars_read = fread(entry_name, sizeof(char), entry_header.name_length, stream);
        if (chars_read != entry_header.name_length) {
            break;
        }
        entry_name[entry_header.name_length] = '\0';

        directory[i] = (Pak_Entry_t){
                .name = entry_name,
                .offset = entry_header.offset,
                .size = entry_header.size
            };
    }

    fclose(stream);

    qsort(directory, header.entries, sizeof(Pak_Entry_t), pak_entry_compare); // Keep sorted to use binary-search.

    Pak_Context_t *pak_context = malloc(sizeof(Pak_Context_t));
    if (!pak_context) {
        free(directory);
        return NULL;
    }

    strcpy(pak_context->base_path, path);
    pak_context->entries = header.entries;
    pak_context->directory = directory;

    return pak_context;
}

static void pakio_deinit(void *context)
{
    Pak_Context_t *pak_context = (Pak_Context_t *)context;

    for (size_t i = 0; i < pak_context->entries; ++i) {
        free(pak_context->directory[i].name);
    }
    free(pak_context->directory);
    free(pak_context);
}

static void *pakio_open(const void *context, const char *file, char mode, size_t *size_in_bytes)
{
    Pak_Context_t *pak_context = (Pak_Context_t *)context;

    const Pak_Entry_t key = { .name = (char *)file };
    Pak_Entry_t *entry = bsearch((const void *)&key, pak_context->directory, pak_context->entries, sizeof(Pak_Entry_t), pak_entry_compare);
    if (!entry) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't find entry `%s`", file);
        return NULL;
    }

    FILE *stream = fopen(pak_context->base_path, mode == 'b' ? "rb" :"rt");
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access entry `%s`", file);
        return NULL;
    }

    fseek(stream, entry->offset, SEEK_SET);

    *size_in_bytes = entry->size;

    Pak_Handle_t *handle = malloc(sizeof(Pak_Handle_t));
    if (!handle) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate handle for file `%s`", file);
        fclose(stream);
        return NULL;
    }

    *handle = (Pak_Handle_t){ .stream = stream, .eof = entry->offset + entry->size };

    return handle;
}

static size_t pakio_read(void *handle, char *buffer, size_t bytes_to_read)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long position = ftell(pak_handle->stream);
    if (position == -1) {
        return 0;
    }

    size_t bytes_to_eof = pak_handle->eof - position;
    if (bytes_to_read > bytes_to_eof) {
        bytes_to_read = bytes_to_eof;
    }

    return fread(buffer, sizeof(uint8_t), bytes_to_read, pak_handle->stream);
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
