/*
 * MIT License
 *
 * Copyright (c) 2019-2021 Marco Lizza
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
#include <libs/path.h>
#include <libs/stb.h>
#include <libs/xor.h>

#include <ctype.h>

#define LOG_CONTEXT "fs-pak"

#define PAK_SIGNATURE           "TOFUPAK!"
#define PAK_SIGNATURE_LENGTH    8

#define PAK_VERSION             0

#define PAK_FLAG_ENCRYPTED      0x0001

// TODO: no in-memory directory?

/*
+---------+
| HEADER  | sizeof(Pak_Header_t)
+---------+
| BLOB 0  |
+---------+
| BLOB 1  |
+---------+
    ...
    ...
    ...
+---------+
| BLOB n  |
+---------+
| ENTRY 0 | sizeof(uint16_t) + N * sizeof(char) + sizeof(uint32_t) + sizeof(uint32_t)
+---------+
| ENTRY 1 |
+---------+
    ...
    ...
    ...
+---------+
| ENTRY n |
+---------+
|  INDEX  | sizeof(Pak_Index_t)
+---------+
*/

#define PAK_ID_LENGTH       MD5_SIZE
#define PAK_ID_LENGTH_SZ    (MD5_SIZE * 2 + 1)

#pragma pack(push, 1)
typedef struct Pak_Header_s {
    char signature[PAK_SIGNATURE_LENGTH];
    uint8_t version;
    uint8_t flags;
    uint16_t __reserved;
} Pak_Header_t;

typedef struct Pak_Entry_Header_s {
    uint32_t offset;
    uint32_t size;
    uint16_t chars;
} Pak_Entry_Header_t;

typedef struct Pak_Entry_s {
    char *name;
    uint8_t id[PAK_ID_LENGTH];
    uint32_t offset;
    uint32_t size;
} Pak_Entry_t;

typedef struct Pak_Index_s {
    uint32_t offset;
    uint32_t entries; // Redundant, we could check file offsets, but it's quicker that way.
} Pak_Index_t;
#pragma pack(pop)

typedef struct Pak_Mount_s {
    Mount_VTable_t vtable; // Matches `_FS_Mount_t` structure.
    char path[PLATFORM_PATH_MAX];
    size_t entries;
    Pak_Entry_t *directory;
    uint8_t flags;
} Pak_Mount_t;

typedef struct Pak_Handle_s {
    Handle_VTable_t vtable; // Matches `_FS_Handle_t` structure.
    FILE *stream;
    size_t stream_size;
    long beginning_of_stream;
    long end_of_stream;
    bool encrypted;
    xor_context_t cipher_context;
} Pak_Handle_t;

static void _pak_mount_ctor(FS_Mount_t *mount, const char *path, size_t entries, Pak_Entry_t *directory, uint8_t flags);
static void _pak_mount_dtor(FS_Mount_t *mount);
static bool _pak_mount_contains(const FS_Mount_t *mount, const char *name);
static FS_Handle_t *_pak_mount_open(const FS_Mount_t *mount, const char *name);

static void _pak_handle_ctor(FS_Handle_t *handle, FILE *stream, long offset, size_t size, bool encrypted, const uint8_t id[PAK_ID_LENGTH]);
static void _pak_handle_dtor(FS_Handle_t *handle);
static size_t _pak_handle_size(FS_Handle_t *handle);
static size_t _pak_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested);
static bool _pak_handle_seek(FS_Handle_t *handle, long offset, int whence);
static long _pak_handle_tell(FS_Handle_t *handle);
static bool _pak_handle_eof(FS_Handle_t *handle);

static bool _pak_validate_archive(FILE *stream, const char *path, uint8_t *flags)
{
    Pak_Header_t header;
    size_t entries_read = fread(&header, sizeof(Pak_Header_t), 1, stream);
    if (entries_read != 1) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read header from file `%s`", path);
        return false;
    }
    if (strncmp(header.signature, PAK_SIGNATURE, PAK_SIGNATURE_LENGTH) != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "file `%s` is not a valid archive", path);
        return false;
    }
    if (header.version != PAK_VERSION) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "archive `%s` version mismatch (found %d, required %d)", path, header.version, PAK_VERSION);
        return false;
    }
    if (flags) {
        *flags = header.flags;
    }
    return true;
}

bool FS_pak_is_valid(const char *path)
{
    if (!path_is_file(path)) {
        return false;
    }

    FILE *stream = fopen(path, "rb");
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", path);
        return false;
    }

    bool validated = _pak_validate_archive(stream, path, NULL);
    if (!validated) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't validate file `%s` as archive", path);
        fclose(stream);
        return false;
    }

    fclose(stream);

    return true;
}

static inline void _hash_file(const char *name, uint8_t id[PAK_ID_LENGTH], char sz[PAK_ID_LENGTH_SZ])
{
    md5_context_t context;
    md5_init(&context);
    for (size_t i = 0; i < strlen(name); ++i) {
        uint8_t c = tolower(name[i]); // Treat file names as lowercase/case-insensitive.
        md5_update(&context, &c, 1);
    }
    md5_final(&context, id);
    for (size_t i = 0; i < PAK_ID_LENGTH; ++i) { // Also convert to string representation.
        sprintf(sz + i * 2, "%02x", id[i]);
    }
}

static bool _read_entry(Pak_Entry_t *entry, FILE *stream)
{
    Pak_Entry_Header_t header;
    size_t entries_read = fread(&header, sizeof(Pak_Entry_Header_t), 1, stream);
    if (entries_read != 1) {
        return false;
    }

    char *name = malloc(header.chars + 1);
    entries_read = fread(name, sizeof(char), header.chars, stream);
    if (entries_read != header.chars) {
        free(name);
        return false;
    }
    name[header.chars] = '\0';

    *entry = (Pak_Entry_t){
            .name = name,
            .id = { 0 },
            .offset = header.offset,
            .size = header.size
        };

    char id[PAK_ID_LENGTH_SZ] = { 0 };
    _hash_file(name, entry->id, id);
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "entry `%s` id is `%s`", name, id);

    return true;
}

static void _free_directory(Pak_Entry_t *directory)
{
    if (!directory) { // Corner case, array empty.
        return;
    }
    for (size_t i = 0; i < (size_t)arrlen(directory); ++i) {
        free(directory[i].name);
    }
    arrfree(directory);
}

static int _pak_entry_compare(const void *lhs, const void *rhs)
{
    const Pak_Entry_t *l = (const Pak_Entry_t *)lhs;
    const Pak_Entry_t *r = (const Pak_Entry_t *)rhs;
    return strcasecmp(l->name, r->name);
}

FS_Mount_t *FS_pak_mount(const char *path)
{
    FILE *stream = fopen(path, "rb");
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access file `%s`", path);
        return NULL;
    }

    uint8_t flags;
    bool validated = _pak_validate_archive(stream, path, &flags);
    if (!validated) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't validate file `%s` as archive", path);
        fclose(stream);
        return NULL;
    }

    bool seeked = fseek(stream, -((long)sizeof(Pak_Index_t)), SEEK_END) == 0; // Cast to fix on x64 Windows build.
    if (!seeked) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't seek directory-header in archive `%s`", path);
        fclose(stream);
        return NULL;
    }

    Pak_Index_t index;
    size_t entries_read = fread(&index, sizeof(Pak_Index_t), 1, stream);
    if (entries_read != 1) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read directory-header from archive `%s`", path);
        fclose(stream);
        return NULL;
    }

    seeked = fseek(stream, (long)index.offset, SEEK_SET) == 0;
    if (!seeked) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't seek directory-header in archive `%s`", path);
        fclose(stream);
        return NULL;
    }

    Pak_Entry_t *directory = NULL;
    for (uint32_t i = 0; i < index.entries; ++i) {
        Pak_Entry_t entry = { 0 };
        bool read = _read_entry(&entry, stream);
        if (!read) {
            Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read entry #%d (%d total) from archive `%s`", i, index.entries, entries_read, path);
            _free_directory(directory);
            fclose(stream);
            return NULL;
        }
        arrpush(directory, entry);
    }

    fclose(stream);

    qsort(directory, index.entries, sizeof(Pak_Entry_t), _pak_entry_compare); // Keep sorted to use binary-search.
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "directory w/ %d entries sorted", index.entries);

    FS_Mount_t *mount = malloc(sizeof(Pak_Mount_t));
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate mount for archive `%s`", path);
        _free_directory(directory);
        return NULL;
    }

    _pak_mount_ctor(mount, path, index.entries, directory, flags);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "mount initialized w/ %d entries (flags 0x%02x) for archive `%s`", index.entries, flags, path);

    return mount;
}

static void _pak_mount_ctor(FS_Mount_t *mount, const char *path, size_t entries, Pak_Entry_t *directory, uint8_t flags)
{
    Pak_Mount_t *pak_mount = (Pak_Mount_t *)mount;

    *pak_mount = (Pak_Mount_t){
            .vtable = (Mount_VTable_t){
                .dtor = _pak_mount_dtor,
                .contains = _pak_mount_contains,
                .open = _pak_mount_open
            },
            .path = { 0 },
            .entries = entries,
            .directory = directory,
            .flags = flags
        };

    strncpy(pak_mount->path, path, PLATFORM_PATH_MAX - 1);
}

static void _pak_mount_dtor(FS_Mount_t *mount)
{
    Pak_Mount_t *pak_mount = (Pak_Mount_t *)mount;

    _free_directory(pak_mount->directory);
}

static bool _pak_mount_contains(const FS_Mount_t *mount, const char *name)
{
    const Pak_Mount_t *pak_mount = (const Pak_Mount_t *)mount;

    Pak_Entry_t key = { .name = (char *)name };
    const Pak_Entry_t *entry = bsearch((const void *)&key, pak_mount->directory, pak_mount->entries, sizeof(Pak_Entry_t), _pak_entry_compare);

    bool exists = entry; // FIXME: should be `!!`?
    Log_assert(!exists, LOG_LEVELS_TRACE, LOG_CONTEXT, "entry `%s` found in mount %p", name, pak_mount);
    return exists;
}

static FS_Handle_t *_pak_mount_open(const FS_Mount_t *mount, const char *name)
{
    const Pak_Mount_t *pak_mount = (const Pak_Mount_t *)mount;

    Pak_Entry_t key = { .name = (char *)name };
    const Pak_Entry_t *entry = bsearch((const void *)&key, pak_mount->directory, pak_mount->entries, sizeof(Pak_Entry_t), _pak_entry_compare);
    if (!entry) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't find entry `%s`", name);
        return NULL;
    }

    FILE *stream = fopen(pak_mount->path, "rb"); // Always in binary mode, line-terminators aren't an issue.
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't access entry `%s`", pak_mount->path);
        return NULL;
    }

    bool seeked = fseek(stream, (long)entry->offset, SEEK_SET) == 0; // Move to the found entry position into the file.
    if (!seeked) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't seek entry `%s` at offset %d in archive `%s`", name, entry->offset, pak_mount->path);
        fclose(stream);
        return NULL;
    }
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "entry `%s` located at offset %d in archive `%s`", name, entry->offset, pak_mount->path);

    FS_Handle_t *handle = malloc(sizeof(Pak_Handle_t));
    if (!handle) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate handle for entry `%s`", name);
        fclose(stream);
        return NULL;
    }

    _pak_handle_ctor(handle, stream, (long)entry->offset, (size_t)entry->size, pak_mount->flags & PAK_FLAG_ENCRYPTED, entry->id);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry `%s` opened w/ handle %p (%d bytes)", name, handle, entry->size);

    return handle;
}

static void _pak_handle_ctor(FS_Handle_t *handle, FILE *stream, long offset, size_t size, bool encrypted, const uint8_t id[PAK_ID_LENGTH])
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
            .cipher_context = { { 0 } } // Uh! The first member of the structure is an array, need additional braces!
        };

    if (encrypted) {
        // Encryption is implemented throught a XOR stream cipher.
        // The key is the entry's MD5 digest of the name.
        xor_schedule(&pak_handle->cipher_context, id, PAK_ID_LENGTH);
#ifdef __DEBUG_FS_CALLS__
        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "cipher context initialized");
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

#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p is", std_handle);
#endif  /* VERBOSE_DEBUG */

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
        xor_process(&pak_handle->cipher_context, buffer, buffer, bytes_read);
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
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "offset %d (position %d) is outside valid range for handle %p", offset, position, handle);
        return false;
    }

    bool seeked = fseek(pak_handle->stream, position, SEEK_SET) == 0;
#ifdef __DEBUG_FS_CALLS__
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d bytes seeked w/ mode %d for handle %p w/ result %d", offset, whence, handle, seeked);
#endif

    if (pak_handle->encrypted) {
        size_t index = position - pak_handle->beginning_of_stream;
        xor_adjust(&pak_handle->cipher_context, index);
#ifdef __DEBUG_FS_CALLS__
        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "cipher context adjusted to %d", index);
#endif
    }

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
