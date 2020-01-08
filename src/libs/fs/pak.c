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

#include "pak.h"

#include <libs/log.h>
#include <libs/md5.h>
#include <libs/rc4.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOG_CONTEXT "fs-pak"

#define PAK_SIGNATURE           "TOFUPAK!"
#define PAK_SIGNATURE_LENGTH    8

#define PAK_FLAG_ENCRYPTED      0x0001

#pragma pack(push, 1)
typedef struct _Pak_Header_t {
    char signature[PAK_SIGNATURE_LENGTH];
    uint8_t version;
    uint8_t flags;
    uint16_t __reserved;
    uint32_t entries;
} Pak_Header_t;

typedef struct _Pak_Entry_Header_t {
    uint16_t __reserved;
    uint16_t name; // The entry header is followed by `name` chars and `size` bytes.
    uint32_t size;
} Pak_Entry_Header_t;
#pragma pack(pop)

typedef struct _Pak_Entry_t {
    char *name;
    long offset;
    size_t size;
} Pak_Entry_t;

typedef struct _Pak_Context_t {
    char archive_path[FILE_PATH_MAX];
    size_t entries;
    Pak_Entry_t *directory;
    bool encrypted;
} Pak_Context_t;

typedef struct _Pak_Handle_t {
    const File_System_Handle_Callbacks_t *callbacks;
    FILE *stream;
    long end_of_stream;
    bool encrypted;
    rc4_context_t cipher_context;
} Pak_Handle_t;

static size_t pakio_read(void *handle, void *buffer, size_t bytes_requested)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long position = ftell(pak_handle->stream);
    if (position == -1) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get current position for handle %p", handle);
        return 0;
    }

    size_t bytes_available = pak_handle->end_of_stream - position;

    size_t bytes_to_read = bytes_requested;
    if (bytes_to_read > bytes_available) {
        bytes_to_read = bytes_available;
    }

    size_t bytes_read = fread(buffer, sizeof(uint8_t), bytes_to_read, pak_handle->stream);
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d bytes read out of %d (%d requested)", bytes_read, bytes_to_read, bytes_requested);

    if (pak_handle->encrypted) {
        rc4_process(&pak_handle->cipher_context, buffer, bytes_read);
        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d bytes decrypted", bytes_read);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes read for handle %p", bytes_read, handle);
    return bytes_read;
}

static void pakio_skip(void *handle, int offset)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    fseek(pak_handle->stream, offset, SEEK_CUR);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes seeked for handle %p", offset, handle);
}

static bool pakio_eof(void *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long position = ftell(pak_handle->stream);
    if (position == -1) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get current position for handle %p", handle);
        return true;
    }

    bool end_of_file = position >= pak_handle->end_of_stream;
    Log_assert(!end_of_file, LOG_LEVELS_DEBUG, LOG_CONTEXT, "end-of-file reached for handle %p", handle);
    return end_of_file;
}

static void pakio_close(void *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    fclose(pak_handle->stream);
    free(pak_handle);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry w/ handle %p closed", pak_handle);
}

const File_System_Handle_Callbacks_t *_pakio_handle_callbacks = &(File_System_Handle_Callbacks_t){
    pakio_read,
    pakio_skip,
    pakio_eof,
    pakio_close
};

static int _pak_entry_compare(const void *lhs, const void *rhs)
{
    const Pak_Entry_t *l = (const Pak_Entry_t *)lhs;
    const Pak_Entry_t *r = (const Pak_Entry_t *)rhs;
    return strcasecmp(l->name, r->name);
}

// Encryption is implemented throught a RC4 stream cipher.
// The key is the MD5 digest of the entry name (w/ relative path).
static void _initialize_cipher_context(rc4_context_t *cipher_context, const char *file)
{
    md5_context_t digest_context;
    md5_init(&digest_context);
    md5_update(&digest_context, (const uint8_t *)file, strlen(file));

    uint8_t cipher_key[MD5_SIZE];
    md5_final(&digest_context, cipher_key);

    rc4_schedule(cipher_context, cipher_key, sizeof(cipher_key));
#ifdef DROP_256
    uint8_t drop[256] = { 0 };
    rc4_process(cipher_context, drop, drop, sizeof(drop));
#endif
}

static void *pakio_init(const char *path)
{
    FILE *stream = fopen(path, "rb");
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", path);
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

        char *entry_name = malloc((entry_header.name + 1) * sizeof(char));
        if (!entry_name) {
            Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate memory for entry #%d", i);
            break;
        }
        size_t chars_read = fread(entry_name, sizeof(char), entry_header.name, stream);
        if (chars_read != entry_header.name) {
            Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read name for entry #%d", i);
            break;
        }
        entry_name[entry_header.name] = '\0';

        directory[i] = (Pak_Entry_t){
                .name = entry_name,
                .offset = ftell(stream),
                .size = entry_header.size,
            };

        fseek(stream, entry_header.size, SEEK_CUR); // Skip the curren entry data and move the next entry header.

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

    qsort(directory, header.entries, sizeof(Pak_Entry_t), _pak_entry_compare); // Keep sorted to use binary-search.
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

    strcpy(pak_context->archive_path, path);
    pak_context->entries = entries;
    pak_context->directory = directory;
    pak_context->encrypted = header.flags & PAK_FLAG_ENCRYPTED;

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "I/O initialized for archive `%s` w/ %d entries (%sencrypted)",
        path, entries,
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

static bool pakio_exists(const void *context, const char *file)
{
    const Pak_Context_t *pak_context = (const Pak_Context_t *)context;

    const Pak_Entry_t key = { .name = (char *)file };
    Pak_Entry_t *entry = bsearch((const void *)&key, pak_context->directory, pak_context->entries, sizeof(Pak_Entry_t), _pak_entry_compare);
    
    bool exists = entry;
    Log_assert(!exists, LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry `%s` found in context %p", file, context);
    return exists;
}

static void *pakio_open(const void *context, const char *file, size_t *size_in_bytes)
{
    const Pak_Context_t *pak_context = (const Pak_Context_t *)context;

    const Pak_Entry_t key = { .name = (char *)file };
    Pak_Entry_t *entry = bsearch((const void *)&key, pak_context->directory, pak_context->entries, sizeof(Pak_Entry_t), _pak_entry_compare);
    if (!entry) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't find entry `%s`", file);
        return NULL;
    }

    FILE *stream = fopen(pak_context->archive_path, "rb"); // Always in binary mode, line-terminators aren't an issue.
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", pak_context->archive_path);
        return NULL;
    }

    fseek(stream, entry->offset, SEEK_SET); // Move to the found entry position into the file.
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "entry `%s` found at offset %d in file `%s`", file, entry->offset, pak_context->archive_path);

    Pak_Handle_t *pak_handle = malloc(sizeof(Pak_Handle_t));
    if (!pak_handle) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate handle for entry `%s`", file);
        fclose(stream);
        return NULL;
    }
    *pak_handle = (Pak_Handle_t){
            .callbacks = _pakio_handle_callbacks,
            .stream = stream,
            .end_of_stream = entry->offset + entry->size,
            .encrypted = pak_context->encrypted
        };

    if (pak_context->encrypted) {
        _initialize_cipher_context(&pak_handle->cipher_context, entry->name);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry `%s` opened w/ handle %p (%d bytes)", file, pak_handle, entry->size);

    *size_in_bytes = entry->size;

    return pak_handle;
}

const File_System_Mount_Callbacks_t *pakio_callbacks = &(File_System_Mount_Callbacks_t){
    pakio_init,
    pakio_deinit,
    pakio_exists,
    pakio_open
};

bool pakio_is_archive(const char *path)
{
    FILE *stream = fopen(path, "rb");
    if (!stream) {
        return false;
    }

    char signature[PAK_SIGNATURE_LENGTH];
    const size_t chars_to_read = sizeof(signature) / sizeof(char);
    size_t chars_read = fread(signature, sizeof(char), chars_to_read, stream);

    fclose(stream);

    return chars_read == chars_to_read && strncmp(signature, PAK_SIGNATURE, PAK_SIGNATURE_LENGTH) == 0;
}
