/*
 * MIT License
 *
 * Copyright (c) 2019-2023 Marco Lizza
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

#include "internal.h"

#include <core/platform.h>
#include <libs/bytes.h>
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

/*
+---------+
| HEADER  | sizeof(Pak_Header_t)
+---------+
| ENTRY 0 | sizeof(Pak_Entry_t) + sizeof(Entry) * sizeof(uint8_t)
+---------+
| ENTRY 1 |    "                                             "
+---------+
    ...
    ...
    ...
+---------+
| ENTRY n |    "                                            "
+---------+

NOTE: `uint16_t` and `uint32_t` data is explicitly stored in little-endian.
*/

#define PAK_ID_LENGTH       MD5_SIZE
#define PAK_ID_LENGTH_SZ    (PAK_ID_LENGTH * 2 + 1)

#pragma pack(push, 1)
typedef struct Pak_Header_s {
    char signature[PAK_SIGNATURE_LENGTH];
    uint8_t version;
    struct { // Bit ordering is implementation dependent, il LE machines lower bits come first.
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        uint8_t encrypted : 1;
        uint8_t : 7;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        uint8_t : 7;
        uint8_t encrypted : 1;
#else
#    error unsupported endianness
#endif
    } flags;
    uint16_t __reserved;
    uint32_t entries;
} Pak_Header_t;

typedef struct Pak_Entry_Header_s {
//    uint32_t checksum;
    uint8_t id[PAK_ID_LENGTH];
    uint32_t size;
    // The struct is followed by:
    //   - sizeof(uint8_t) * size
} Pak_Entry_Header_t;
#pragma pack(pop)

typedef struct Pak_Entry_s {
    uint8_t id[PAK_ID_LENGTH];
    long offset;
    size_t size;
} Pak_Entry_t;

typedef struct Pak_Mount_s {
    Mount_VTable_t vtable; // Matches `FS_Mount_t` structure.
    char path[PLATFORM_PATH_MAX];
    size_t entries;
    Pak_Entry_t *directory;
    struct {
        bool encrypted;
    } flags;
} Pak_Mount_t;

typedef struct Pak_Handle_s {
    Handle_VTable_t vtable; // Matches `FS_Handle_t` structure.
    FILE *stream;
    size_t stream_size;
    long begin_of_stream; // Both begin and end markers are *inclusive*.
    long end_of_stream;
    bool encrypted;
    xor_context_t cipher_context;
} Pak_Handle_t;

static void _pak_mount_ctor(FS_Mount_t *mount, const char *path, size_t entries, Pak_Entry_t *directory, bool encrypted);
static void _pak_mount_dtor(FS_Mount_t *mount);
static bool _pak_mount_contains(const FS_Mount_t *mount, const char *name);
static FS_Handle_t *_pak_mount_open(const FS_Mount_t *mount, const char *name);

static void _pak_handle_ctor(FS_Handle_t *handle, FILE *stream, long begin_of_stream, size_t size, bool encrypted, const uint8_t id[PAK_ID_LENGTH]);
static void _pak_handle_dtor(FS_Handle_t *handle);
static size_t _pak_handle_size(FS_Handle_t *handle);
static size_t _pak_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested);
static bool _pak_handle_seek(FS_Handle_t *handle, long offset, int whence);
static long _pak_handle_tell(FS_Handle_t *handle);
static bool _pak_handle_eof(FS_Handle_t *handle);

static bool _pak_validate_archive(FILE *stream, const char *path)
{
    Pak_Header_t header;
    size_t entries_read = fread(&header, sizeof(Pak_Header_t), 1, stream);
    if (entries_read != 1) {
        LOG_E(LOG_CONTEXT, "can't read header from file `%s`", path);
        return false;
    }
    if (strncmp(header.signature, PAK_SIGNATURE, PAK_SIGNATURE_LENGTH) != 0) {
        LOG_E(LOG_CONTEXT, "file `%s` is not a valid archive", path);
        return false;
    }
    if (header.version != PAK_VERSION) {
        LOG_E(LOG_CONTEXT, "archive `%s` version mismatch (found %d, required %d)", path, header.version, PAK_VERSION);
        return false;
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
        LOG_E(LOG_CONTEXT, "can't access file `%s`", path);
        return false;
    }

    bool is_valid = _pak_validate_archive(stream, path);

    fclose(stream);

    return is_valid;
}

static inline void _to_hex(char sz[PAK_ID_LENGTH_SZ], uint8_t id[PAK_ID_LENGTH])
{
    for (size_t i = 0; i < PAK_ID_LENGTH; ++i) { // Also convert to string representation.
        sprintf(sz + i * 2, "%02x", id[i]);
    }
}

static inline void _hash_file(const char *name, uint8_t id[PAK_ID_LENGTH], char sz[PAK_ID_LENGTH_SZ])
{
    md5_hash_sz(id, name, false);
    _to_hex(sz, id);
}

static bool _read_entry(Pak_Entry_t *entry, FILE *stream)
{
    Pak_Entry_Header_t header;
    size_t entries_read = fread(&header, sizeof(Pak_Entry_Header_t), 1, stream);
    if (entries_read != 1) {
        LOG_E(LOG_CONTEXT, "can't read entry at offset %ld", ftell(stream));
        return false;
    }

    size_t size = bytes_ui32le(header.size);

    *entry = (Pak_Entry_t){ 0 };

    memcpy(entry->id, header.id, PAK_ID_LENGTH);
    entry->offset = ftell(stream);
    entry->size = size;

    char id_hex[PAK_ID_LENGTH_SZ];
    _to_hex(id_hex, entry->id);

    bool sought = fseek(stream, size, SEEK_CUR) != -1; // Skip the entry content.
    if (!sought) {
        LOG_E(LOG_CONTEXT, "can't skip entry `%s`", id_hex);
        return false;
    }

    LOG_T(LOG_CONTEXT, "entry `%s` at %d w/ size %d", id_hex, entry->offset, entry->size);

    return true;
}

static void _free_directory(Pak_Entry_t *directory)
{
    if (!directory) { // Corner case, array empty.
        return;
    }
    arrfree(directory);
}

static int _pak_entry_compare(const void *lhs, const void *rhs)
{
    const Pak_Entry_t *l = (const Pak_Entry_t *)lhs;
    const Pak_Entry_t *r = (const Pak_Entry_t *)rhs;
    return memcmp(l->id, r->id, PAK_ID_LENGTH);
}

// Precondition: the path need to be pre-validated as being an archive.
FS_Mount_t *FS_pak_mount(const char *path)
{
    FILE *stream = fopen(path, "rb");
    if (!stream) {
        LOG_E(LOG_CONTEXT, "can't access file `%s`", path);
        return NULL;
    }

    Pak_Header_t header;
    size_t entries_read = fread(&header, sizeof(Pak_Header_t), 1, stream);
    if (entries_read != 1) {
        LOG_E(LOG_CONTEXT, "can't read header from file `%s`", path);
        goto error_close;
    }

    size_t entries = bytes_ui32le(header.entries);
    LOG_T(LOG_CONTEXT, "archive `%s` contains %d entries", path, entries);

    Pak_Entry_t *directory = NULL;
    for (size_t i = 0; i < entries; ++i) {
        Pak_Entry_t entry = { 0 };
        bool read = _read_entry(&entry, stream);
        if (!read) {
            LOG_E(LOG_CONTEXT, "can't read entry #%d (%d total) from archive `%s`", i, entries, entries_read, path);
            goto error_free_directory;
        }
        arrpush(directory, entry);
    }

    qsort(directory, entries, sizeof(Pak_Entry_t), _pak_entry_compare); // Keep sorted to use binary-search.
    LOG_T(LOG_CONTEXT, "directory w/ %d entries sorted", entries);

    FS_Mount_t *mount = malloc(sizeof(Pak_Mount_t));
    if (!mount) {
        LOG_E(LOG_CONTEXT, "can't allocate mount for archive `%s`", path);
        goto error_free_directory;
    }

    fclose(stream);

    _pak_mount_ctor(mount, path, entries, directory, header.flags.encrypted);

    return mount;

error_free_directory:
    _free_directory(directory);
error_close:
    fclose(stream);
    return NULL;
}

static void _pak_mount_ctor(FS_Mount_t *mount, const char *path, size_t entries, Pak_Entry_t *directory, bool encrypted)
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
            .flags = {
                .encrypted = encrypted
            }
        };

    strncpy(pak_mount->path, path, PLATFORM_PATH_MAX - 1);

    LOG_T(LOG_CONTEXT, "mount %p initialized w/ %d entries (encrypted is %d) for archive `%s`", mount, entries, encrypted, path);
}

static void _pak_mount_dtor(FS_Mount_t *mount)
{
    Pak_Mount_t *pak_mount = (Pak_Mount_t *)mount;

    _free_directory(pak_mount->directory);

    *pak_mount = (Pak_Mount_t){ 0 };

    LOG_T(LOG_CONTEXT, "mount %p uninitialized", mount);
}

static bool _pak_mount_contains(const FS_Mount_t *mount, const char *name)
{
    const Pak_Mount_t *pak_mount = (const Pak_Mount_t *)mount;

    Pak_Entry_t needle = { 0 };
    char id_hex[PAK_ID_LENGTH_SZ];
    _hash_file(name, needle.id, id_hex);
    LOG_T(LOG_CONTEXT, "entry `%s` has id `%s`", name, id_hex);

    Pak_Entry_t *entry = bsearch((const void *)&needle, pak_mount->directory, pak_mount->entries, sizeof(Pak_Entry_t), _pak_entry_compare);
    LOG_IF_T(!entry, LOG_CONTEXT, "entry `%s` not found in mount %p", name, pak_mount);

    bool exists = entry; // FIXME: should be `!!`?
    return exists;
}

static FS_Handle_t *_pak_mount_open(const FS_Mount_t *mount, const char *name)
{
    const Pak_Mount_t *pak_mount = (const Pak_Mount_t *)mount;

    Pak_Entry_t needle = { 0 };
    char id_hex[PAK_ID_LENGTH_SZ];
    _hash_file(name, needle.id, id_hex);
    LOG_T(LOG_CONTEXT, "entry `%s` has id `%s`", name, id_hex);

    Pak_Entry_t *entry = bsearch((const void *)&needle, pak_mount->directory, pak_mount->entries, sizeof(Pak_Entry_t), _pak_entry_compare);
    if (!entry) {
        LOG_E(LOG_CONTEXT, "can't find entry `%s`", name);
        return NULL;
    }

    FILE *stream = fopen(pak_mount->path, "rb"); // Always in binary mode, line-terminators aren't an issue.
    if (!stream) {
        LOG_E(LOG_CONTEXT, "can't access entry `%s`", pak_mount->path);
        return NULL;
    }

    bool sought = fseek(stream, entry->offset, SEEK_SET) != -1; // Move to the found entry position into the file.
    if (!sought) {
        LOG_E(LOG_CONTEXT, "can't seek entry `%s` at offset %d in archive `%s`", name, entry->offset, pak_mount->path);
        goto error_close;
    }
    LOG_T(LOG_CONTEXT, "entry `%s` located at offset %d in archive `%s`", name, entry->offset, pak_mount->path);

    FS_Handle_t *handle = malloc(sizeof(Pak_Handle_t));
    if (!handle) {
        LOG_E(LOG_CONTEXT, "can't allocate handle for entry `%s`", name);
        goto error_close;
    }

    _pak_handle_ctor(handle, stream, entry->offset, entry->size, pak_mount->flags.encrypted, entry->id);

    LOG_D(LOG_CONTEXT, "entry `%s` opened w/ handle %p (%d bytes)", name, handle, entry->size);

    return handle;

error_close:
    fclose(stream);
    return NULL;
}

static void _pak_handle_ctor(FS_Handle_t *handle, FILE *stream, long begin_of_stream, size_t size, bool encrypted, const uint8_t id[PAK_ID_LENGTH])
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
            .begin_of_stream = begin_of_stream,
            .end_of_stream = begin_of_stream + (long)size - 1L,
            .encrypted = encrypted,
            .cipher_context = { { 0 } } // Uh! The first member of the structure is an array, need additional braces!
        };

    if (encrypted) {
        // Encryption is implemented w/ a XOR stream cipher.
        // The shared-key is the entry's id.
        xor_schedule(&pak_handle->cipher_context, id, PAK_ID_LENGTH);
#ifdef __DEBUG_FS_CALLS__
        LOG_T(LOG_CONTEXT, "cipher context initialized");
#endif
    }

    LOG_T(LOG_CONTEXT, "handle %p initialized at %ld (%d bytes)", handle, begin_of_stream, size);
}

static void _pak_handle_dtor(FS_Handle_t *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    fclose(pak_handle->stream);

    LOG_T(LOG_CONTEXT, "handle %p uninitialized", handle);
}

static size_t _pak_handle_size(FS_Handle_t *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

#ifdef VERBOSE_DEBUG
    LOG_D(LOG_CONTEXT, "handle %p is", std_handle);
#endif  /* VERBOSE_DEBUG */

    return pak_handle->stream_size;
}

static size_t _pak_handle_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long position = ftell(pak_handle->stream);
    if (position == -1) {
        LOG_E(LOG_CONTEXT, "can't get current position for handle %p", handle);
        return 0;
    }

    size_t bytes_available = (size_t)(pak_handle->end_of_stream - position + 1);

    size_t bytes_to_read = bytes_requested;
    if (bytes_to_read > bytes_available) {
        bytes_to_read = bytes_available;
    }

    size_t bytes_read = fread(buffer, sizeof(uint8_t), bytes_to_read, pak_handle->stream);
#ifdef __DEBUG_FS_CALLS__
    LOG_T(LOG_CONTEXT, "%d bytes read out of %d (%d requested)", bytes_read, bytes_to_read, bytes_requested);
#endif

    if (pak_handle->encrypted) {
        xor_process(&pak_handle->cipher_context, buffer, buffer, bytes_read);
#ifdef __DEBUG_FS_CALLS__
        LOG_T(LOG_CONTEXT, "%d bytes decrypted", bytes_read);
#endif
    }

#ifdef __DEBUG_FS_CALLS__
    LOG_D(LOG_CONTEXT, "%d bytes read for handle %p", bytes_read, handle);
#endif
    return bytes_read;
}

static bool _pak_handle_seek(FS_Handle_t *handle, long offset, int whence)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long origin;
    if (whence == SEEK_SET) {
        origin = pak_handle->begin_of_stream;
    } else
    if (whence == SEEK_CUR) {
        origin = ftell(pak_handle->stream);
    } else
    if (whence == SEEK_END) {
        origin = pak_handle->end_of_stream;
    } else {
        LOG_E(LOG_CONTEXT, "wrong seek mode %d for handle %p", whence, handle);
        return false;
    }

    long position = origin + offset;
    if (position < pak_handle->begin_of_stream || position > pak_handle->end_of_stream) {
        LOG_E(LOG_CONTEXT, "offset %d (position %d) is outside valid range for handle %p", offset, position, handle);
        return false;
    }

    bool sought = fseek(pak_handle->stream, position, SEEK_SET) == 0;
#ifdef __DEBUG_FS_CALLS__
    LOG_T(LOG_CONTEXT, "%d bytes sought w/ mode %d for handle %p w/ result %d", offset, whence, handle, sought);
#endif

    if (pak_handle->encrypted) { // If encrypted, re-sync the cipher to the sought position.
        size_t index = position - pak_handle->begin_of_stream;
        xor_seek(&pak_handle->cipher_context, index);
#ifdef __DEBUG_FS_CALLS__
        LOG_T(LOG_CONTEXT, "cipher context adjusted to %d", index);
#endif
    }

    return sought;
}

static long _pak_handle_tell(FS_Handle_t *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    return ftell(pak_handle->stream) - pak_handle->begin_of_stream;
}

static bool _pak_handle_eof(FS_Handle_t *handle)
{
    Pak_Handle_t *pak_handle = (Pak_Handle_t *)handle;

    long position = ftell(pak_handle->stream);
    if (position == -1) {
        LOG_E(LOG_CONTEXT, "can't get current position for handle %p", handle);
        return true;
    }

    bool end_of_file = position > pak_handle->end_of_stream;
#ifdef __DEBUG_FS_CALLS__
    LOG_IF_D(end_of_file, LOG_CONTEXT, "end-of-file reached for handle %p", handle);
#endif
    return end_of_file;
}
