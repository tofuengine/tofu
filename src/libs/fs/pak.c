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
| BLOB 0  | N * sizeof(char)
+---------+
| BLOB 1  |    "        "
+---------+
    ...
    ...
    ...
+---------+
| BLOB n  |    "        "
+---------+
| ENTRY 0 | sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint16_t) + N * sizeof(char)
+---------+
| ENTRY 1 |    "                                                                 "
+---------+
    ...
    ...
    ...
+---------+
| ENTRY n |    "                                                                 "
+---------+
|  INDEX  | sizeof(Pak_Index_t)
+---------+

NOTE: `uint16_t` and `uint32_t` data is explicitly stored in little-endian.
*/

#define PAK_ID_LENGTH       MD5_SIZE
#define PAK_ID_LENGTH_SZ    (MD5_SIZE * 2 + 1)

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
} Pak_Header_t;

typedef struct Pak_Entry_Header_s {
    int32_t offset;
    uint32_t size;
    uint16_t chars;
} Pak_Entry_Header_t;

typedef struct Pak_Index_s {
    int32_t offset;
    uint32_t entries; // Redundant, we could check file offsets, but it's quicker that way.
} Pak_Index_t;
#pragma pack(pop)

typedef struct Pak_Entry_s {
    char *name;
    uint8_t id[PAK_ID_LENGTH];
    long offset;
    size_t size;
} Pak_Entry_t;

// TODO: ditch in-memory directory and seek always from file?
typedef struct Pak_Mount_s {
    Mount_VTable_t vtable; // Matches `_FS_Mount_t` structure.
    char path[PLATFORM_PATH_MAX];
    size_t entries;
    Pak_Entry_t *directory;
    struct {
        bool encrypted;
    } flags;
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

static void _pak_mount_ctor(FS_Mount_t *mount, const char *path, size_t entries, Pak_Entry_t *directory, bool encrypted);
static void _pak_mount_dtor(FS_Mount_t *mount);
static void _pak_mount_scan(const FS_Mount_t *mount, FS_Scan_Callback_t callback, void *user_data);
static bool _pak_mount_contains(const FS_Mount_t *mount, const char *name);
static FS_Handle_t *_pak_mount_open(const FS_Mount_t *mount, const char *name);

static void _pak_handle_ctor(FS_Handle_t *handle, FILE *stream, long offset, size_t size, bool encrypted, const uint8_t id[PAK_ID_LENGTH]);
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

    size_t chars = bytes_ui16le(header.chars);
    long offset = bytes_i32le(header.offset);
    size_t size = bytes_ui32le(header.size);
    
    char *name = malloc(chars + 1);
    if (!name) {
        LOG_E(LOG_CONTEXT, "can't allocate entry's name");
        return false;
    }

    entries_read = fread(name, sizeof(char), chars, stream);
    if (entries_read != chars) {
        goto error_free;
    }
    name[chars] = '\0';

    *entry = (Pak_Entry_t){
            .name = name,
            .id = { 0 },
            .offset = offset,
            .size = size
        };

    char id[PAK_ID_LENGTH_SZ] = { 0 };
    _hash_file(name, entry->id, id);
    LOG_T(LOG_CONTEXT, "entry `%s` id is `%s`", name, id);

    return true;

error_free:
    free(name);
    return false;
}

static void _free_directory(Pak_Entry_t *directory)
{
    if (!directory) { // Corner case, array empty.
        return;
    }
    for (size_t i = 0; i < arrlenu(directory); ++i) {
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

    bool seeked = fseek(stream, -((long)sizeof(Pak_Index_t)), SEEK_END) == 0; // Cast to fix on x64 Windows build.
    if (!seeked) {
        LOG_E(LOG_CONTEXT, "can't seek directory-header in archive `%s`", path);
        goto error_close;
    }

    Pak_Index_t index;
    entries_read = fread(&index, sizeof(Pak_Index_t), 1, stream);
    if (entries_read != 1) {
        LOG_E(LOG_CONTEXT, "can't read directory-header from archive `%s`", path);
        goto error_close;
    }

    long offset = bytes_i32le(index.offset);
    size_t entries = bytes_ui32le(index.entries);

    seeked = fseek(stream, offset, SEEK_SET) == 0;
    if (!seeked) {
        LOG_E(LOG_CONTEXT, "can't seek directory-header in archive `%s`", path);
        goto error_close;
    }

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

    LOG_D(LOG_CONTEXT, "mount initialized w/ %d entries (encrypted is %d) for archive `%s`", entries, header.flags.encrypted, path);

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
                .scan = _pak_mount_scan,
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
}

static void _pak_mount_dtor(FS_Mount_t *mount)
{
    Pak_Mount_t *pak_mount = (Pak_Mount_t *)mount;

    _free_directory(pak_mount->directory);
}

static void _pak_mount_scan(const FS_Mount_t *mount, FS_Scan_Callback_t callback, void *user_data)
{
    const Pak_Mount_t *pak_mount = (const Pak_Mount_t *)mount;

    for (size_t index = 0; index < pak_mount->entries; ++index) {
        const Pak_Entry_t *entry = &pak_mount->directory[index];
        callback(user_data, entry->name);
    }
}

static bool _pak_mount_contains(const FS_Mount_t *mount, const char *name)
{
    const Pak_Mount_t *pak_mount = (const Pak_Mount_t *)mount;

    Pak_Entry_t needle = { .name = (char *)name };
    const Pak_Entry_t *entry = bsearch((const void *)&needle, pak_mount->directory, pak_mount->entries, sizeof(Pak_Entry_t), _pak_entry_compare);

    bool exists = entry; // FIXME: should be `!!`?
    LOG_IF_T(exists, LOG_CONTEXT, "entry `%s` found in mount %p", name, pak_mount);
    return exists;
}

static FS_Handle_t *_pak_mount_open(const FS_Mount_t *mount, const char *name)
{
    const Pak_Mount_t *pak_mount = (const Pak_Mount_t *)mount;

    Pak_Entry_t needle = { .name = (char *)name };
    const Pak_Entry_t *entry = bsearch((const void *)&needle, pak_mount->directory, pak_mount->entries, sizeof(Pak_Entry_t), _pak_entry_compare);
    if (!entry) {
        LOG_E(LOG_CONTEXT, "can't find entry `%s`", name);
        return NULL;
    }

    FILE *stream = fopen(pak_mount->path, "rb"); // Always in binary mode, line-terminators aren't an issue.
    if (!stream) {
        LOG_E(LOG_CONTEXT, "can't access entry `%s`", pak_mount->path);
        return NULL;
    }

    bool seeked = fseek(stream, entry->offset, SEEK_SET) == 0; // Move to the found entry position into the file.
    if (!seeked) {
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
            .end_of_stream = offset + (long)size - 1L,
            .encrypted = encrypted,
            .cipher_context = { { 0 } } // Uh! The first member of the structure is an array, need additional braces!
        };

    if (encrypted) {
        // Encryption is implemented throught a XOR stream cipher.
        // The key is the entry's MD5 digest of the name.
        xor_schedule(&pak_handle->cipher_context, id, PAK_ID_LENGTH);
#ifdef __DEBUG_FS_CALLS__
        LOG_T(LOG_CONTEXT, "cipher context initialized");
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
        origin = pak_handle->beginning_of_stream;
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
    if (position < pak_handle->beginning_of_stream || position > pak_handle->end_of_stream) {
        LOG_E(LOG_CONTEXT, "offset %d (position %d) is outside valid range for handle %p", offset, position, handle);
        return false;
    }

    bool seeked = fseek(pak_handle->stream, position, SEEK_SET) == 0;
#ifdef __DEBUG_FS_CALLS__
    LOG_T(LOG_CONTEXT, "%d bytes seeked w/ mode %d for handle %p w/ result %d", offset, whence, handle, seeked);
#endif

    if (pak_handle->encrypted) { // If encrypted, re-sync the cipher to the seeked position.
        size_t index = position - pak_handle->beginning_of_stream;
        xor_seek(&pak_handle->cipher_context, index);
#ifdef __DEBUG_FS_CALLS__
        LOG_T(LOG_CONTEXT, "cipher context adjusted to %d", index);
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
        LOG_E(LOG_CONTEXT, "can't get current position for handle %p", handle);
        return true;
    }

    bool end_of_file = position > pak_handle->end_of_stream;
#ifdef __DEBUG_FS_CALLS__
    LOG_IF_D(end_of_file, LOG_CONTEXT, "end-of-file reached for handle %p", handle);
#endif
    return end_of_file;
}
