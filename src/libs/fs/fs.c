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
#include "pak.h"
#include "std.h"

#include <dirent.h>

struct _FS_Context_t {
    FS_Mount_t **mounts;
};

#define LOG_CONTEXT "fs"

static inline FS_Mount_t *_mount(const char *path)
{
    if (FS_std_is_valid(path)) {
        return FS_std_mount(path);
    } else
    if (FS_pak_is_valid(path)) {
        return FS_pak_mount(path);
    } else {
        return NULL;
    }
}

static inline void _unmount(FS_Mount_t *mount)
{
    mount->vtable.dtor(mount);
    free(mount);
}

static bool _attach(FS_Context_t *context, const char *path)
{
    FS_Mount_t *mount = _mount(path); // Path need to be already resolved.
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't attach mount-point `%s`", path);
        return false;
    }

    arrpush(context->mounts, mount);

    return true;
}

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

FS_Context_t *FS_create(const char *path)
{
    FS_Context_t *context = malloc(sizeof(FS_Context_t));
    if (!context) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate context");
        return NULL;
    }

    *context = (FS_Context_t){ 0 };

    DIR *dp = opendir(path);
    if (dp) { // Path is a folder, scan and mount all valid archives.
        for (struct dirent *entry = readdir(dp); entry; entry = readdir(dp)) {
            char subpath[PLATFORM_PATH_MAX] = { 0 };
            path_join(subpath, path, entry->d_name);
#ifdef __FS_ENFORCE_ARCHIVE_EXTENSION__
            if (!_ends_with(subpath, FS_ARCHIVE_EXTENSION)) {
                continue;
            }
#endif  /* __FS_ENFORCE_ARCHIVE_EXTENSION__ */
            if (!FS_pak_is_valid(subpath)) {
                continue;
            }

            _attach(context, subpath);
        }

        closedir(dp);
    }

    _attach(context, path); // Mount the resolved folder, as well (overriding archives).

    return context;
}

void FS_destroy(FS_Context_t *context)
{
    FS_Mount_t **current = context->mounts;
    for (size_t count = arrlen(context->mounts); count; --count) {
        FS_Mount_t *mount = *(current++);
        _unmount(mount);
    }

    arrfree(context->mounts);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context mount(s) freed");

    free(context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context freed");
}

bool FS_attach(FS_Context_t *context, const char *path)
{
    return _attach(context, path);
}

FS_Handle_t *FS_locate_and_open(const FS_Context_t *context, const char *name)
{
    const FS_Mount_t *mount = FS_locate(context, name);
    if (!mount) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't locate file `%s`", name);
        return NULL;
    }

    return FS_open(mount, name);
}

const FS_Mount_t *FS_locate(const FS_Context_t *context, const char *name)
{
#ifdef __FS_SUPPORT_MOUNT_OVERRIDE__
    // Backward scan, later mounts gain priority over existing ones.
    for (int index = (int)arrlen(context->mounts) - 1; index >= 0; --index) {
        const FS_Mount_t *mount = context->mounts[index];
#else
    FS_Mount_t **current = context->mounts;
    for (size_t count = arrlen(context->mounts); count; --count) {
        const FS_Mount_t *mount = *(current++);
#endif
        if (mount->vtable.contains(mount, name)) {
            return mount;
        }
    }

    return NULL;
}

FS_Handle_t *FS_open(const FS_Mount_t *mount, const char *name)
{
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
