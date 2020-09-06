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

#ifndef __FS_INTERNALS_H__
#define __FS_INTERNALS_H__

#include "fs.h"

typedef struct _Mount_VTable_t {
    void                  (*dtor)    (File_System_Mount_t *mount);
    bool                  (*contains)(File_System_Mount_t *mount, const char *file);
    File_System_Handle_t *(*open)    (File_System_Mount_t *mount, const char *file);
} Mount_VTable_t;

typedef struct _Handle_VTable_t {
    void   (*dtor)(File_System_Handle_t *handle);
    size_t (*size)(File_System_Handle_t *handle);
    size_t (*read)(File_System_Handle_t *handle, void *buffer, size_t bytes_requested);
    bool   (*seek)(File_System_Handle_t *handle, long offset, int whence);
    long   (*tell)(File_System_Handle_t *handle);
    bool   (*eof) (File_System_Handle_t *handle);
} Handle_VTable_t;

typedef struct _Mount_t {
    Mount_VTable_t vtable;
} Mount_t;

typedef struct _Handle_t {
    Handle_VTable_t vtable;
} Handle_t;

#endif /* __FS_INTERNALS_H__ */
