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

#include <ctype.h>
#include <sys/stat.h>

#define LOG_CONTEXT "fs-pak"

#define PAK_SIGNATURE           "TOFUPAK!"
#define PAK_SIGNATURE_LENGTH    8

#define PAK_FLAG_ENCRYPTED      0x0001

/*
+------------------+
|    PAK HEADER    | sizeof(Pak_Header_t)
+------------------+
|      BLOB 0      |
+------------------+
|      BLOB 1      |
+------------------+
         ..
         ..
         ..
+------------------+
|      BLOB n      |
+------------------+
|  ENTRY HEADER 0  | sizeof(Pak_Entry_Header_t)
+------------------+
|   ENTRY NAME 0   | sizeof(char) * name_length
+------------------+
|  ENTRY HEADER 1  |
+------------------+
|   ENTRY NAME 1   |
+------------------+
         ..
         ..
         ..
+------------------+
|  ENTRY HEADER n  |
+------------------+
|   ENTRY NAME n   |
+------------------+
| DIRECTORY HEADER | sizeof(Pak_Directory_Header_t)
+------------------+
*/

#define PAK_NAME_LENGTH     MD5_SIZE

#pragma pack(push, 1)
typedef struct _Pak_Header_t {
    char signature[PAK_SIGNATURE_LENGTH];
    uint8_t version;
    uint8_t flags;
    uint16_t __reserved;
} Pak_Header_t;

typedef struct _Pak_Entry_Header_t {
    uint8_t id[PAK_NAME_LENGTH]; // The entry name is hashed.
    uint32_t offset;
    uint32_t size;
} Pak_Entry_Header_t;

typedef struct _Pak_Directory_Header_t {
    uint32_t offset;
    uint32_t entries; // Redundant, we could check file offsets, but it's quicker that way.
} Pak_Directory_Header_t;
#pragma pack(pop)

typedef struct _Pak_Entry_t {
    uint8_t id[PAK_NAME_LENGTH];
    long offset;
    size_t size;
} Pak_Entry_t;

typedef struct _Pak_Mount_t {
    Mount_VTable_t vtable; // Matches `_FS_Mount_t` structure.
    char archive_path[FILE_PATH_MAX];
    size_t entries;
    Pak_Entry_t *directory;
    uint8_t flags;
} Pak_Mount_t;

typedef struct _Pak_Handle_t {
    Handle_VTable_t vtable; // Matches `_FS_Handle_t` structure.
    FILE *stream;
    size_t stream_size;
    long beginning_of_stream;
    long end_of_stream;
    bool encrypted;
    rc4_context_t cipher_context;
} Pak_Handle_t;

static void _pak_mount_ctor(FS_Mount_t *mount, const char *archive_path, size_t entries, Pak_Entry_t *directory, uint8_t flags);
static void _pak_mount_dtor(FS_Mount_t *mount);
static bool _pak_mount_contains(const FS_Mount_t *mount, const char *file);
static FS_Handle_t *_pak_mount_open(const FS_Mount_t *mount, const char *file);

static void _pak_handle_ctor(FS_Handle_t *handle, FILE *stream, long offset, size_t size, bool encrypted, const uint8_t id[PAK_NAME_LENGTH]);
static void _pak_handle_dtor(FS_Handle_t *handle);
static size_t _pak_handle_size(FS_Handle_t *handle);
static size_t _pak_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested);
static bool _pak_handle_seek(FS_Handle_t *handle, long offset, int whence);
static long _pak_handle_tell(FS_Handle_t *handle);
static bool _pak_handle_eof(FS_Handle_t *handle);

bool FS_pak_is_valid(const char *path)
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
    return memcmp(l->id, r->id, PAK_NAME_LENGTH);
}

FS_Mount_t *FS_pak_mount(const char *path)
{
    FILE *stream = fopen(path, "rb");
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", path);
        return NULL;
    }

    Pak_Header_t header;
    size_t headers_read = fread(&header, sizeof(Pak_Header_t), 1, stream);
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

    int result = fseek(stream, -sizeof(Pak_Directory_Header_t), SEEK_END);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't seek file `%s` directory-header", path);
        fclose(stream);
        return NULL;
    }

    Pak_Directory_Header_t directory_header;
    headers_read = fread(&directory_header, sizeof(Pak_Directory_Header_t), 1, stream);
    if (headers_read != 1) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read file `%s` directory-header", path);
        fclose(stream);
        return NULL;
    }

    Pak_Entry_t *directory = malloc(sizeof(Pak_Entry_t) * directory_header.entries);
    if (!directory) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d directory entries", directory_header.entries);
        fclose(stream);
        return NULL;
    }

    result = fseek(stream, directory_header.offset, SEEK_SET);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't seek file `%s` directory-header", path);
        free(directory);
        fclose(stream);
        return NULL;
    }

    size_t entries = 0;
    for (size_t i = 0; i < directory_header.entries; ++i) {
        Pak_Entry_Header_t entry_header;
        size_t entries_read = fread(&entry_header, sizeof(Pak_Entry_Header_t), 1, stream);
        if (entries_read != 1) {
            Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read header for entry #%d", i);
            break;
        }

        directory[i] = (Pak_Entry_t){
                .id = { 0 },
                .offset = entry_header.offset,
                .size = entry_header.size,
            };
        memcpy(directory[i].id, entry_header.id, PAK_NAME_LENGTH);

        entries += 1;
    }

    fclose(stream);

    if (entries < directory_header.entries) {
        free(directory);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "directory w/ %d entries freed", entries);
        return NULL;
    }

    qsort(directory, directory_header.entries, sizeof(Pak_Entry_t), _pak_entry_compare); // Keep sorted to use binary-search.
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "directory w/ %d entries sorted", entries);

    FS_Mount_t *mount = malloc(sizeof(Pak_Mount_t));
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate mount for path `%s`", path);
        free(directory);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "directory w/ %d entries freed", entries);
        return NULL;
    }

    _pak_mount_ctor(mount, path, entries, directory, header.flags);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "mount initialized for archive `%s` w/ %d entries (flags 0x%02x)",
        path, entries, header.flags);

    return mount;
}

static void _pak_mount_ctor(FS_Mount_t *mount, const char *archive_path, size_t entries, Pak_Entry_t *directory, uint8_t flags)
{
    Pak_Mount_t *pak_mount = (Pak_Mount_t *)mount;

    *pak_mount = (Pak_Mount_t){
            .vtable = (Mount_VTable_t){
                .dtor = _pak_mount_dtor,
                .contains = _pak_mount_contains,
                .open = _pak_mount_open
            },
            .archive_path = { 0 },
            .entries = entries,
            .directory = directory,
            .flags = flags
        };

    strcpy(pak_mount->archive_path, archive_path);
}

static void _pak_mount_dtor(FS_Mount_t *mount)
{
    Pak_Mount_t *pak_mount = (Pak_Mount_t *)mount;

    free(pak_mount->directory);
}

static inline void _hash_file(const char *file, uint8_t id[PAK_NAME_LENGTH])
{
    md5_context_t context;
    md5_init(&context);
    for (size_t i = 0; i < strlen(file); ++i) {
        uint8_t c = tolower(file[i]); // Treat file names as lowercase/case-insensitive.
        md5_update(&context, &c, 1);
    }
    md5_final(&context, id);
}

static bool _pak_mount_contains(const FS_Mount_t *mount, const char *file)
{
    const Pak_Mount_t *pak_mount = (const Pak_Mount_t *)mount;

    Pak_Entry_t key = { 0 };
    _hash_file(file, key.id);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "file `%s` id is `%.*s`", file, PAK_NAME_LENGTH, key.id);

    const Pak_Entry_t *entry = bsearch((const void *)&key, pak_mount->directory, pak_mount->entries, sizeof(Pak_Entry_t), _pak_entry_compare);

    bool exists = entry; // FIXME: should be `!!`?
    Log_assert(!exists, LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry `%s` found in mount %p", file, pak_mount);
    return exists;
}

static FS_Handle_t *_pak_mount_open(const FS_Mount_t *mount, const char *file)
{
    const Pak_Mount_t *pak_mount = (const Pak_Mount_t *)mount;

    Pak_Entry_t key = { 0 };
    _hash_file(file, key.id);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "file `%s` id is `%.*s`", file, PAK_NAME_LENGTH, key.id);

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

    FS_Handle_t *handle = malloc(sizeof(Pak_Handle_t));
    if (!handle) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate handle for entry `%s`", file);
        fclose(stream);
        return NULL;
    }

    _pak_handle_ctor(handle, stream, entry->offset, entry->size, pak_mount->flags & PAK_FLAG_ENCRYPTED, entry->id);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry `%s` opened w/ handle %p (%d bytes)", file, handle, entry->size);

    return handle;
}

static void _pak_handle_ctor(FS_Handle_t *handle, FILE *stream, long offset, size_t size, bool encrypted, const uint8_t id[PAK_NAME_LENGTH])
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    *pak_handle = (Pak_Handle_t){
            .vtable = (Handle_VTable_t){
                .dtor = _pak_handle_dtor,
                .size = _pak_handle_size,
                .read = _pak_handle_read,
                .seek = _pak_handle_seek,
                .tell = _pak_handle_tell,
                .eof = _pak_handle_eof
            },
            .stream = stream,
            .stream_size = size,
            .beginning_of_stream = offset,
            .end_of_stream = offset + size - 1,
            .encrypted = encrypted,
            .cipher_context = { 0 }
        };

    if (encrypted) {
        // Encryption is implemented throught a RC4 stream cipher.
        // The key is the entry name (which is an MD5 digest for encrypted archives).
        rc4_schedule(&pak_handle->cipher_context, id, PAK_NAME_LENGTH);
#ifdef DROP_256
        uint8_t drop[256] = { 0 };
        rc4_process(cipher_context, drop, drop, sizeof(drop));
#endif
    }
}

static void _pak_handle_dtor(FS_Handle_t *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    fclose(pak_handle->stream);
}

static size_t _pak_handle_size(FS_Handle_t *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

//    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p is", std_handle);

    return pak_handle->stream_size;
}

static size_t _pak_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long position = ftell(pak_handle->stream);
    if (position == -1) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get current position for handle %p", handle);
        return 0;
    }

    size_t bytes_available = (size_t)(pak_handle->end_of_stream - position + 1);

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

static bool _pak_handle_seek(FS_Handle_t *handle, long offset, int whence)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long origin;
    if (whence == SEEK_SET) {
        origin = pak_handle->beginning_of_stream;
    } else
    if (whence == SEEK_CUR) {
        origin = ftell(pak_handle->stream);
    } else
    if (whence == SEEK_END) {
        origin = pak_handle->end_of_stream;
    } else {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "wrong seek mode %d for handle %p", whence, handle);
        return false;
    }

    long position = origin + offset;
    if (position < pak_handle->beginning_of_stream || position > pak_handle->end_of_stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "position %d is outside valid range for handle %p", position, handle);
        return false;
    }

    bool seeked = fseek(pak_handle->stream, position, SEEK_SET) == 0;
#ifdef __DEBUG_FS_CALLS__
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes seeked w/ mode %d for handle %p w/ result %d", offset, whence, handle, seeked);
#endif
    return seeked;
}

static long _pak_handle_tell(FS_Handle_t *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    return ftell(pak_handle->stream) - pak_handle->beginning_of_stream;
}

static bool _pak_handle_eof(FS_Handle_t *handle)
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
