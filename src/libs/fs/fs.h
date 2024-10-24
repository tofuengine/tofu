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

#ifndef TOFU_LIBS_FS_H
#define TOFU_LIBS_FS_H

#include <stdbool.h>
#include <stddef.h>

#define FS_PATH_SEPARATOR    '/'
#define FS_PATH_SEPARATOR_SZ "/"

#define FS_ARCHIVE_EXTENSION_SZ ".pak"

typedef struct FS_Mount_s FS_Mount_t;
typedef struct FS_Handle_s FS_Handle_t;

typedef struct FS_Callbacks_s {
    bool   (*contains)(void *user_data, const char *name);
    void * (*open)    (void *user_data, const char *name);
    void   (*close)   (void *stream);
    size_t (*size)    (void *stream);
    size_t (*read)    (void *stream, void *buffer, size_t bytes_requested);
    bool   (*seek)    (void *stream, long offset, int whence);
    long   (*tell)    (void *stream);
    bool   (*eof)     (void *stream);
} FS_Callbacks_t;

typedef struct FS_Context_s FS_Context_t;

extern FS_Context_t *FS_create(void);
extern void FS_destroy(FS_Context_t *context);

extern bool FS_attach_folder_or_archive(FS_Context_t *context, const char *path);
extern bool FS_attach_folder(FS_Context_t *context, const char *path);
extern bool FS_attach_archive(FS_Context_t *context, const char *path);
extern bool FS_attach_from_callbacks(FS_Context_t *context, FS_Callbacks_t callbacks, void *user_data);

extern bool FS_exists(const FS_Context_t *context, const char *name);
extern FS_Handle_t *FS_open(const FS_Context_t *context, const char *name);
extern void FS_close(FS_Handle_t *handle);
extern size_t FS_size(FS_Handle_t *handle);
extern size_t FS_read(FS_Handle_t *handle, void *buffer, size_t bytes_requested);
extern bool FS_seek(FS_Handle_t *handle, long offset, int whence);
extern long FS_tell(FS_Handle_t *handle);
extern bool FS_eof(FS_Handle_t *handle);

#endif /* TOFU_LIBS_FS_H */
