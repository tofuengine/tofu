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

#include "storage.h"

#include <config.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <stdint.h>

// This defines how many seconds a resource persists in the cache after the initial load (or a reuse).
#define STORAGE_RESOURCE_AGE_LIMIT  30.0

typedef Storage_Resource_t *(*Storage_Load_Function_t)(FS_Handle_t *handle);

#define LOG_CONTEXT "storage"

Storage_t *Storage_create(const Storage_Configuration_t *configuration)
{
    Storage_t *storage = malloc(sizeof(Storage_t));
    if (!storage) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate storage");
        return NULL;
    }

    *storage = (Storage_t){
            .configuration = *configuration
        };

    storage->context = FS_create(configuration->base_path);
    if (!storage->context) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create file-system context");
        free(storage);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "storage file-system context created at %p", storage->context);

    return storage;
}

static void _release(Storage_Resource_t *resource)
{
    if (resource->type == STORAGE_RESOURCE_STRING) {
        free(resource->var.string.chars);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "resource-data `%s` at %p freed (%d characters string)",
            resource->file, resource->var.string.chars, resource->var.string.length);
    } else
    if (resource->type == STORAGE_RESOURCE_BLOB) {
        free(resource->var.blob.ptr);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "resource-data `%s` at %p freed (%d bytes blob)",
            resource->file, resource->var.blob.ptr, resource->var.blob.size);
    } else
    if (resource->type == STORAGE_RESOURCE_IMAGE) {
        stbi_image_free(resource->var.image.pixels);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "resource-data `%s` at %p freed (%dx%d image)",
            resource->file, resource->var.image.pixels, resource->var.image.width, resource->var.image.height);
    }
    free(resource->file);
    free(resource);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "resource %p freed", resource);
}

void Storage_destroy(Storage_t *storage)
{
    Storage_Resource_t **current = storage->resources;
    for (size_t count = arrlen(storage->resources); count; --count) {
        Storage_Resource_t *resource = *(current++);
        _release(resource);
    }
    arrfree(storage->resources);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "storage cache emptied");

    FS_destroy(storage->context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "file-system context destroyed");

    free(storage);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "storage freed");
}

bool Storage_exists(const Storage_t *storage, const char *file)
{
    return FS_locate(storage->context, file);
}

static void *_load(FS_Handle_t *handle, bool null_terminate, size_t *size)
{
    size_t bytes_requested = FS_size(handle);

    size_t bytes_to_allocate = bytes_requested + (null_terminate ? 1 : 0);
    void *data = malloc(bytes_to_allocate * sizeof(uint8_t)); // Add null terminator for the string.
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d bytes of memory", bytes_to_allocate);
        FS_close(handle);
        return NULL;
    }

    size_t bytes_read = FS_read(handle, data, bytes_requested);
    if (bytes_read < bytes_requested) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read %d bytes of data (%d available)", bytes_requested, bytes_read);
        free(data);
        return NULL;
    }

    if (null_terminate) {
        ((char *)data)[bytes_read] = '\0';
    }

    *size = bytes_read;
    return data;
}

static Storage_Resource_t *_load_as_string(FS_Handle_t *handle)
{
    size_t length;
    void *chars = _load(handle, true, &length);
    if (!chars) {
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "loaded a %d characters long string", length);

    Storage_Resource_t *resource = malloc(sizeof(Storage_Resource_t));
    if (!resource) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "cant' allocate resource");
        free(chars);
        return NULL;
    }

    *resource = (Storage_Resource_t){
            .type = STORAGE_RESOURCE_STRING,
            .var = {
                .string = {
                    .chars = (char *)chars,
                    .length = length
                }
            },
            .age = 0.0
        };

    return resource;
}

static Storage_Resource_t *_load_as_blob(FS_Handle_t *handle)
{
    size_t size;
    void *ptr = _load(handle, false, &size);
    if (!ptr) {
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "loaded %d bytes blob", size);

    Storage_Resource_t *resource = malloc(sizeof(Storage_Resource_t));
    if (!resource) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "cant' allocate resource");
        free(ptr);
        return NULL;
    }

    *resource = (Storage_Resource_t){
            .type = STORAGE_RESOURCE_BLOB,
            .var = {
                .blob = {
                    .ptr = ptr,
                    .size = size
                }
            },
            .age = 0.0
        };

    return resource;
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

static Storage_Resource_t* _load_as_image(FS_Handle_t *handle)
{
    int width, height, components;
    void *pixels = stbi_load_from_callbacks(&_stbi_io_callbacks, handle, &width, &height, &components, STBI_rgb_alpha);
    if (!pixels) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't decode surface from handle `%p` (%s)", handle, stbi_failure_reason());
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "loaded %dx%d image", width, height);

    Storage_Resource_t *resource = malloc(sizeof(Storage_Resource_t));
    if (!resource) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "cant' allocate resource");
        stbi_image_free(pixels);
        return NULL;
    }

    *resource = (Storage_Resource_t){
            .type = STORAGE_RESOURCE_IMAGE,
            .var = {
                .image = {
                    .width = (size_t)width,
                    .height = (size_t)height,
                    .pixels = pixels
                }
            },
            .age = 0.0
        };

    return resource;
}

static int _resource_compare_by_name(const void *lhs, const void *rhs)
{
    const Storage_Resource_t **l = (const Storage_Resource_t **)lhs;
    const Storage_Resource_t **r = (const Storage_Resource_t **)rhs;
    return strcasecmp((*l)->file, (*r)->file);
}

#ifdef __STORAGE_CACHE_ENTRIES_LIMIT__
static int _resource_compare_by_age(const void *lhs, const void *rhs)
{
    const Storage_Resource_t **l = (const Storage_Resource_t **)lhs;
    const Storage_Resource_t **r = (const Storage_Resource_t **)rhs;
    const float delta = (*l)->age - (*r)->age;
    if (delta < 0.0f) {
        return -1;
    } else
    if (delta > 0.0f) {
        return 1;
    } else {
        return 0;
    }
}
#endif

static const Storage_Load_Function_t _load_functions[Storage_Resource_Types_t_CountOf] = {
    _load_as_string,
    _load_as_blob,
    _load_as_image
};

const Storage_Resource_t *Storage_load(Storage_t *storage, const char *file, Storage_Resource_Types_t type)
{
    const Storage_Resource_t *key = &(Storage_Resource_t){ .file = (char *)file };
    Storage_Resource_t **entry = bsearch((const void *)&key, storage->resources, arrlen(storage->resources), sizeof(Storage_Resource_t *), _resource_compare_by_name);
    if (entry) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "cache-hit for resource `%s`, resetting age and returning", file);
        Storage_Resource_t *resource = *entry;
        resource->age = 0.0;
        return resource;
    }

    FS_Handle_t *handle = FS_locate_and_open(storage->context, file);
    if (!handle) {
        return NULL;
    }

    Storage_Resource_t *resource = _load_functions[type](handle);

    FS_close(handle);

    if (!resource) {
        return NULL;
    }
    resource->file = memdup(file, strlen(file) + 1);

    arrpush(storage->resources, resource);
#ifdef __STORAGE_CACHE_ENTRIES_LIMIT__
    if (arrlen(storage->resources) > __STORAGE_CACHE_ENTRIES_LIMIT__) {
        qsort(storage->resources, arrlen(storage->resources), sizeof(Storage_Resource_t *), _resource_compare_by_age);
        arrlast(storage->resources)->age = STORAGE_RESOURCE_AGE_LIMIT; // Mark the oldest for release in the next cycle.
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "resource `%s` marked for release", storage->resources[0]->file);
    }
#endif

    qsort(storage->resources, arrlen(storage->resources), sizeof(Storage_Resource_t *), _resource_compare_by_name); // Keep sorted to use binary-search.
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "resource `%s` stored as %p, cache optimized", file, resource);

    return resource;
}

FS_Handle_t *Storage_open(const Storage_t *storage, const char *file)
{
    return FS_locate_and_open(storage->context, file);
}

bool Storage_update(Storage_t *storage, float delta_time)
{
    // Backward scan, to remove to-be-released resources.
    for (int index = (int)arrlen(storage->resources) - 1; index >= 0; --index) {
        Storage_Resource_t *resource = storage->resources[index];
        resource->age += delta_time;
        if (resource->age >= STORAGE_RESOURCE_AGE_LIMIT) {
            _release(resource);
            arrdel(storage->resources, index); // No need to resort, removing preserve ordering.
        }
    }
    return true;
}
