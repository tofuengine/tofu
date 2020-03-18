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

#ifndef __FS_AUX_H__
#define __FS_AUX_H__

#include "fs.h"

typedef enum _File_System_Resource_Types_t {
    FILE_SYSTEM_RESOURCE_NULL,
    FILE_SYSTEM_RESOURCE_STRING,
    FILE_SYSTEM_RESOURCE_BLOB,
    FILE_SYSTEM_RESOURCE_IMAGE,
} File_System_Resource_Types_t;

typedef struct _File_System_Resource_t { // TODO: add caching.
    File_System_Resource_Types_t type;
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
} File_System_Resource_t;

extern bool FSaux_exists(const File_System_t *file_system, const char *file);
extern File_System_Resource_t FSaux_load(const File_System_t *file_system, const char *file, File_System_Resource_Types_t type);
extern void FSaux_release(File_System_Resource_t resource);

#endif /* __FS_AUX_H__ */
