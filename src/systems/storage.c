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

#include "storage.h"

#include <core/config.h>
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>

#include <stdint.h>

// This defines how many seconds a resource persists in the cache after the initial load (or a reuse).
#define STORAGE_RESOURCE_AGE_LIMIT  30.0

typedef bool (*Storage_Load_Function_t)(Storage_Resource_t *resource, FS_Handle_t *handle);

#define LOG_CONTEXT "storage"

Storage_t *Storage_create(const Storage_Configuration_t *configuration)
{
    Storage_t *storage = malloc(sizeof(Storage_t));
    if (!storage) {
        LOG_E(LOG_CONTEXT, "can't allocate storage");
        return NULL;
    }

    *storage = (Storage_t){
            .configuration = *configuration,
            .path = {
                .user = { 0 },
                .local = { 0 }
            }
        };

    path_expand(PLATFORM_PATH_USER, storage->path.user); // Expand and resolve the user-dependent folder.
    LOG_D(LOG_CONTEXT, "user path is `%s`", storage->path.user);

    // This would be correct to do, since the local path is initialized, but it's also very pedant.
    // Storage_set_identity(storage, DEFAULT_IDENTITY);

    storage->context = FS_create();
    if (!storage->context) {
        LOG_E(LOG_CONTEXT, "can't create file-system context");
        goto error_free;
    }
    LOG_D(LOG_CONTEXT, "file-system context %p created", storage->context);

    storage->cache = Storage_Cache_create(storage->context);
    if (!storage->cache) {
        LOG_E(LOG_CONTEXT, "can't create cache");
        goto error_destroy_context;
    }
    LOG_D(LOG_CONTEXT, "cache %p created", storage->cache);

    bool kernal_attached = FS_attach_folder_or_archive(storage->context, configuration->kernal_path);
    if (!kernal_attached) {
        LOG_E(LOG_CONTEXT, "can't attach kernal folder/archive at `%s`", configuration->kernal_path);
        goto error_destroy_cache;
    }
    LOG_D(LOG_CONTEXT, "kernal folder/archive attached w/ path `%s`", configuration->kernal_path);

    bool attached = FS_attach_folder_or_archive(storage->context, configuration->data_path);
    if (!attached) {
        LOG_E(LOG_CONTEXT, "can't attach data folder/archive at `%s`", configuration->data_path);
        goto error_destroy_cache;
    }
    LOG_D(LOG_CONTEXT, "data folder/archive attached w/ path `%s`", configuration->data_path);

    return storage;

error_destroy_cache:
    Storage_Cache_destroy(storage->cache);
error_destroy_context:
    FS_destroy(storage->context);
error_free:
    free(storage);
    return NULL;
}

static void _release(Storage_Resource_t *resource)
{
    if (!resource->allocated) {
        LOG_D(LOG_CONTEXT, "resource %p wasn't allocated, won't release", resource);
    } else
    if (resource->type == STORAGE_RESOURCE_STRING) {
        free(resource->var.string.chars);
        LOG_D(LOG_CONTEXT, "resource-data `%s` at %p freed (%d characters string)",
            resource->name, resource->var.string.chars, resource->var.string.length);
    } else
    if (resource->type == STORAGE_RESOURCE_BLOB) {
        free(resource->var.blob.ptr);
        LOG_D(LOG_CONTEXT, "resource-data `%s` at %p freed (%d bytes blob)",
            resource->name, resource->var.blob.ptr, resource->var.blob.size);
    } else
    if (resource->type == STORAGE_RESOURCE_IMAGE) {
        stbi_image_free(resource->var.image.pixels);
        LOG_D(LOG_CONTEXT, "resource-data `%s` at %p freed (%dx%d image)",
            resource->name, resource->var.image.pixels, resource->var.image.width, resource->var.image.height);
    }
    free(resource->name);
    free(resource);
    LOG_D(LOG_CONTEXT, "resource %p freed", resource);
}

void Storage_destroy(Storage_t *storage)
{
    Storage_Resource_t **current = storage->resources;
    for (size_t count = arrlenu(storage->resources); count; --count) {
        Storage_Resource_t *resource = *(current++);
        _release(resource);
    }
    arrfree(storage->resources);
    LOG_D(LOG_CONTEXT, "storage cache emptied");

    Storage_Cache_destroy(storage->cache);
    LOG_D(LOG_CONTEXT, "storage cache destroyed");

    FS_destroy(storage->context);
    LOG_D(LOG_CONTEXT, "file-system context destroyed");

    free(storage);
    LOG_D(LOG_CONTEXT, "storage freed");
}

typedef struct Scan_Callback_Closure_s {
    Storage_Scan_Callback_t callback;
    void *user_data;
} Scan_Callback_Closure_t;

static void _scan_callback(void *user_data, const char *name)
{
    const Scan_Callback_Closure_t *closure = (const Scan_Callback_Closure_t *)user_data;

    closure->callback(closure->user_data, name);
}

void Storage_scan(const Storage_t *storage, Storage_Scan_Callback_t callback, void *user_data)
{
    FS_scan(storage->context, _scan_callback, &(Scan_Callback_Closure_t){ // The closure is pedantic, as the callback signatures match.
            .callback = callback,
            .user_data = user_data
        });
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
    //     LOG_D(LOG_CONTEXT, "user-dependent path `%s` detached", storage->path.local);
    // }

    path_join(storage->path.local, storage->path.user, identity); // Build the local storage-path, using identity.

    bool created = path_mkdirs(storage->path.local);
    if (!created) {
        LOG_E(LOG_CONTEXT, "can't create used-dependent path `%s`", storage->path.local);
        return false;
    }

    bool attached = FS_attach_folder(storage->context, storage->path.local);
    if (!attached) {
        LOG_E(LOG_CONTEXT, "can't attach user-dependent path path `%s`", storage->path.local);
        return false;
    }

    LOG_D(LOG_CONTEXT, "user-dependent path `%s` attached", storage->path.local);
    return true;
}

static void *_load(FS_Handle_t *handle, bool null_terminate, size_t *size)
{
    size_t bytes_requested = FS_size(handle);

    size_t bytes_to_allocate = bytes_requested + (null_terminate ? 1 : 0);
    void *data = malloc(sizeof(uint8_t) * bytes_to_allocate); // Add null terminator for the string.
    if (!data) {
        LOG_E(LOG_CONTEXT, "can't allocate %d bytes of memory", bytes_to_allocate);
        FS_close(handle);
        return NULL;
    }

    size_t bytes_read = FS_read(handle, data, bytes_requested);
    if (bytes_read < bytes_requested) {
        LOG_E(LOG_CONTEXT, "can't read %d bytes of data (%d available)", bytes_requested, bytes_read);
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
    LOG_D(LOG_CONTEXT, "loaded a %d characters long string", length);

    *resource = (Storage_Resource_t){
            .type = STORAGE_RESOURCE_STRING,
            .var = {
                .string = {
                    .chars = (char *)chars,
                    .length = length
                }
            },
            .age = 0.0,
            .allocated = true
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
    LOG_D(LOG_CONTEXT, "loaded %d bytes blob", size);

    *resource = (Storage_Resource_t){
            .type = STORAGE_RESOURCE_BLOB,
            .var = {
                .blob = {
                    .ptr = ptr,
                    .size = size
                }
            },
            .age = 0.0,
            .allocated = true
        };

    return true;
}

static int _stbi_io_read(void *user_data, char *data, int size)
{
    FS_Handle_t *handle = (FS_Handle_t *)user_data;
    return (int)FS_read(handle, data, (size_t)size);
}

static void _stbi_io_skip(void *user_data, int n)
{
    FS_Handle_t *handle = (FS_Handle_t *)user_data;
    FS_seek(handle, n, SEEK_CUR); // We are discaring the return value, yep. :|
}

static int _stbi_io_eof(void *user_data)
{
    FS_Handle_t *handle = (FS_Handle_t *)user_data;
    return FS_eof(handle) ? -1 : 0;
}

static const stbi_io_callbacks _stbi_io_callbacks = {
    _stbi_io_read,
    _stbi_io_skip,
    _stbi_io_eof,
};

static bool _load_as_image(Storage_Resource_t *resource, FS_Handle_t *handle)
{
    int width, height, components;
    void *pixels = stbi_load_from_callbacks(&_stbi_io_callbacks, handle, &width, &height, &components, STBI_rgb_alpha);
    if (!pixels) {
        LOG_E(LOG_CONTEXT, "can't decode surface from handle `%p` (%s)", handle, stbi_failure_reason());
        return false;
    }
    LOG_D(LOG_CONTEXT, "loaded %dx%d image", width, height);

    *resource = (Storage_Resource_t){
            .type = STORAGE_RESOURCE_IMAGE,
            .var = {
                .image = {
                    .width = (size_t)width,
                    .height = (size_t)height,
                    .pixels = pixels
                }
            },
            .age = 0.0,
            .allocated = true
        };

    return true;
}

static const Storage_Load_Function_t _load_functions[Storage_Resource_Types_t_CountOf] = {
    _load_as_string,
    _load_as_blob,
    _load_as_image
};

static int _resource_compare_by_name(const void *lhs, const void *rhs)
{
    const Storage_Resource_t **l = (const Storage_Resource_t **)lhs;
    const Storage_Resource_t **r = (const Storage_Resource_t **)rhs;
    return strcasecmp((*l)->name, (*r)->name);
}

#ifdef __STORAGE_CACHE_ENTRIES_LIMIT__
static int _resource_compare_by_age(const void *lhs, const void *rhs)
{
    const Storage_Resource_t **l = (const Storage_Resource_t **)lhs;
    const Storage_Resource_t **r = (const Storage_Resource_t **)rhs;
    const double age_delta = (*l)->age - (*r)->age;
    if (age_delta > 0.0) { // Sort by highest age.
        return -1;
    } else
    if (age_delta < 0.0) {
        return 1;
    } else {
        return 0;
    }
}
#endif

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

Storage_Resource_t *Storage_load(Storage_t *storage, const char *name, Storage_Resource_Types_t type)
{
#ifdef __STORAGE_CHECK_ABSOLUTE_PATHS__
    if (path_is_absolute(name) || !path_is_normalized(name)) {
        LOG_E(LOG_CONTEXT, "path `%s` is not allowed (only relative non-parent paths in sandbox mode)", name);
        return NULL;
    }
#endif  /* __STORAGE_CHECK_ABSOLUTE_PATHS__*/

    const Storage_Resource_t *needle = &(Storage_Resource_t){ .name = (char *)name };
    Storage_Resource_t **entry = bsearch((const void *)&needle, storage->resources, arrlenu(storage->resources), sizeof(Storage_Resource_t *), _resource_compare_by_name);
    if (entry) {
        LOG_D(LOG_CONTEXT, "cache-hit for resource `%s`, resetting age and returning", name);
        Storage_Resource_t *resource = *entry;
        resource->age = 0.0f; // Reset age on cache-hit.
        return resource;
    }

    Storage_Resource_t *resource = malloc(sizeof(Storage_Resource_t));
    if (!resource) {
        LOG_E(LOG_CONTEXT, "can't allocate resource");
        return NULL;
    }

    if (_resource_load(resource, name, type, storage->context)) {
        LOG_D(LOG_CONTEXT, "resource `%s` loaded from file-system", name);
    } else {
        LOG_E(LOG_CONTEXT, "can't load resource `%s`", name);
        goto error_free_resource;
    }
    resource->name = stb_memdup(name, strlen(name) + 1);

    arrpush(storage->resources, resource);
#ifdef __STORAGE_CACHE_ENTRIES_LIMIT__
    if (arrlenu(storage->resources) > __STORAGE_CACHE_ENTRIES_LIMIT__) {
        qsort(storage->resources, arrlenu(storage->resources), sizeof(Storage_Resource_t *), _resource_compare_by_age);
        storage->resources[0]->age = STORAGE_RESOURCE_AGE_LIMIT; // Mark the oldest for release in the next cycle.
        LOG_D(LOG_CONTEXT, "resource `%s` marked for release", storage->resources[0]->name);
    }
#endif

    qsort(storage->resources, arrlenu(storage->resources), sizeof(Storage_Resource_t *), _resource_compare_by_name); // Keep sorted to use binary-search.
    LOG_D(LOG_CONTEXT, "resource `%s` stored as %p, cache optimized", name, resource);

    return resource;

error_free_resource:
    free(resource);
    return NULL;
}

static void _stbi_write_func(void *context, void *data, int size)
{
    FILE *stream = (FILE *)context;
    size_t bytes_to_write = (size_t)size;
    size_t bytes_written = fwrite(data, sizeof(uint8_t), bytes_to_write, stream);
    if (bytes_written != bytes_to_write) {
        LOG_E(LOG_CONTEXT, "can't write %d byte(s) (%d written)", bytes_to_write, bytes_written);
    }
}

// This function saves a file into the local user-dependent storage. The mount points aren't modified.
bool Storage_store(Storage_t *storage, const char *name, const Storage_Resource_t *resource)
{
    char path[PLATFORM_PATH_MAX] = { 0 };
    path_join(path, storage->path.local, name);

    FILE *stream = fopen(path, "wb");
    if (!stream) {
        LOG_E(LOG_CONTEXT, "can't create file `%s`", path);
        return false;
    }

    bool result = false;

    switch (resource->type) {
        case STORAGE_RESOURCE_STRING: {
            size_t chars_to_write = S_SLENTGH(resource);
            size_t chars_written = fwrite(S_SCHARS(resource), sizeof(char), chars_to_write, stream);
            result = chars_written == chars_to_write;
            break;
        }
        case STORAGE_RESOURCE_BLOB: {
            size_t bytes_to_write = S_BSIZE(resource);
            size_t bytes_written = fwrite(S_BPTR(resource), sizeof(uint8_t), bytes_to_write, stream);
            result = bytes_written == bytes_to_write;
            break;
        }
        case STORAGE_RESOURCE_IMAGE: {
            int done = stbi_write_png_to_func(_stbi_write_func, (void *)stream, S_IWIDTH(resource), S_IHEIGHT(resource), 4, S_IPIXELS(resource), S_IWIDTH(resource) * 4);
            result = done != 0;
            break;
        }
        default: {
            break;
        }
    }

    fclose(stream);

    if (!result) {
        LOG_E(LOG_CONTEXT, "can't write resource `%s` w/ type %d to file `%s`", name, resource->type, path);
    }

    return result;
}

FS_Handle_t *Storage_open(const Storage_t *storage, const char *name)
{
    return FS_open(storage->context, name);
}

bool Storage_update(Storage_t *storage, float delta_time)
{
    // Backward scan, to remove to-be-released resources.
    for (int index = arrlen(storage->resources) - 1; index >= 0; --index) {
        Storage_Resource_t *resource = storage->resources[index];
        resource->age += delta_time;
        if (resource->age < STORAGE_RESOURCE_AGE_LIMIT) {
            continue;
        }
        _release(resource);
        arrdel(storage->resources, index); // No need to resort, removing preserve ordering.
    }
    return true;
}
