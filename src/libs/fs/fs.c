/*
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

#include "fs.h"

#include "callbacks.h"
#include "internal.h"
#include "pak.h"
#include "std.h"

#include <core/config.h>
#define _LOG_TAG "fs"
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>

// Basically a key/value pair to store along with the `mount` pointer an
// identifier. This is crucial to enable opaque handling of the mount-point
// in the API, most notably to attach/detach it.
typedef struct _mount_point_s {
    int id;
    FS_Mount_t *mount;
    int priority;
} _mount_point_t;

struct FS_Context_s {
    _mount_point_t *mount_points;
};

FS_Context_t *FS_create(void)
{
    FS_Context_t *context = malloc(sizeof(FS_Context_t));
    if (!context) {
        LOG_E("can't allocate context");
        return NULL;
    }

    *context = (FS_Context_t){ 0 };

    LOG_D("context %p allocated", context);

    return context;
}

void FS_destroy(FS_Context_t *context)
{
    LOG_D("destroying context %p", context);

    LOG_D("freeing %d mounts for context %p", arrlenu(context->mount_points), context);
    const _mount_point_t *current = context->mount_points;
    for (size_t count = arrlenu(context->mount_points); count; --count) {
        _mount_point_t mount_point = *(current++);

        FS_Mount_t *mount = mount_point.mount;
        mount->vtable.dtor(mount);
        free(mount);
    }
    arrfree(context->mount_points);
    LOG_D("context mount-points(s) freed");

    free(context);
    LOG_D("context freed");
}

bool FS_attach_folder_or_archive(FS_Context_t *context, const char *path, int priority, int *mount_id)
{
    if (FS_std_is_valid(path)) {
        return FS_attach_folder(context, path, priority, mount_id);
    } else 
    if (FS_pak_is_valid(path)) {
        return FS_attach_archive(context, path, priority, mount_id);
    } else {
        LOG_E("path `%s` is neither a folder nor an archive", path);
        return false;
    }
}

// The mount-point id is a monotonically increasing integer.
static int _last_mount_id = 0;

// Used to compare and sort in increasing order of priority OR identifier.
//
// This means that mount-points with the same priority will be preserved in
// "attach" order.
//
// The `File` sub-system supports multiple mount-points. When scanning for a
// file, the file instance present in "highest priority mount" is used. We call
// this "mount-override" as it enables a file to be present in more than an
// archive/folder, with only one instance to be used.
//
// In the context of the game-engine, it means that a file in the `data`
// archive/folder can have the same name of a `kernal` counterpart *and*
// override/redefine its implementation.
static int _compare_mount_points(const void *first, const void *second)
{
    // Note: we *invert* the the comparison terms so that a *reversed* ordering
    //       is applied. This ensures that a linear first-to-last scan of the
    //       mount-points array will correctly find higher priority (overriden)
    //       ones first!
    const _mount_point_t *first_mount_point = second;
    const _mount_point_t *second_mount_point = first;

    int delta = first_mount_point->priority - second_mount_point->priority;
    if (delta == 0) {
        delta = first_mount_point->id - second_mount_point->id;
    }
    return delta;
}

static inline int _push_mount(FS_Context_t *context, FS_Mount_t *mount, int priority)
{
    _mount_point_t mount_point = (_mount_point_t){
            .id = ++_last_mount_id,
            .mount = mount,
            .priority = priority
        };
    arrpush(context->mount_points, mount_point);

    qsort(context->mount_points, arrlenu(context->mount_points), sizeof(_mount_point_t), _compare_mount_points);

    return mount_point.id;
}

bool FS_attach_folder(FS_Context_t *context, const char *path, int priority, int *mount_id)
{
    if (!FS_std_is_valid(path)) {
        LOG_D("path `%s` is not a folder", path);
        return false;
    }

    FS_Mount_t *mount = FS_std_mount(path); // Path need to be already resolved.
    if (!mount) {
        LOG_E("can't attach archive `%s`", path);
        return false;
    }

    *mount_id = _push_mount(context, mount, priority);

    return true;
}

bool FS_attach_archive(FS_Context_t *context, const char *path, int priority, int *mount_id)
{
    if (!FS_pak_is_valid(path)) {
        LOG_D("path `%s` is not an archive", path);
        return false;
    }

    FS_Mount_t *mount = FS_pak_mount(path); // Path need to be already resolved.
    if (!mount) {
        LOG_E("can't attach archive `%s`", path);
        return false;
    }

    *mount_id = _push_mount(context, mount, priority);

    return true;
}

bool FS_attach_from_callbacks(FS_Context_t *context, const FS_Callbacks_t *callbacks, void *user_data, int priority, int *mount_id)
{
    FS_Mount_t *mount = FS_callbacks_mount(callbacks, user_data);
    if (!mount) {
        LOG_E("can't attach cache w/ user-data `%p`", user_data);
        return false;
    }

    *mount_id = _push_mount(context, mount, priority);

    return true;
}

bool FS_detach(FS_Context_t *context, int mount_id)
{
    LOG_D("detaching mount w/ id #%d from context %p", mount_id, context);

    for (size_t i = 0; i < arrlenu(context->mount_points); ++i) {
        _mount_point_t mount_point = context->mount_points[i];

        if (mount_point.id == mount_id) {
            LOG_D("mount w/ id #%d found in context %p", mount_id, context);

            FS_Mount_t *mount = mount_point.mount;
            mount->vtable.dtor(mount);
            free(mount);
            LOG_D("mount w/ id #%d released", mount_id);

            arrdel(context->mount_points, i);
            LOG_D("mount w/ id #%d detached", mount_id);

            return true;
        }
    }

    LOG_E("mount w/ id #%d not found in context %p", mount_id, context);
    return false;
}

static const FS_Mount_t *_locate(const FS_Context_t *context, const char *name)
{
    const _mount_point_t *current = context->mount_points;
    for (size_t count = arrlenu(context->mount_points); count; --count) {
        _mount_point_t mount_point = *(current++);

        const FS_Mount_t *mount = mount_point.mount;
        if (mount->vtable.contains(mount, name)) {
            return mount;
        }
    }
    return NULL;
}

bool FS_exists(const FS_Context_t *context, const char *name)
{
    return _locate(context, name) != NULL;
}

FS_Handle_t *FS_open(const FS_Context_t *context, const char *name)
{
    const FS_Mount_t *mount = _locate(context, name);

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

size_t FS_size(const FS_Handle_t *handle)
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

long FS_tell(const FS_Handle_t *handle)
{
    return handle->vtable.tell(handle);
}

bool FS_eof(const FS_Handle_t *handle)
{
    return handle->vtable.eof(handle);
}

size_t FS_gets(FS_Handle_t *handle, char *buffer, size_t buffer_size)
{
    if (buffer_size == 0) {
        return 0;
    }

    size_t total_read = 0;
    while (total_read < buffer_size - 1) { // Leave space for the null-terminator.
        char ch;
        size_t bytes_read = FS_read(handle, &ch, sizeof(char)); // Cast away constness for API compatibility.
        if (bytes_read == 0) {
            break; // EOF or error.
        }

        buffer[total_read++] = ch;

        if (ch == '\n') {
            break; // Newline encountered.
        }
    }

    buffer[total_read] = '\0'; // Null-terminate the string.

    return total_read;
}
