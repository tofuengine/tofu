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

#include <miniz/miniz.h>

#include <libs/log.h>
#include <libs/rc4.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define LOG_CONTEXT "fs-pak"

#define PAK_FLAG_COMPRESSED     0x00000001
#define PAK_FLAG_ENCRYPTED      0x00000002

#pragma pack(push, 1)
typedef struct _Pak_Header_t {
    char signature[8];
    uint32_t version;
    uint32_t flags;
    uint32_t entries;
} Pak_Header_t;

typedef struct _Pak_Entry_Header_t {
    uint32_t archive_size;
    uint32_t original_size;
    uint32_t name_length; // The entry header is followed by `name_length` chars and `archive_size` bytes.
} Pak_Entry_Header_t;
#pragma pack(pop)

typedef struct _Pak_Entry_t {
    char *name;
    long offset;
    size_t archive_size;
    size_t original_size;
} Pak_Entry_t;

typedef struct _Pak_Context_t {
    char base_path[FILE_PATH_MAX];
    size_t entries;
    Pak_Entry_t *directory;
    bool compressed;
    bool encrypted;
} Pak_Context_t;

typedef struct _Pak_Handle_t {
    uint8_t *data;
    uint8_t *end_of_data;
    uint8_t *pointer;
} Pak_Handle_t;

#define PAK_SIGNATURE   "TOFUPAK!"

static uint8_t KEY[] = {
    0xe6, 0xea, 0xa9, 0xf5, 0xd3, 0xe1, 0xae, 0xd1, 0xce, 0xd6, 0x7d, 0x40, 0x65, 0xaa, 0x9a, 0xc9    
};

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
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read file `%s` header", path);
        fclose(stream);
        return NULL;
    }
    if (strncmp(header.signature, PAK_SIGNATURE, sizeof(header.signature)) != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "file `%s` is not a valid archive", path);
        fclose(stream);
        return NULL;
    }

    Pak_Entry_t *directory = malloc(sizeof(Pak_Entry_t) * header.entries);
    if (!directory) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate #%d directory entries", header.entries);
        fclose(stream);
        return NULL;
    }
    memset(directory, 0x00, sizeof(Pak_Entry_t) * header.entries);

    size_t entries = 0;
    for (size_t i = 0; i < header.entries; ++i) {
        Pak_Entry_Header_t entry_header;
        size_t entries_read = fread(&entry_header, sizeof(Pak_Entry_Header_t), 1, stream);
        if (entries_read != 1) {
            Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read header for entry #%d", i);
            break;
        }

        char *entry_name = malloc((entry_header.name_length + 1) * sizeof(char));
        if (!entry_name) {
            Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate memory for entry #%d", i);
            break;
        }
        size_t chars_read = fread(entry_name, sizeof(char), entry_header.name_length, stream);
        if (chars_read != entry_header.name_length) {
            Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read name for entry #%d", i);
            break;
        }
        entry_name[entry_header.name_length] = '\0';

        directory[i] = (Pak_Entry_t){
                .name = entry_name,
                .offset = ftell(stream),
                .archive_size = entry_header.archive_size,
                .original_size = entry_header.original_size
            };

        fseek(stream, entry_header.archive_size, SEEK_CUR); // Skip the curren entry data and move the next entry header.

        entries += 1;
    }

    fclose(stream);

    if (entries < header.entries) {
        for (size_t i = 0; i < entries; ++i) {
            free(directory[i].name);
        }
        free(directory);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "directory w/ #%d entries deallocated", entries);
        return NULL;
    }

    qsort(directory, header.entries, sizeof(Pak_Entry_t), pak_entry_compare); // Keep sorted to use binary-search.
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "directory w/ #%d entries sorted", entries);

    Pak_Context_t *pak_context = malloc(sizeof(Pak_Context_t));
    if (!pak_context) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate context");
        for (size_t i = 0; i < entries; ++i) {
            free(directory[i].name);
        }
        free(directory);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "directory w/ #%d entries deallocated", entries);
        return NULL;
    }

    strcpy(pak_context->base_path, path);
    pak_context->entries = entries;
    pak_context->directory = directory;
    pak_context->compressed = header.flags & PAK_FLAG_COMPRESSED;
    pak_context->encrypted = header.flags & PAK_FLAG_ENCRYPTED;

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "I/O initialized for archive `%s` w/ %d entries (%scompressed, %sencrypted)",
        path, entries,
        pak_context->compressed ? "" : "un",
        pak_context->encrypted ? "" : "un");

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

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "I/O deinitialized");
}

// Upon opening the entry data is pre-loaded into memory (decrypting and inflating, if necessary),
// and later on is served through memory access.
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
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", pak_context->base_path);
        return NULL;
    }

    fseek(stream, entry->offset, SEEK_SET); // Move to the found entry position into the file.
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "entry `%s` found at position %d", file, entry->offset);

    uint8_t *source = malloc(entry->archive_size);
    if (!source) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate memory for entry `%s`", file);
        fclose(stream);
        return NULL;
    }

    size_t bytes_read = fread(source, sizeof(uint8_t), entry->archive_size, stream);
    if (bytes_read != entry->archive_size) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read %d bytes for entry `%s`", entry->archive_size, file);
        free(source);
        fclose(stream);
    }

    fclose(stream);

    uint8_t *data = malloc(entry->original_size);
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate memory for entry `%s`", file);
        free(source);
        return NULL;
    }

    if (pak_context->encrypted) {
        rc4_context_t rc4_context;
        rc4_schedule(&rc4_context, KEY, sizeof(KEY));
#ifdef DROP_256
        uint8_t drop[256] = { 0 };
        rc4_process(&pak_handle->rc4_context, drop, drop, sizeof(drop));
#endif
        rc4_process(&rc4_context, source, entry->archive_size);
        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d bytes decrypted", entry->archive_size);
    }

    if (pak_context->compressed) {
        mz_ulong data_length = entry->original_size;
        mz_uncompress(data, &data_length, source, entry->archive_size);
        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d bytes inflated to %d bytes", entry->archive_size, entry->original_size);
    } else {
        memcpy(data, source, entry->original_size);
        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d bytes stored", entry->original_size);
    }

    free(source);

    Pak_Handle_t *pak_handle = malloc(sizeof(Pak_Handle_t));
    if (!pak_handle) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate handle for entry `%s`", file);
        free(data);
        return NULL;
    }
    *pak_handle = (Pak_Handle_t){
            .data = data,
            .end_of_data = data + entry->original_size,
            .pointer = data
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry `%s` opened w/ handle %p (%d bytes)", file, pak_handle, entry->original_size);

    *size_in_bytes = entry->original_size;

    return pak_handle;
}

static size_t pakio_read(void *handle, void *buffer, size_t bytes_requested)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    size_t bytes_available = pak_handle->end_of_data - pak_handle->pointer;
    if (bytes_requested > bytes_available) {
        bytes_requested = bytes_available;
    }

    memcpy(buffer, pak_handle->pointer, bytes_requested);

    pak_handle->pointer += bytes_requested;

    return bytes_requested;
}

static void pakio_skip(void *handle, int offset)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    pak_handle->pointer += offset;
}

static bool pakio_eof(void *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    return pak_handle->pointer >= pak_handle->end_of_data;
}

static void pakio_close(void *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    free(pak_handle->data);
    free(pak_handle);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry w/ handle %p closed", pak_handle);
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
