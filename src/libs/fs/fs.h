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

#ifndef __FS_H__
#define __FS_H__

#include <platform.h>

#include <stdbool.h>
#include <stddef.h>

#if PLATFORM_ID == PLATFORM_LINUX
  #include <linux/limits.h>

  #define FILE_PATH_SEPARATOR     '/'
  #define FILE_PATH_SEPARATOR_SZ  "/"
  #define FILE_PATH_CURRENT_SZ    "./"
  #define FILE_PATH_PARENT_SZ     "../"
  #define FILE_PATH_MAX           PATH_MAX
#elif PLATFORM_ID == PLATFORM_WINDOWS
  #define FILE_PATH_SEPARATOR     '\\'
  #define FILE_PATH_SEPARATOR_SZ  "\\"
  #define FILE_PATH_CURRENT_SZ    ".\\"
  #define FILE_PATH_PARENT_SZ     "..\\"
  #define FILE_PATH_MAX           260
#elif PLATFORM_ID == PLATFORM_OSX
  #define FILE_PATH_SEPARATOR     '/'
  #define FILE_PATH_SEPARATOR_SZ  "/"
  #define FILE_PATH_CURRENT_SZ    "./"
  #define FILE_PATH_PARENT_SZ     "../"
  #define FILE_PATH_MAX           1024
#endif

#define FS_PATH_SEPARATOR       '/'
#define FS_PATH_SEPARATOR_SZ    "/"

typedef struct _FS_Mount_t FS_Mount_t;
typedef struct _FS_Handle_t FS_Handle_t;

typedef struct _FS_Context_t FS_Context_t;

extern FS_Context_t *FS_create(const char *base_path);
extern void FS_destroy(FS_Context_t *context);

extern bool FS_attach(FS_Context_t *context, const char *path);
extern const FS_Mount_t *FS_locate(const FS_Context_t *context, const char *file);

extern FS_Handle_t *FS_locate_and_open(const FS_Context_t *context, const char *file);

extern FS_Handle_t *FS_open(const FS_Mount_t *mount, const char *file);
extern void FS_close(FS_Handle_t *handle);
extern size_t FS_size(FS_Handle_t *handle);
extern size_t FS_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested);
extern bool FS_seek(FS_Handle_t *handle, long offset, int whence);
extern long FS_tell(FS_Handle_t *handle);
extern bool FS_eof(FS_Handle_t *handle);

#endif /* __FS_H__ */
