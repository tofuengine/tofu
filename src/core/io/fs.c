/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "fs.h"

#include <libs/stb.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum _File_System_Modes_t {
    FILE_SYSTEM_MODE_BINARY,
    FILE_SYSTEM_MODE_TEXT,
    File_System_Modes_t_CountOf
} File_System_Modes_t;

static void *load(const File_System_t *fs, const char *file, File_System_Modes_t mode, size_t *size)
{
    char full_path[PATH_FILE_MAX];
    strcpy(full_path, fs->base_path);
    strcat(full_path, file);

    FILE *stream = fopen(full_path, mode == FILE_SYSTEM_MODE_BINARY ? "rb" :"rt");
    if (!stream) {
        return NULL;
    }
    fseek(stream, 0L, SEEK_END);
    const size_t length = ftell(stream);
    void *data = malloc((length + (mode == FILE_SYSTEM_MODE_TEXT ? 1 : 0)) * sizeof(char)); // Add null terminator for the string.
    rewind(stream);
    size_t read_bytes = fread(data, sizeof(char), length, stream);
    fclose(stream);
    if (read_bytes < length) {
        free(data);
        return NULL;
    }
    if (mode == FILE_SYSTEM_MODE_TEXT) {
        ((char *)data)[length] = '\0';
    }
    *size = read_bytes;
    return data;
}

#if PLATFORM_ID == PLATFORM_WINDOWS
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <windows.h>
static char *realpath(const char *path, char resolved_path[PATH_MAX])
{
  char *return_path = 0;

  if (path) //Else EINVAL
  {
    if (resolved_path)
    {
      return_path = resolved_path;
    }
    else
    {
      //Non standard extension that glibc uses
      return_path = malloc(PATH_MAX); 
    }

    if (return_path) //Else EINVAL
    {
      //This is a Win32 API function similar to what realpath() is supposed to do
      size_t size = GetFullPathNameA(path, PATH_MAX, return_path, 0);

      //GetFullPathNameA() returns a size larger than buffer if buffer is too small
      if (size > PATH_MAX)
      {
        if (return_path != resolved_path) //Malloc'd buffer - Unstandard extension retry
        {
          size_t new_size;
          
          free(return_path);
          return_path = malloc(size);

          if (return_path)
          {
            new_size = GetFullPathNameA(path, size, return_path, 0); //Try again

            if (new_size > size) //If it's still too large, we have a problem, don't try again
            {
              free(return_path);
              return_path = 0;
              errno = ENAMETOOLONG;
            }
            else
            {
              size = new_size;
            }
          }
          else
          {
            //I wasn't sure what to return here, but the standard does say to return EINVAL
            //if resolved_path is null, and in this case we couldn't malloc large enough buffer
            errno = EINVAL;
          }  
        }
        else //resolved_path buffer isn't big enough
        {
          return_path = 0;
          errno = ENAMETOOLONG;
        }
      }

      //GetFullPathNameA() returns 0 if some path resolve problem occured
      if (!size) 
      {
        if (return_path != resolved_path) //Malloc'd buffer
        {
          free(return_path);
        }
        
        return_path = 0;

        //Convert MS errors into standard errors
        switch (GetLastError())
        {
          case ERROR_FILE_NOT_FOUND:
            errno = ENOENT;
            break;

          case ERROR_PATH_NOT_FOUND: case ERROR_INVALID_DRIVE:
            errno = ENOTDIR;
            break;

          case ERROR_ACCESS_DENIED:
            errno = EACCES;
            break;
          
          default: //Unknown Error
            errno = EIO;
            break;
        }
      }

      //If we get to here with a valid return_path, we're still doing good
      if (return_path)
      {
        struct stat stat_buffer;

        //Make sure path exists, stat() returns 0 on success
        if (stat(return_path, &stat_buffer)) 
        {
          if (return_path != resolved_path)
          {
            free(return_path);
          }
        
          return_path = 0;
          //stat() will set the correct errno for us
        }
        //else we succeeded!
      }
    }
    else
    {
      errno = EINVAL;
    }
  }
  else
  {
    errno = EINVAL;
  }
    
  return return_path;
}
#endif

// TODO: implement a generic resource-loader, for later use with bundled applications.
void FS_initialize(File_System_t *fs, const char *base_path)
{
    *fs = (File_System_t){ 0 };

    char resolved[PATH_FILE_MAX]; // Using local buffer to avoid un-tracked `malloc()` for the syscall.
    char *ptr = realpath(base_path ? base_path : FILE_PATH_CURRENT_SZ, resolved);
    if (!ptr) {
        return;
    }

    size_t length = strlen(resolved);
    if (resolved[length - 1] != '/') {
        strcat(resolved, FILE_PATH_SEPARATOR_SZ);
        length += 1;
    }

    fs->base_path = malloc((length + 1) * sizeof(char));
    strcpy(fs->base_path, resolved);
}

void FS_terminate(File_System_t *fs)
{
    free(fs->base_path);
}

char *FS_load_as_string(const File_System_t *fs, const char *file, size_t *size)
{
    return load(fs, file, FILE_SYSTEM_MODE_TEXT, size);
}

void *FS_load_as_binary(const File_System_t *fs, const char *file, size_t *size)
{
    return load(fs, file, FILE_SYSTEM_MODE_BINARY, size);
}
