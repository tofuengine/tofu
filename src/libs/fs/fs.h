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

#include <core/platform.h>

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
  #define FILE_PATH_MAX           256
#elif PLATFORM_ID == PLATFORM_OSX
  #define FILE_PATH_SEPARATOR     '/'
  #define FILE_PATH_SEPARATOR_SZ  "/"
  #define FILE_PATH_CURRENT_SZ    "./"
  #define FILE_PATH_PARENT_SZ     "../"
  #define FILE_PATH_MAX           1024
#endif

#define FILE_SYSTEM_PATH_SEPARATOR    '/'
#define FILE_SYSTEM_PATH_SEPARATOR_SZ "/"

typedef struct _File_System_Callbacks_t {
   void * (*init)  (const char *path);
   void   (*deinit)(void *context);
   bool   (*exists)  (const void *context, const char *file);
   void * (*open) (const void *context, const char *file, size_t *size_in_bytes);
   size_t (*read) (void *handle, void *buffer, size_t bytes_requested);
   void   (*skip) (void *handle, int offset);
   bool   (*eof)  (void *handle);
   void   (*close)(void *handle);
} File_System_Callbacks_t;

typedef struct _File_System_Mount_t {
    const File_System_Callbacks_t *callbacks;
    void *context;
} File_System_Mount_t;

typedef struct _File_System_t {
    File_System_Mount_t *mount_points;
} File_System_t;

typedef enum _File_System_Chunk_Types_t {
    FILE_SYSTEM_CHUNK_NULL,
    FILE_SYSTEM_CHUNK_STRING,
    FILE_SYSTEM_CHUNK_BLOB,
    FILE_SYSTEM_CHUNK_IMAGE,
} File_System_Chunk_Types_t;

typedef struct _File_System_Chunk_t {
    File_System_Chunk_Types_t type;
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
} File_System_Chunk_t;

extern bool FS_initialize(File_System_t *file_system, const char *base_path);
extern void FS_terminate(File_System_t *file_system);

extern File_System_Chunk_t FS_load(const File_System_t *file_system, const char *file, File_System_Chunk_Types_t type);
extern void FS_release(File_System_Chunk_t chunk);

#endif /* __FS_H__ */
