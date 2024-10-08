/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#include <core/platform.h>
#include <libs/fs/fs.h>
#define _LOG_TAG "path"
#include <libs/log.h>

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#if PLATFORM_ID == PLATFORM_WINDOWS
    #if defined(PATH_USE_OS_NATIVE_API)
        #include <shlobj.h>
    #endif
#endif

#if PLATFORM_ID == PLATFORM_WINDOWS
    #define realpath(N,R) _fullpath((R),(N),PLATFORM_PATH_MAX)
#endif

#if PLATFORM_ID == PLATFORM_LINUX
    #define createdir(P)  mkdir((P), 0755)
#elif PLATFORM_ID == PLATFORM_WINDOWS
    #define createdir(P)  mkdir((P))
#endif

static inline bool _path_is_trailed(const char *path)
{
    return path[strlen(path) - 1] == PLATFORM_PATH_SEPARATOR;
}

static inline void _path_chop(char *path)
{
    path[strlen(path) - 1] = '\0';
}

void path_expand(const char *path, char *expanded)
{
    char resolved[PLATFORM_PATH_MAX] = { 0 };
#if PLATFORM_ID == PLATFORM_LINUX
    if (path[0] == '~') {
        const char *home = getenv("HOME");
        strcpy(resolved, home);
        strcat(resolved, path + 1);
#elif PLATFORM_ID == PLATFORM_WINDOWS
    if (strncasecmp(path, "%AppData%", 9) == 0) {
    #if defined(PATH_USE_OS_NATIVE_API)
        char appdata[PLATFORM_PATH_MAX] = { 0 };
        SHGetFolderPathA(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, appdata);
    #else
        const char *appdata = getenv("APPDATA"); // https://pureinfotech.com/list-environment-variables-windows-10/
    #endif
        strcpy(resolved, appdata);
        strcat(resolved, path + 9);
#endif
    } else {
        strcpy(resolved, path);
    }

    char *ptr = realpath(resolved, expanded);
    if (!ptr) {
        LOG_E("can't resolve path `%s`", resolved);
        return;
    }

    if (_path_is_trailed(expanded)) {
        _path_chop(expanded);
    }
}

bool path_exists(const char *path)
{
    return access(path, R_OK) != -1;
}

bool path_mkdirs(const char *path)
{
    char aux[PLATFORM_PATH_MAX] = { 0 };
    const char *p = path;
    bool finished = false;
    while (!finished) {
        const char *e = strchr(p + 1, PLATFORM_PATH_SEPARATOR);
        if (e) {
            strncat(aux, p, e - p);
        } else {
            strcat(aux, p);
            finished = true;
        }
        if (path_exists(aux)) { // Incremental path never ends with the separator (`/` is skipped on Linux).
            if (!path_is_folder(aux)) {
                return false;
            }
        } else
        if (createdir(aux) == -1) {
            return false;
        }
        p = e; // Don't skip the separator, we need it!
    }
    return true;
}

#if PLATFORM_ID == PLATFORM_WINDOWS
static inline bool _path_is_root(const char *path)
{
    return strlen(path) >= 2 && strlen(path) <= 3 && path[1] == ':'; // e.g. `C:` or `C:\`)
}
#endif

static int _path_stat(const char *pathname, struct stat *statbuf)
{
    char path[PLATFORM_PATH_MAX] = { 0 };
    strcpy(path, pathname);
#if PLATFORM_ID == PLATFORM_WINDOWS
    bool is_root = _path_is_root(path);
    bool is_trailed = _path_is_trailed(path);
    if (is_root && !is_trailed) { // On Windows, calling `stat()` on drives' root requires a trailing separator...
        strcat(path, PLATFORM_PATH_SEPARATOR_SZ);
    } else
    if (is_trailed) { // ... and no separator on every other path.
        _path_chop(path);
    }
#endif
    return stat(path, statbuf);
}

bool path_is_folder(const char *path)
{
    struct stat path_stat;
    int result = _path_stat(path, &path_stat);
    if (result != 0) {
        LOG_E("can't get stats for file `%s`", path);
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
    int result = _path_stat(path, &path_stat);
    if (result != 0) {
        LOG_E("can't get stats for file `%s`", path);
        return false;
    }

    if (!S_ISREG(path_stat.st_mode)) {
        return false;
    }

    return true;
}

bool path_is_absolute(const char *path)
{
#if PLATFORM_ID == PLATFORM_WINDOWS
    return strlen(path) >= 3 && path[1] == ':' && path[2] == PLATFORM_PATH_SEPARATOR; // e.g. `C:\`
#else
    return strlen(path) >= 1 && path[0] == PLATFORM_PATH_SEPARATOR;
#endif
}

static inline bool _starts_with(const char *string, const char *prefix)
{
    return strncmp(string, prefix, strlen(prefix)) == 0;
}

bool path_is_normalized(const char *path) // I.e. it doesn't contains current/relative path parts to "rewrite" the path specifier.
{
    return !_starts_with(path, PLATFORM_PATH_CURRENT_SZ)
        && !_starts_with(path, PLATFORM_PATH_PARENT_SZ)
        && !strstr(path, PLATFORM_PATH_SEPARATOR_SZ "." PLATFORM_PATH_SEPARATOR_SZ)
        && !strstr(path, PLATFORM_PATH_SEPARATOR_SZ ".." PLATFORM_PATH_SEPARATOR_SZ);
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
            folder[length] = '\0';
        }
        if (file) {
            strcpy(file, separator + 1);
        }
    }

    if (folder && _path_is_trailed(folder)) {
        _path_chop(folder);
    }
}

void path_join(char *path, const char *folder, const char *file)
{
    strcpy(path, folder);
    if (!_path_is_trailed(path)) {
        strcat(path, PLATFORM_PATH_SEPARATOR_SZ);
    }
    strcat(path, file);
    for (char *ptr = path; *ptr != '\0'; ++ptr) { // Replace virtual file-system separator `/` with the actual one.
        if (*ptr == FS_PATH_SEPARATOR) {
            *ptr = PLATFORM_PATH_SEPARATOR;
        }
    }
}

const char *path_lua_to_fs(char *fqn, const char *module_name)
{
    strcpy(fqn, "@"); // Prepend a `@`, required by Lua to track files.
    strcat(fqn, module_name);
    for (char *ptr = fqn; *ptr != '\0'; ++ptr) { // Replace `.` with `/` to map (virtual) file system entry.
        if (*ptr == '.') {
            *ptr = FS_PATH_SEPARATOR;
        }
    }
    strcat(fqn, ".lua");
    return fqn + 1; // Skip the `@` character and return the actual path.
}
