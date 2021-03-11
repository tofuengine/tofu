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

#include "path.h"

#include <libs/fs/fs.h>
#include <libs/log.h>
#include <platform.h>

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if PLATFORM_ID == PLATFORM_WINDOWS
  #define realpath(N,R) _fullpath((R),(N),PATH_MAX)
#endif

#define LOG_CONTEXT "path"

void path_expand(const char *path, char *expanded)
{
    char resolved[PLATFORM_PATH_MAX];
#if PLATFORM_ID == PLATFORM_LINUX
    if (path[0] == '~') {
        const char *home = getenv("HOME");
        strcpy(resolved, home);
        strcat(resolved, path + 1);
#elif PLATFORM_ID == PLATFORM_WINDOWS
    if (strncasecmp(folder, "%AppData%", 10) {
        const char *appdata = getenv("APPDATA");
        strcpy(resolved, appdata);
        strcat(resolved, path + 10);
#else
    #error "Platform is neither Linux nor Windows!"
#endif
    } else {
        strcpy(resolved, path);
    }

    (void)realpath(resolved, expanded);
}

bool path_exists(const char *path)
{
    return access(path, R_OK) != -1;
}

bool path_mkdirs(const char *path)
{
    char aux[PLATFORM_PATH_MAX];
    strcpy(aux, path);

    // FIXME: rewrite this mess!!!
    for (char *p = strchr(aux + 1, PLATFORM_PATH_SEPARATOR); p; p = strchr(p + 1, PLATFORM_PATH_SEPARATOR)) {
        *p = '\0';
        if (path_exists(aux)) {
            if (!path_is_folder(aux)) {
                return false;
            }
        } else
        if (mkdir(aux, 0755) == -1) {
            return false;
        }
        *p = PLATFORM_PATH_SEPARATOR;
    }
    if (path_exists(aux)) {
        if (!path_is_folder(aux)) {
            return false;
        }
    } else
    if (mkdir(aux, 0755) == -1) {
        return false;
    }
    return true;
}

bool path_is_folder(const char *path)
{
    struct stat path_stat;
    int result = stat(path, &path_stat);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get stats for file `%s`", path);
        return false;
    }

    if (!S_ISDIR(path_stat.st_mode)) {
        return false;
    }

    return true;
}

bool path_is_file(const char *path)
{
    struct stat path_stat;
    int result = stat(path, &path_stat);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get stats for file `%s`", path);
        return false;
    }

    if (!S_ISREG(path_stat.st_mode)) {
        return false;
    }

    return true;
}

void path_split(const char *path, char *folder, char *file)
{
    if (path_is_folder(path)) {
        if (folder) {
            strcpy(folder, path);
        }
        if (file) {
            file[0] = '\0';
        }
    } else {
        const char *separator = strrchr(path, PLATFORM_PATH_SEPARATOR);
        if (folder) {
            size_t length = separator - path + 1;
            strncpy(folder, path, length);
        }
        if (file) {
            strcpy(file, separator + 1);
        }
    }

    if (folder && folder[strlen(folder) - 1] == PLATFORM_PATH_SEPARATOR) {
        folder[strlen(folder) - 1] = '\0';
    }
}

void path_join(char *path, const char *folder, const char *file)
{
    strcpy(path, folder);
    strcat(path, PLATFORM_PATH_SEPARATOR_SZ);
    strcat(path, file);
    for (char *ptr = path; *ptr != '\0'; ++ptr) { // Replace virtual file-system separator `/` with the actual one.
        if (*ptr == FS_PATH_SEPARATOR) {
            *ptr = PLATFORM_PATH_SEPARATOR;
        }
    }
}
