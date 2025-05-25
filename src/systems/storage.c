/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2025 Marco Lizza
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

#include "storage.h"

#include <core/config.h>
#include <libs/image.h>
#define _LOG_TAG "storage"
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>

#include <stdint.h>

typedef bool (*Storage_Load_Function_t)(Storage_Resource_t *resource, FS_Handle_t *handle);

Storage_t *Storage_create(const Storage_Configuration_t *configuration)
{
    Storage_t *storage = malloc(sizeof(Storage_t));
    if (!storage) {
        LOG_E("can't allocate storage");
        goto error_exit;
    }

    *storage = (Storage_t){
            .configuration = *configuration,
            .path = {
                .user = { 0 },
                .local = { 0 }
            }
        };

    path_expand(PLATFORM_PATH_USER, storage->path.user); // Expand and resolve the user-dependent folder.
    LOG_D("user path is `%s`", storage->path.user);

    // This would be correct to do, since the local path is initialized, but it's also very pedant.
    // Storage_set_identity(storage, DEFAULT_IDENTITY);

    storage->context = FS_create();
    if (!storage->context) {
        LOG_E("can't create file-system context");
        goto error_free_storage;
    }
    LOG_D("file-system context %p created", storage->context);

    storage->cache = Storage_Cache_create(storage->context);
    if (!storage->cache) {
        LOG_E("can't create cache");
        goto error_destroy_context;
    }
    LOG_D("cache %p created", storage->cache);

    // Scan for `xxx.pak.0`, `xxx.pak.1`, ...
    //
    // Note: appending an incremental number is more consistent as we don't enforce
    //       the name of the kernal/data archives.
    const char *paths[] = {
            configuration->kernal_path,
            configuration->data_path,
            NULL
        };

    for (int i = 0; paths[i]; ++i) {
        const char *path = paths[i];
        for (int index = -1; ; ++index) { // Start from `-1` as the first entry lacks the extension.
            char archive_path[PATH_MAX];
            if (index == -1) {
                strcpy(archive_path, path);
            } else {
                sprintf(archive_path, "%s.%d", path, index); // Bail out at the first not-existing "indexed" path.
                if (!path_exists(archive_path)) {
                    break;
                }
            }
            LOG_D("attaching folder/archive `%s`", archive_path);

            int mount_id;
            bool archive_attached = FS_attach_folder_or_archive(storage->context, archive_path, FS_PRIORITY_DEFAULT, &mount_id);
            if (!archive_attached) {
                LOG_E("can't attach folder/archive at `%s`", archive_path);
                goto error_destroy_cache;
            }
            LOG_D("folder/archive attached w/ path `%s` (mount-id is #%d)", archive_path, mount_id);
        }
    }

    LOG_D("storage %p created", storage);

    return storage;

error_destroy_cache:
    Storage_Cache_destroy(storage->cache);
error_destroy_context:
    FS_destroy(storage->context);
error_free_storage:
    free(storage);
error_exit:
    return NULL;
}

static void _release(Storage_Resource_t *resource)
{
    LOG_D("releasing resource %p", resource);

    if (resource->type == STORAGE_RESOURCE_STRING) {
        free(resource->var.string.chars);
        LOG_D("resource-data %p at %p freed (%d characters string)",
            resource, resource->var.string.chars, resource->var.string.length);
    } else
    if (resource->type == STORAGE_RESOURCE_BLOB) {
        free(resource->var.blob.ptr);
        LOG_D("resource-data %p at %p freed (%d bytes blob)",
            resource, resource->var.blob.ptr, resource->var.blob.size);
    } else
    if (resource->type == STORAGE_RESOURCE_IMAGE) {
        free(resource->var.image.pixels);
        LOG_D("resource-data %p at %p freed (%dx%d image)",
            resource, resource->var.image.pixels, resource->var.image.width, resource->var.image.height);
    }
    free(resource);
    LOG_D("resource freed");
}

void Storage_destroy(Storage_t *storage)
{
    LOG_D("destroying storage %p", storage);

    if (arrlenu(storage->resources) > 0) {
        LOG_I("releasing %d resources for storage %p", arrlenu(storage->resources), storage);
        Storage_Resource_t **current = storage->resources;
        for (size_t count = arrlenu(storage->resources); count; --count) {
            Storage_Resource_t *resource = *(current++);
            _release(resource);
        }
        LOG_D("storage cache %p emptied", storage->resources);
    }
    arrfree(storage->resources);
    LOG_D("storage cache %p freed", storage->resources);

    Storage_Cache_destroy(storage->cache);
    LOG_D("storage cache %p destroyed", storage->cache);

    FS_destroy(storage->context);
    LOG_D("file-system context %p destroyed", storage->context);

#if defined(TOFU_STORAGE_LEAK_CHECK)
    if (arrlenu(storage->handles) > 0) {
        LOG_E("%u handle(s) were still in use!", arrlenu(storage->handles));
    }
    arrfree(storage->handles);
    LOG_D("file-system handles %p freed", storage->handles);
#endif

    free(storage);
    LOG_D("storage freed");
}

bool Storage_inject_base64(Storage_t *storage, const char *name, const char *encoded_data, size_t length)
{
    return Storage_Cache_inject_base64(storage->cache, name, encoded_data, length);
}

bool Storage_inject_ascii85(Storage_t *storage, const char *name, const char *encoded_data, size_t length)
{
    return Storage_Cache_inject_ascii85(storage->cache, name, encoded_data, length);
}

bool Storage_inject_raw(Storage_t *storage, const char *name, const void *raw_data, size_t size)
{
    return Storage_Cache_inject_raw(storage->cache, name, raw_data, size);
}

bool Storage_set_identity(Storage_t *storage, const char *identity)
{
    // if (storage->path.local[0] != '\0') {
    //     FS_detach(storage->context, storage->path.local);
    //     LOG_D("user-dependent path `%s` detached", storage->path.local);
    // }

    path_join(storage->path.local, storage->path.user, identity); // Build the local storage-path, using identity.

    bool created = path_mkdirs(storage->path.local);
    if (!created) {
        LOG_E("can't create used-dependent path `%s`", storage->path.local);
        return false;
    }

    int mount_id;
    bool attached = FS_attach_folder(storage->context, storage->path.local, FS_PRIORITY_DEFAULT, &mount_id);
    if (!attached) {
        LOG_E("can't attach user-dependent path path `%s`", storage->path.local);
        return false;
    }

    LOG_D("user-dependent path `%s` attached (mount-id is #%d)", storage->path.local, mount_id);
    return true;
}

static void *_load(FS_Handle_t *handle, bool null_terminate, size_t *size)
{
    size_t bytes_requested = FS_size(handle);

    size_t bytes_to_allocate = bytes_requested + (null_terminate ? 1 : 0);
    void *data = malloc(sizeof(uint8_t) * bytes_to_allocate); // Add null terminator for the string.
    if (!data) {
        LOG_E("can't allocate %d bytes of memory", bytes_to_allocate);
        FS_close(handle);
        return NULL;
    }

    size_t bytes_read = FS_read(handle, data, bytes_requested);
    if (bytes_read < bytes_requested) {
        LOG_E("can't read %d bytes of data (%d available)", bytes_requested, bytes_read);
        free(data);
        return NULL;
    }

    if (null_terminate) {
        ((char *)data)[bytes_read] = '\0';
    }

    *size = bytes_read;
    return data;
}

static bool _load_as_string(Storage_Resource_t *resource, FS_Handle_t *handle)
{
    size_t length;
    void *chars = _load(handle, true, &length);
    if (!chars) {
        return false;
    }
    LOG_D("loaded a %d characters long string", length);

    *resource = (Storage_Resource_t){
            .type = STORAGE_RESOURCE_STRING,
            .var = {
                .string = {
                    .chars = (char *)chars,
                    .length = length
                }
            },
#if defined(TOFU_STORAGE_AUTO_COLLECT)
            .age = 0.0
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
        };

    return true;
}

static bool _load_as_blob(Storage_Resource_t *resource, FS_Handle_t *handle)
{
    size_t size;
    void *ptr = _load(handle, false, &size);
    if (!ptr) {
        return false;
    }
    LOG_D("loaded %d bytes blob", size);

    *resource = (Storage_Resource_t){
            .type = STORAGE_RESOURCE_BLOB,
            .var = {
                .blob = {
                    .ptr = ptr,
                    .size = size
                }
            },
#if defined(TOFU_STORAGE_AUTO_COLLECT)
            .age = 0.0
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
        };

    return true;
}

static size_t _read(void *user_data, void *buffer, size_t bytes_to_read)
{
    FS_Handle_t *handle = (FS_Handle_t *)user_data;
    return FS_read(handle, buffer, bytes_to_read);
}

static bool _seek(void *user_data, long offset, int whence)
{
    FS_Handle_t *handle = (FS_Handle_t *)user_data;
    return FS_seek(handle, offset, whence);
}

static long _tell(void *user_data)
{
    const FS_Handle_t *handle = (const FS_Handle_t *)user_data;
    return FS_tell(handle);
}

static bool _eof(void *user_data)
{
    const FS_Handle_t *handle = (const FS_Handle_t *)user_data;
    return FS_eof(handle);
}

static const image_io_callbacks_t _image_io_callbacks = {
    .read = _read,
    .seek = _seek,
    .tell = _tell,
    .eof  = _eof
};

typedef struct _decode_callbacks_closure_s {
    size_t width;
    size_t height;
    size_t bytes_per_pixel;
    void *pixels;
} _decode_callbacks_closure_t;

static bool _on_allocate(void *user_data, size_t width, size_t height, size_t bytes_per_pixel)
{
    _decode_callbacks_closure_t *closure = (_decode_callbacks_closure_t *)user_data;

    void *pixels = malloc(width * height * bytes_per_pixel);
    if (!pixels) {
        LOG_E("can't allocate `%dx%d` pixel-data", width, height);
        return false;
    }

    *closure = (_decode_callbacks_closure_t){
            .width = width,
            .height = height,
            .bytes_per_pixel = bytes_per_pixel,
            .pixels = pixels
        };

    return true;
}

static bool _on_scanline(void *user_data, size_t index, const void *pixels)
{
    _decode_callbacks_closure_t *closure = (_decode_callbacks_closure_t *)user_data;

    void *ptr = (uint8_t *)closure->pixels + (index * closure->width * closure->bytes_per_pixel); // FIXME: can be optimized.

    memcpy(ptr, pixels, closure->width * closure->bytes_per_pixel);

    return true; // Continue decoding.
}

static void _on_free(void *user_data, bool success)
{
    _decode_callbacks_closure_t *closure = (_decode_callbacks_closure_t *)user_data;

    if (!success) {
        LOG_E("decoding failed, destroying surface");
        free(closure->pixels);
    } else {
        LOG_D("image %p decoded", closure->pixels);
    }
}

static const image_decode_callbacks_t _image_decode_callbacks = {
    .on_allocate = _on_allocate,
    .on_scanline = _on_scanline,
    .on_free     = _on_free
};

static bool _load_as_image(Storage_Resource_t *resource, FS_Handle_t *handle)
{
    _decode_callbacks_closure_t decode_callbacks_closure = { 0 };

    bool decoded = image_decode_from_callbacks(&_image_io_callbacks, handle,
        &_image_decode_callbacks, &decode_callbacks_closure);

    if (!decoded) {
        LOG_E("can't decode surface from handle `%p`", handle);
        return NULL;
    }
    LOG_D("loaded %dx%d image", decode_callbacks_closure.width, decode_callbacks_closure.height);

    *resource = (Storage_Resource_t){
            .type = STORAGE_RESOURCE_IMAGE,
            .var = {
                .image = {
                    .width = decode_callbacks_closure.width,
                    .height = decode_callbacks_closure.height,
                    .pixels = decode_callbacks_closure.pixels
                }
            },
#if defined(TOFU_STORAGE_AUTO_COLLECT)
            .age = 0.0
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
        };

    return true;
}

static const Storage_Load_Function_t _load_functions[Storage_Resource_Types_t_CountOf] = {
    _load_as_string,
    _load_as_blob,
    _load_as_image
};

static bool _resource_load(Storage_Resource_t *resource, const char *name, Storage_Resource_Types_t type, const FS_Context_t *context)
{
    FS_Handle_t *handle = FS_open(context, name);
    if (!handle) {
        return false;
    }

    bool loaded = _load_functions[type](resource, handle);
    FS_close(handle);
    return loaded;
}

// We are using a linear search instead for `bsearch()` since the resource cache is limited in size (won't
// exceed hundred of entries, typically). Also, since we were (occasionally) keeping the array sorted by "age",
// binary-searching by name would be impossible! (unless we are re-sorting the array twice just for sake of it)
//
// We single-handedly got rid of both of them. The array is never sorted which means a faster and more cache-friendly
// code. Also, we are removing the entries with the SWAP-AND-POP idiom, which is as fast as possible.
//
// Note: we are receiving and passing a *non* `const` pointer to the resources array as the returned value
//       is usually manipulated and modified in a *non* `const` manner.
static inline Storage_Resource_t *_lookup(Storage_Resource_t **resources, const uint8_t id[STORAGE_RESOURCE_ID_LENGTH])
{
    size_t length = arrlenu(resources);
    for (size_t i = 0; i < length; ++i) {
        Storage_Resource_t *resource = resources[i];
        if (memcmp(resource->id, id, STORAGE_RESOURCE_ID_LENGTH) == 0) {
            return resource;
        }
    }
    return NULL;
}

#if defined(TOFU_STORAGE_CACHE_ENTRIES_LIMIT)
static inline size_t _used_cache_slots(Storage_Resource_t **resources)
{
    const size_t length = arrlenu(resources);
#if defined(TOFU_STORAGE_AUTO_COLLECT)
    // Returns the amount of cache slots occupied by *not "aged" resources, as the one already "aged" will
    // be automatically free on the next update call and are not really going to stay in the cache.
    size_t count = 0;
    for (size_t i = 1; i < length; ++i) {
        const Storage_Resource_t *resource = resources[i];
        if (resource->age >= TOFU_STORAGE_RESOURCE_MAX_AGE) {
            continue;
        }
        count += 1;
    }
#else   /* TOFU_STORAGE_AUTO_COLLECT */
    size_t count = length;
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
    LOG_D("cache is currently holding %d resources", count);
    return count;
}

static inline void _free_cache_slot(Storage_Resource_t **resources)
{
#if defined(TOFU_STORAGE_AUTO_COLLECT)
    // Finds the oldest entry among the resources, we skip already "aged" ones.
    const size_t length = arrlenu(resources);
    Storage_Resource_t *oldest = NULL; // We can't pick the first one, as it might be already "aged".
    for (size_t i = 0; i < length; ++i) {
        Storage_Resource_t *resource = resources[i];
        if (resource->age >= TOFU_STORAGE_RESOURCE_MAX_AGE) {
            continue;
        }
        if (!oldest || oldest->age < resource->age) {
            oldest = resource;
        }
    }

    if (!oldest) { // This is mostly an assertion, as it is IMPOSSIBLE that a resource is not found as to-be-released!
        LOG_W("Great Scott! No resources marked for release!");
        return;
    }

    oldest->age = TOFU_STORAGE_RESOURCE_MAX_AGE; // Mark the oldest for release in the next cycle.
    LOG_D("resource %p marked for release", oldest);
#else   /* TOFU_STORAGE_AUTO_COLLECT */
    Storage_Resource_t *oldest = resources[0];
    arrdel(resources, 0); // FIFO removal.
    LOG_D("resource %p released", oldest);
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
}
#endif

bool Storage_exists(Storage_t *storage, const char *name)
{
    return FS_exists(storage->context, name);
}

static inline void _calculate_id(uint8_t id[STORAGE_RESOURCE_ID_LENGTH], const char *name)
{
    // The resources id is a digest of the name in order to have fixed records and
    // reduce the amount of heap operations and fragmentation.
    md5_hash_sz(id, name, false); // TODO: move to SHA1?
}

Storage_Resource_t *Storage_load(Storage_t *storage, const char *name, Storage_Resource_Types_t type)
{
    if (path_is_absolute(name) || !path_is_normalized(name)) {
        LOG_E("path `%s` is not allowed (only relative non-parent paths are permitted in sandbox mode)", name);
        goto error_exit;
    }

    uint8_t id[STORAGE_RESOURCE_ID_LENGTH];
    _calculate_id(id, name);

    Storage_Resource_t *entry = _lookup(storage->resources, id);
    if (entry) {
        LOG_D("cache-hit for resource `%s`, resetting age and returning", name);
#if defined(TOFU_STORAGE_AUTO_COLLECT)
        entry->age = 0.0f; // Reset age on cache-hit.
#endif  /* TOFU_STORAGE_AUTO_COLLECT) */
        return entry;
    }

    Storage_Resource_t *resource = malloc(sizeof(Storage_Resource_t));
    if (!resource) {
        LOG_E("can't allocate resource");
        goto error_exit;
    }

    if (_resource_load(resource, name, type, storage->context)) {
        LOG_D("resource `%s` loaded from file-system", name);
    } else {
        // Note: not being able to load a resource could be considered an error
        //       worth having a proper "logging trace". However, in many cases
        //       we are trying and load a file, but only the called has the
        //       right to decide if that's an error!
        goto error_free_resource;
    }
    _calculate_id(resource->id, name);

    arrpush(storage->resources, resource);

#if defined(TOFU_STORAGE_CACHE_ENTRIES_LIMIT)
    if (_used_cache_slots(storage->resources) > TOFU_STORAGE_CACHE_ENTRIES_LIMIT) {
        LOG_D("cache is full, picking a resource to release");
        _free_cache_slot(storage->resources);
    }
#endif

    LOG_D("resource `%s` stored as %p", name, resource);

    return resource;

error_free_resource:
    free(resource);
error_exit:
    return NULL;
}

// static void _stbi_write_func(void *context, void *data, int size)
// {
//     FILE *stream = (FILE *)context;
//     size_t bytes_to_write = (size_t)size;
//     size_t bytes_written = fwrite(data, sizeof(uint8_t), bytes_to_write, stream);
//     if (bytes_written != bytes_to_write) {
//         LOG_E("can't write %d byte(s) (%d written)", bytes_to_write, bytes_written);
//     }
// }

// This function saves a file into the local user-dependent storage. The mount points aren't modified.
bool Storage_store(Storage_t *storage, const char *name, const Storage_Resource_t *resource)
{
    char path[PLATFORM_PATH_MAX] = { 0 };
    path_join(path, storage->path.local, name);

    FILE *stream = fopen(path, "wb");
    if (!stream) {
        LOG_E("can't create file `%s`", path);
        return false;
    }

    bool result = false;

    switch (resource->type) {
        case STORAGE_RESOURCE_STRING: {
            size_t chars_to_write = SR_SLENTGH(resource);
            size_t chars_written = fwrite(SR_SCHARS(resource), sizeof(char), chars_to_write, stream);
            result = chars_written == chars_to_write;
            break;
        }
        case STORAGE_RESOURCE_BLOB: {
            size_t bytes_to_write = SR_BSIZE(resource);
            size_t bytes_written = fwrite(SR_BPTR(resource), sizeof(uint8_t), bytes_to_write, stream);
            result = bytes_written == bytes_to_write;
            break;
        }
        case STORAGE_RESOURCE_IMAGE: {
//            int done = stbi_write_png_to_func(_stbi_write_func, (void *)stream, SR_IWIDTH(resource), SR_IHEIGHT(resource), 4, SR_IPIXELS(resource), SR_IWIDTH(resource) * 4);
            int done = 0;
            result = done != 0;
            break;
        }
        default: {
            break;
        }
    }

    fclose(stream);

    if (!result) {
        LOG_E("can't write resource `%s` w/ type %d to file `%s`", name, resource->type, path);
    }

    return result;
}

FS_Handle_t *Storage_open(Storage_t *storage, const char *name)
{
    FS_Handle_t *handle = FS_open(storage->context, name);
    if (!handle) {
        return NULL;
    }
#if defined(TOFU_STORAGE_LEAK_CHECK)
    arrpush(storage->handles, handle);
#endif
    return handle;
}

void Storage_close(Storage_t *storage, FS_Handle_t *handle)
{
#if defined(TOFU_STORAGE_LEAK_CHECK)
    // Backward scan, to properly implement the SWAP-AND-POP(tm) idiom along the whole array
    // when removing the handle reference.
    for (ptrdiff_t index = arrlen(storage->handles) - 1; index >= 0; --index) {
        if (storage->handles[index] != handle) {
            continue;
        }
        arrdelswap(storage->handles, index);
    }
#endif
    FS_close(handle);
}

#if defined(TOFU_STORAGE_AUTO_COLLECT)
bool Storage_update(Storage_t *storage, float delta_time)
{
    // Backward scan, to properly implement the SWAP-AND-POP(tm) idiom along the whole array
    // when removing the to-be-released resources.
    for (ptrdiff_t index = arrlen(storage->resources) - 1; index >= 0; --index) {
        Storage_Resource_t *resource = storage->resources[index];
        resource->age += delta_time;
        if (resource->age < TOFU_STORAGE_RESOURCE_MAX_AGE) {
            continue;
        }

        _release(resource); // Release the way-too-old resource...

        arrdelswap(storage->resources, index); // ... shrink the array!
    }

    return true;
}
#else   /* TOFU_STORAGE_AUTO_COLLECT */
size_t Storage_flush(Storage_t *storage)
{
    size_t count = arrlenu(storage->resources);

    Storage_Resource_t **current = storage->resources;
    for (size_t i = count; i; --i) {
        Storage_Resource_t *resource = *(current++);
        _release(resource);
    }

    arrfree(storage->resources);

    return count;
}
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
