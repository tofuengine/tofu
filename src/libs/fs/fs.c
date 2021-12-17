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

#include "fs.h"

#include <config.h>
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>

#include "internals.h"
#include "cache.h"
#include "pak.h"
#include "std.h"

struct FS_Context_s {
    FS_Mount_t **mounts;
};

#define LOG_CONTEXT "fs"

#ifdef __FS_ENFORCE_ARCHIVE_EXTENSION__
static inline bool _ends_with(const char *string, const char *suffix)
{
    size_t string_length = strlen(string);
    size_t suffix_length = strlen(suffix);
    if (string_length < suffix_length) {
        return false;
    }
    return strcasecmp(string + string_length - suffix_length, suffix) == 0;
}
#endif  /* __FS_ENFORCE_ARCHIVE_EXTENSION__ */

FS_Context_t *FS_create(void)
{
    FS_Context_t *context = malloc(sizeof(FS_Context_t));
    if (!context) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate context");
        return NULL;
    }

    *context = (FS_Context_t){ 0 };

    return context;
}

void FS_destroy(FS_Context_t *context)
{
    FS_Mount_t **current = context->mounts;
    for (size_t count = arrlenu(context->mounts); count; --count) {
        FS_Mount_t *mount = *(current++);
        mount->vtable.dtor(mount);
        free(mount);
    }

    arrfree(context->mounts);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context mount(s) freed");

    free(context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context freed");
}

bool FS_attach_folder_or_archive(FS_Context_t *context, const char *path)
{
    if (FS_std_is_valid(path)) {
        return FS_attach_folder(context, path);
    } else 
    if (FS_pak_is_valid(path)) {
        return FS_attach_archive(context, path);
    } else {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "path `%s` is neither a folder nor an archive", path);
        return false;
    }
}

bool FS_attach_folder(FS_Context_t *context, const char *path)
{
    if (!FS_std_is_valid(path)) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "path `%s` is not a folder", path);
        return false;
    }

    FS_Mount_t *mount = FS_std_mount(path); // Path need to be already resolved.
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't attach archive `%s`", path);
        return false;
    }

    arrpush(context->mounts, mount);

    return true;
}

bool FS_attach_archive(FS_Context_t *context, const char *path)
{
    if (!FS_pak_is_valid(path)) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "path `%s` is not an archive", path);
        return false;
    }

    FS_Mount_t *mount = FS_pak_mount(path); // Path need to be already resolved.
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't attach archive `%s`", path);
        return false;
    }

    arrpush(context->mounts, mount);

    return true;
}

bool FS_attach_cache(FS_Context_t *context, FS_Cache_Callbacks_t callbacks, void *user_data)
{
    FS_Mount_t *mount = FS_cache_mount(callbacks, user_data);
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't attach cache w/ user-data `%p`", user_data);
        return false;
    }

    arrpush(context->mounts, mount);

    return true;
}

FS_Handle_t *FS_open(const FS_Context_t *context, const char *name)
{
    const FS_Mount_t *mount = NULL;
#ifdef __FS_SUPPORT_MOUNT_OVERRIDE__
    // Backward scan, later mounts gain priority over existing ones.
    for (int index = arrlen(context->mounts) - 1; index >= 0; --index) {
#else
    for (int index = 0; index < arrlen(context->mounts); ++index) {
#endif
        FS_Mount_t *current = context->mounts[index];
        if (current->vtable.contains(current, name)) {
            mount = current;
            break;
        }
    }

    if (!mount) {
        return NULL;
    }

    return mount->vtable.open(mount, name);
}

void FS_close(FS_Handle_t *handle)
{
    handle->vtable.dtor(handle);
    free(handle);
}

size_t FS_size(FS_Handle_t *handle)
{
    return handle->vtable.size(handle);
}

size_t FS_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested)
{
    return handle->vtable.read(handle, buffer, bytes_requested);
}

bool FS_seek(FS_Handle_t *handle, long offset, int whence)
{
    return handle->vtable.seek(handle, offset, whence);
}

long FS_tell(FS_Handle_t *handle)
{
    return handle->vtable.tell(handle);
}

bool FS_eof(FS_Handle_t *handle)
{
    return handle->vtable.eof(handle);
}
