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

#ifndef TOFU_SYSTEMS_STORAGE_H
#define TOFU_SYSTEMS_STORAGE_H

#include "storage/cache.h"

#include <core/config.h>
#include <core/platform.h>
#include <libs/fs/fs.h>
#include <libs/md5.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STORAGE_RESOURCE_ID_LENGTH  MD5_SIZE

typedef enum Storage_Resource_Types_e {
    STORAGE_RESOURCE_STRING,
    // STORAGE_RESOURCE_ENCODED,
    STORAGE_RESOURCE_BLOB,
    STORAGE_RESOURCE_IMAGE,
    Storage_Resource_Types_t_CountOf
} Storage_Resource_Types_t;

typedef struct Storage_Resource_s {
    uint8_t id[STORAGE_RESOURCE_ID_LENGTH];
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
#if defined(TOFU_STORAGE_AUTO_COLLECT)
    double age;
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
} Storage_Resource_t;

typedef struct Storage_Configuration_s {
    const char *kernal_path;
    const char *data_path;
} Storage_Configuration_t;

typedef struct Storage_s {
    Storage_Configuration_t configuration;

    struct {
        char user[PLATFORM_PATH_MAX]; // User-dependent folder, where the engine can save.
        // TODO: add the possibility to save to a shared folder, e.g. for saving hiscores.
        // char shared[PLATFORM_PATH_MAX];
        char local[PLATFORM_PATH_MAX]; // Identity-derived folder.
    } path;

    FS_Context_t *context;

    Storage_Cache_t *cache;

    Storage_Resource_t **resources;
} Storage_t;

#define S_SCHARS(r)         ((r)->var.string.chars)
#define S_SLENTGH(r)        ((r)->var.string.length)
#define S_BPTR(r)           ((r)->var.blob.ptr)
#define S_BSIZE(r)          ((r)->var.blob.size)
#define S_IWIDTH(r)         ((r)->var.image.width)
#define S_IHEIGHT(r)        ((r)->var.image.height)
#define S_IPIXELS(r)        ((r)->var.image.pixels)

extern Storage_t *Storage_create(const Storage_Configuration_t *configuration);
extern void Storage_destroy(Storage_t *storage);

// Resources are referenced by a name, which can is (base-path) relative.
extern bool Storage_inject_base64(Storage_t *storage, const char *name, const char *encoded_data, size_t length);
extern bool Storage_inject_ascii85(Storage_t *storage, const char *name, const char *encoded_data, size_t length);
extern bool Storage_inject_raw(Storage_t *storage, const char *name, const void *data, size_t size);

extern bool Storage_set_identity(Storage_t *storage, const char *identity);

extern Storage_Resource_t *Storage_load(Storage_t *storage, const char *name, Storage_Resource_Types_t type);
extern bool Storage_store(Storage_t *storage, const char *name, const Storage_Resource_t *resource);

extern FS_Handle_t *Storage_open(const Storage_t *storage, const char *name); // Use `FS` API to control and close it.

#if defined(TOFU_STORAGE_AUTO_COLLECT)
extern bool Storage_update(Storage_t *storage, float delta_time);
#else   /* TOFU_STORAGE_AUTO_COLLECT */
extern size_t Storage_flush(Storage_t *storage);
#endif  /* TOFU_STORAGE_AUTO_COLLECT */

#endif  /* TOFU_SYSTEMS_STORAGE_H */
