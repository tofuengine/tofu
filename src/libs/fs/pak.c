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

#include "internals.h"

#include <libs/log.h>
#include <libs/md5.h>
#include <libs/rc4.h>
#include <libs/stb.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

typedef struct _Pak_Mount_t {
    Mount_VTable_t vtable;
    char archive_path[FILE_PATH_MAX];
    size_t entries;
    Pak_Entry_t *directory;
    uint8_t flags;
} Pak_Mount_t;

typedef struct _Pak_Handle_t {
    Handle_VTable_t vtable;
    FILE *stream;
    size_t stream_size;
    long beginning_of_stream;
    long end_of_stream;
    bool encrypted;
    rc4_context_t cipher_context;
} Pak_Handle_t;

static void _pak_mount_ctor(File_System_Mount_t *mount, const char *archive_path, size_t entries, Pak_Entry_t *directory, uint8_t flags);
static void _pak_mount_dtor(File_System_Mount_t *mount);
static bool _pak_mount_contains(File_System_Mount_t *mount, const char *file);
static File_System_Handle_t *_pak_mount_open(File_System_Mount_t *mount, const char *file);

static void _pak_handle_ctor(File_System_Handle_t *handle, FILE *stream, long offset, size_t size, bool encrypted, const char *name);
static void _pak_handle_dtor(File_System_Handle_t *handle);
static size_t _pak_handle_size(File_System_Handle_t *handle);
static size_t _pak_handle_read(File_System_Handle_t *handle, void *buffer, size_t bytes_requested);
static bool _pak_handle_seek(File_System_Handle_t *handle, long offset, int whence);
static long _pak_handle_tell(File_System_Handle_t *handle);
static bool _pak_handle_eof(File_System_Handle_t *handle);

bool pak_is_valid(const char *path)
{
    struct stat path_stat;
    int result = stat(path, &path_stat);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get stats for file `%s`", path);
        return false;
    }

    if (!S_ISREG(path_stat.st_mode)) {
        return false;
    }

    FILE *stream = fopen(path, "rb");
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", path);
        return false;
    }

    char signature[PAK_SIGNATURE_LENGTH];
    const size_t chars_to_read = sizeof(signature) / sizeof(char);
    size_t chars_read = fread(signature, sizeof(char), chars_to_read, stream);

    fclose(stream);

    return chars_read == chars_to_read && strncmp(signature, PAK_SIGNATURE, PAK_SIGNATURE_LENGTH) == 0;
}

static int _pak_entry_compare(const void *lhs, const void *rhs)
{
    const Pak_Entry_t *l = (const Pak_Entry_t *)lhs;
    const Pak_Entry_t *r = (const Pak_Entry_t *)rhs;
    return strcasecmp(l->name, r->name);
}

File_System_Mount_t *pak_mount(const char *path)
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
    if (strncmp(header.signature, PAK_SIGNATURE, PAK_SIGNATURE_LENGTH) != 0) {
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
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "directory w/ #%d entries freed", entries);
        return NULL;
    }

    qsort(directory, header.entries, sizeof(Pak_Entry_t), _pak_entry_compare); // Keep sorted to use binary-search.
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "directory w/ #%d entries sorted", entries);

    File_System_Mount_t *mount = malloc(sizeof(Pak_Mount_t));
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate mount for path `%s`", path);
        for (size_t i = 0; i < entries; ++i) {
            free(directory[i].name);
        }
        free(directory);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "directory w/ #%d entries freed", entries);
        return NULL;
    }

    _pak_mount_ctor(mount, path, entries, directory, header.flags);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "mount initialized for archive `%s` w/ %d entries (flags 0x%02x)",
        path, entries, header.flags);

    return mount;
}

static void _pak_mount_ctor(File_System_Mount_t *mount, const char *archive_path, size_t entries, Pak_Entry_t *directory, uint8_t flags)
{
    Pak_Mount_t *pak_mount = (Pak_Mount_t *)mount;

    *pak_mount = (Pak_Mount_t){ 0 };
    pak_mount->vtable = (Mount_VTable_t){
            .dtor = _pak_mount_dtor,
            .contains = _pak_mount_contains,
            .open = _pak_mount_open
        };

    strcpy(pak_mount->archive_path, archive_path);
    pak_mount->entries = entries;
    pak_mount->directory = directory;
    pak_mount->flags = flags;
}

static void _pak_mount_dtor(File_System_Mount_t *mount)
{
    Pak_Mount_t *pak_mount = (Pak_Mount_t *)mount;

    for (size_t i = 0; i < pak_mount->entries; ++i) {
        free(pak_mount->directory[i].name);
    }
    free(pak_mount->directory);
}

static bool _pak_mount_contains(File_System_Mount_t *mount, const char *file)
{
    Pak_Mount_t *pak_mount = (Pak_Mount_t *)mount;

    const Pak_Entry_t key = { .name = (char *)file };
    const Pak_Entry_t *entry = bsearch((const void *)&key, pak_mount->directory, pak_mount->entries, sizeof(Pak_Entry_t), _pak_entry_compare);

    bool exists = entry;
    Log_assert(!exists, LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry `%s` found in mount %p", file, pak_mount);
    return exists;
}

static File_System_Handle_t *_pak_mount_open(File_System_Mount_t *mount, const char *file)
{
    Pak_Mount_t *pak_mount = (Pak_Mount_t *)mount;

    const Pak_Entry_t key = { .name = (char *)file };
    const Pak_Entry_t *entry = bsearch((const void *)&key, pak_mount->directory, pak_mount->entries, sizeof(Pak_Entry_t), _pak_entry_compare);
    if (!entry) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't find entry `%s`", file);
        return NULL;
    }

    FILE *stream = fopen(pak_mount->archive_path, "rb"); // Always in binary mode, line-terminators aren't an issue.
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", pak_mount->archive_path);
        return NULL;
    }

    fseek(stream, entry->offset, SEEK_SET); // Move to the found entry position into the file.
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "entry `%s` found at offset %d in file `%s`", file, entry->offset, pak_mount->archive_path);

    File_System_Handle_t *handle = malloc(sizeof(Pak_Handle_t));
    if (!handle) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate handle for entry `%s`", file);
        fclose(stream);
        return NULL;
    }

    _pak_handle_ctor(handle, stream, entry->offset, entry->size, pak_mount->flags & PAK_FLAG_ENCRYPTED, entry->name);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry `%s` opened w/ handle %p (%d bytes)", file, handle, entry->size);

    return handle;
}

static void _pak_handle_ctor(File_System_Handle_t *handle, FILE *stream, long offset, size_t size, bool encrypted, const char *name)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    *pak_handle = (Pak_Handle_t){ 0 };
    pak_handle->vtable = (Handle_VTable_t){
            .dtor = _pak_handle_dtor,
            .size = _pak_handle_size,
            .read = _pak_handle_read,
            .seek = _pak_handle_seek,
            .tell = _pak_handle_tell,
            .eof = _pak_handle_eof
        };

    pak_handle->stream = stream;
    pak_handle->stream_size = size;
    pak_handle->beginning_of_stream = offset;
    pak_handle->end_of_stream = offset + size - 1;
    pak_handle->encrypted = encrypted;
    if (encrypted) {
        // Encryption is implemented throught a RC4 stream cipher.
        // The key is the MD5 digest of the entry name (w/ relative path).
        md5_context_t digest_context;
        md5_init(&digest_context);
        md5_update(&digest_context, (const uint8_t *)name, strlen(name));

        uint8_t cipher_key[MD5_SIZE];
        md5_final(&digest_context, cipher_key);

        rc4_schedule(&pak_handle->cipher_context, cipher_key, sizeof(cipher_key));
#ifdef DROP_256
        uint8_t drop[256] = { 0 };
        rc4_process(cipher_context, drop, drop, sizeof(drop));
#endif
    }
}

static void _pak_handle_dtor(File_System_Handle_t *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    fclose(pak_handle->stream);
}

static size_t _pak_handle_size(File_System_Handle_t *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

//    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p is", std_handle);

    return pak_handle->stream_size;
}

static size_t _pak_handle_read(File_System_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long position = ftell(pak_handle->stream);
    if (position == -1) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get current position for handle %p", handle);
        return 0;
    }

    size_t bytes_available = pak_handle->end_of_stream - position + 1;

    size_t bytes_to_read = bytes_requested;
    if (bytes_to_read > bytes_available) {
        bytes_to_read = bytes_available;
    }

    size_t bytes_read = fread(buffer, sizeof(uint8_t), bytes_to_read, pak_handle->stream);
#ifdef __DEBUG_FS_CALLS__
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d bytes read out of %d (%d requested)", bytes_read, bytes_to_read, bytes_requested);
#endif

    if (pak_handle->encrypted) {
        rc4_process(&pak_handle->cipher_context, buffer, bytes_read);
#ifdef __DEBUG_FS_CALLS__
        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d bytes decrypted", bytes_read);
#endif
    }

#ifdef __DEBUG_FS_CALLS__
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes read for handle %p", bytes_read, handle);
#endif
    return bytes_read;
}

static bool _pak_handle_seek(File_System_Handle_t *handle, long offset, int whence)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long offset_from_beginning = 0;
    if (whence == SEEK_SET) {
        offset_from_beginning = pak_handle->beginning_of_stream;
    } else
    if (whence == SEEK_CUR) {
        offset_from_beginning = ftell(pak_handle->stream);
    } else
    if (whence == SEEK_END) {
        offset_from_beginning = pak_handle->end_of_stream;
    }

    bool seeked = fseek(pak_handle->stream, offset_from_beginning + offset, SEEK_SET) == 0;
#ifdef __DEBUG_FS_CALLS__
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes seeked w/ mode %d for handle %p w/ result %d", offset, whence, handle, seeked);
#endif
    return seeked;
}

static long _pak_handle_tell(File_System_Handle_t *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    return ftell(pak_handle->stream) - pak_handle->beginning_of_stream;
}

static bool _pak_handle_eof(File_System_Handle_t *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long position = ftell(pak_handle->stream);
    if (position == -1) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get current position for handle %p", handle);
        return true;
    }

    bool end_of_file = position > pak_handle->end_of_stream;
#ifdef __DEBUG_FS_CALLS__
    Log_assert(!end_of_file, LOG_LEVELS_DEBUG, LOG_CONTEXT, "end-of-file reached for handle %p", handle);
#endif
    return end_of_file;
}
