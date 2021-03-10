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

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <libs/fs/fs.h>

#include <stdbool.h>
#include <stddef.h>

typedef enum _Storage_Resource_Types_t {
    STORAGE_RESOURCE_STRING,
    STORAGE_RESOURCE_BLOB,
    STORAGE_RESOURCE_IMAGE,
    Storage_Resource_Types_t_CountOf
} Storage_Resource_Types_t;

typedef struct _Storage_Resource_t {
    char *name; // Resources are references by a name, which can is (base-path) relative.
    Storage_Resource_Types_t type;
    union {
        struct {
            char *chars;
            size_t length;
        } string;
        struct {
            void *ptr;
            size_t size;
        } blob;
        struct {
            size_t width, height;
            void *pixels;
        } image;
    } var;
    double age;
    int references;
    bool allocated;
} Storage_Resource_t;

typedef struct _Storage_Configuration_t {
    const char *path;
} Storage_Configuration_t;


typedef struct _Storage_t {
    Storage_Configuration_t configuration;

    char base_path[PLATFORM_PATH_MAX];
    FS_Context_t *context;
    Storage_Resource_t **resources;
} Storage_t;

#define S_SCHARS(r)         ((r)->var.string.chars)
#define S_SLENTGH(r)        ((r)->var.string.length)
#define S_BPTR(r)           ((r)->var.blob.ptr)
#define S_BSIZE(r)          ((r)->var.blob.size)
#define S_IWIDTH(r)         ((r)->var.image.width)
#define S_IHEIGHT(r)        ((r)->var.image.height)
#define S_IPIXELS(r)        ((r)->var.image.pixels)

#define RESOURCE_IMAGE_ICON_ID      "icon"
#define RESOURCE_IMAGE_ICON_WB_ID   "icon-bw"
#define RESOURCE_BLOB_MAPPINGS_ID   "gamecontrollerdb"

extern Storage_t *Storage_create(const Storage_Configuration_t *configuration);
extern void Storage_destroy(Storage_t *storage);

extern const char *Storage_get_base_path(const Storage_t *storage);

extern bool Storage_exists(const Storage_t *storage, const char *name);

extern Storage_Resource_t *Storage_load(Storage_t *storage, const char *name, Storage_Resource_Types_t type);
extern bool Storage_store(Storage_t *storage, const char *name, const Storage_Resource_t *resource);

extern void Storage_lock(Storage_Resource_t *resource);
extern void Storage_unlock(Storage_Resource_t *resource);

extern FS_Handle_t *Storage_open(const Storage_t *storage, const char *name);

extern bool Storage_update(Storage_t *storage, float delta_time);

#endif  /* __STORAGE_H__ */
