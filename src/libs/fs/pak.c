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

#include <core/config.h>
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
| ENTRY 0 | sizeof(Pak_Entry_t)
+---------+
| ENTRY 1 |     "         "
+---------+
    ...
    ...
    ...
+---------+
| ENTRY n |     "         "
+---------+
| DATA 0  | sizeof(Entry) * sizeof(uint8_t)
+---------+
| DATA 1  |     "                     "
+---------+
    ...
    ...
    ...
+---------+
| DATA n  |     "                     "
+---------+

NOTE: `uint16_t` and `uint32_t` data are explicitly stored in little-endian.
*/

#define PAK_ID_LENGTH       MD5_SIZE
#define PAK_ID_LENGTH_SZ    (PAK_ID_LENGTH * 2 + 1)

#define PAK_KEY_LENGTH      PAK_ID_LENGTH

#pragma pack(push, 1)
typedef struct Pak_Header_s {
    char signature[PAK_SIGNATURE_LENGTH];
    uint8_t version;
    struct { // Bit ordering is implementation dependent, il LE machines lower bits come first.
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        uint8_t encrypted : 1;
        uint8_t sorted : 1;
        uint8_t : 6;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        uint8_t : 6;
        uint8_t sorted : 1;
        uint8_t encrypted : 1;
#else
#    error unsupported endianness
#endif
    } flags;
    uint16_t __reserved;
    uint32_t entries;
} Pak_Header_t;

typedef struct Pak_Entry_Header_s {
    uint8_t id[PAK_ID_LENGTH];
    uint32_t offset;
    uint32_t size;
//    uint32_t checksum;
} Pak_Entry_Header_t;
#pragma pack(pop)

typedef struct Pak_Entry_s {
    uint8_t id[PAK_ID_LENGTH];
    long offset;
    size_t size;
} Pak_Entry_t;

typedef bool (*Pak_Search_Function_t)(FILE *stream, size_t entries, const uint8_t id[PAK_ID_LENGTH], Pak_Entry_Header_t *header);

typedef struct Pak_Mount_s {
    Mount_VTable_t vtable; // Matches `FS_Mount_t` structure.
    char path[PLATFORM_PATH_MAX];
    size_t entries;
    Pak_Search_Function_t search;
    struct {
        bool encrypted;
        bool sorted;
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

static void _pak_mount_ctor(FS_Mount_t *mount, const char *path, size_t entries, bool encrypted, bool sorted);
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

// FIXME: cache some entries into memory in order to avoid repeated seeks?
static bool _peek_entry(FILE *stream, size_t index, Pak_Entry_Header_t *header)
{
    long offset = sizeof(Pak_Header_t) + index * sizeof(Pak_Entry_Header_t);

    bool sought = fseek(stream, offset, SEEK_SET) != -1;
    if (!sought) {
        return false;
    }

    size_t entries_read = fread(header, sizeof(Pak_Entry_Header_t), 1, stream);
    if (entries_read != 1) {
        return false;
    }

    return true;
}

static bool _linear_search(FILE *stream, size_t entries, const uint8_t id[PAK_ID_LENGTH], Pak_Entry_Header_t *header)
{
    for (size_t i = 0; i < entries; ++i) {
        bool read = _peek_entry(stream, i, header);
        if (!read) {
            LOG_E(LOG_CONTEXT, "can't read header #%d", i);
            return false;
        }

        int ordering = memcmp(header->id, id, PAK_ID_LENGTH);
        if (ordering == 0) {
            return true;
        }
    }

    return false;
}

static bool _binary_search(FILE *stream, size_t entries, const uint8_t id[PAK_ID_LENGTH], Pak_Entry_Header_t *header)
{
    size_t l = 0;
    size_t u = entries - 1;

    while (l <= u) {
        size_t m = (l + u) / 2;
        bool read = _peek_entry(stream, m, header);
        if (!read) {
            LOG_E(LOG_CONTEXT, "can't read header #%d", m);
            return false;
        }

        int ordering = memcmp(header->id, id, PAK_ID_LENGTH);
        if (ordering < 0) {
            l = m + 1;
        } else
        if (ordering > 0) {
            u = m - 1;
        } else {
            return true;
        }
    }

    return false;
}

static FILE *_find_entry(const Pak_Mount_t *pak_mount, const char *name, Pak_Entry_t *entry)
{
    FILE *stream = fopen(pak_mount->path, "rb"); // Always in binary mode, line-terminators aren't an issue.
    if (!stream) {
        LOG_E(LOG_CONTEXT, "can't access entry `%s`", pak_mount->path);
        return NULL;
    }

    char id_hex[PAK_ID_LENGTH_SZ];
    _hash_file(name, entry->id, id_hex);
    LOG_T(LOG_CONTEXT, "entry `%s` has id `%s`", name, id_hex);

    Pak_Entry_Header_t header = { 0 };
    bool found = pak_mount->search(stream,  pak_mount->entries, entry->id, &header);
    if (!found) {
        goto error_close;
    }

    // Once read we need to marshal the serialized entry to conform the data size
    // of the architecture we are running into. Endian-ness need to be fixed, and
    // data need to expanded (32- to 64-bit).
    entry->offset = bytes_ui32le(header.offset);
    entry->size = bytes_ui32le(header.size);

    LOG_T(LOG_CONTEXT, "entry `%s` w/ size %u located at offset %d in archive `%s`", name, entry->size, entry->offset, pak_mount->path);

    return stream;

error_close:
    fclose(stream);
    return NULL;
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

    FS_Mount_t *mount = malloc(sizeof(Pak_Mount_t));
    if (!mount) {
        LOG_E(LOG_CONTEXT, "can't allocate mount for archive `%s`", path);
        goto error_close;
    }

    fclose(stream);

    _pak_mount_ctor(mount, path, entries, header.flags.encrypted, header.flags.sorted);

    return mount;

error_close:
    fclose(stream);
    return NULL;
}

static void _pak_mount_ctor(FS_Mount_t *mount, const char *path, size_t entries, bool encrypted, bool sorted)
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
            .search = sorted ? _binary_search : _linear_search,
            .flags = {
                .encrypted = encrypted,
                sorted
            }
        };

    strncpy(pak_mount->path, path, PLATFORM_PATH_MAX - 1);

    LOG_T(LOG_CONTEXT, "mount %p initialized w/ %d entries (encrypted is %d, sorted is %d) for archive `%s`", mount, entries, encrypted, sorted, path);
}

static void _pak_mount_dtor(FS_Mount_t *mount)
{
    Pak_Mount_t *pak_mount = (Pak_Mount_t *)mount;

    *pak_mount = (Pak_Mount_t){ 0 };

    LOG_T(LOG_CONTEXT, "mount %p uninitialized", mount);
}

static bool _pak_mount_contains(const FS_Mount_t *mount, const char *name)
{
    const Pak_Mount_t *pak_mount = (const Pak_Mount_t *)mount;

    Pak_Entry_t entry;
    FILE *stream = _find_entry(pak_mount, name, &entry);

    bool found = !!stream;
    LOG_IF_T(!found, LOG_CONTEXT, "entry `%s` not found in mount %p", name, pak_mount);

    if (stream) {
        fclose(stream);
    }

    return found;
}

static FS_Handle_t *_pak_mount_open(const FS_Mount_t *mount, const char *name)
{
    const Pak_Mount_t *pak_mount = (const Pak_Mount_t *)mount;

    Pak_Entry_t entry;
    FILE *stream = _find_entry(pak_mount, name, &entry);
    if (!stream) {
        LOG_E(LOG_CONTEXT, "can't find entry `%s` in mount %p", name, pak_mount);
        return NULL;
    }

    bool sought = fseek(stream, entry.offset, SEEK_SET) != -1; // Move to the found entry position into the file.
    if (!sought) {
        LOG_E(LOG_CONTEXT, "can't seek entry `%s` at offset %d mount %p", name, entry.offset, pak_mount);
        goto error_close;
    }

    FS_Handle_t *handle = malloc(sizeof(Pak_Handle_t));
    if (!handle) {
        LOG_E(LOG_CONTEXT, "can't allocate handle for entry `%s`", name);
        goto error_close;
    }

    _pak_handle_ctor(handle, stream, entry.offset, entry.size, pak_mount->flags.encrypted, entry.id);

    LOG_D(LOG_CONTEXT, "entry `%s` opened w/ handle %p", name, handle);

    return handle;

error_close:
    fclose(stream);
    return NULL;
}

static inline void _derive_key(uint8_t key[PAK_KEY_LENGTH], const void *data, size_t length)
{
    // The shared-key is the digest of the entry's id (so that we won't be using the exact
    // data as stored it the file)
    md5_hash(key, data, length);
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

    if (encrypted) { // Encryption is implemented w/ a XOR stream cipher.
        uint8_t key[PAK_KEY_LENGTH];
        _derive_key(key, id, PAK_ID_LENGTH);

        xor_schedule(&pak_handle->cipher_context, key, PAK_ID_LENGTH);
#if defined(TOFU_FILE_DEBUG_ENABLED)
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

#if defined(VERBOSE_DEBUG)
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
#if defined(TOFU_FILE_DEBUG_ENABLED)
    LOG_T(LOG_CONTEXT, "%d bytes read out of %d (%d requested)", bytes_read, bytes_to_read, bytes_requested);
#endif

    if (pak_handle->encrypted) {
        xor_process(&pak_handle->cipher_context, buffer, buffer, bytes_read);
#if defined(TOFU_FILE_DEBUG_ENABLED)
        LOG_T(LOG_CONTEXT, "%d bytes decrypted", bytes_read);
#endif
    }

#if defined(TOFU_FILE_DEBUG_ENABLED)
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
#if defined(TOFU_FILE_DEBUG_ENABLED)
    LOG_T(LOG_CONTEXT, "%d bytes sought w/ mode %d for handle %p w/ result %d", offset, whence, handle, sought);
#endif

    if (pak_handle->encrypted) { // If encrypted, re-sync the cipher to the sought position.
        size_t index = position - pak_handle->begin_of_stream;
        xor_seek(&pak_handle->cipher_context, index);
#if defined(TOFU_FILE_DEBUG_ENABLED)
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
#if defined(TOFU_FILE_DEBUG_ENABLED)
    LOG_IF_D(end_of_file, LOG_CONTEXT, "end-of-file reached for handle %p", handle);
#endif
    return end_of_file;
}
