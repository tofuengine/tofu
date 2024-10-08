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

#ifndef TOFU_LIBS_FS_INTERNAL_H
#define TOFU_LIBS_FS_INTERNAL_H

#include "fs.h"

typedef struct Mount_VTable_s {
    void         (*dtor)    (FS_Mount_t *mount);
    bool         (*contains)(const FS_Mount_t *mount, const char *name);
    FS_Handle_t *(*open)    (const FS_Mount_t *mount, const char *name);
} Mount_VTable_t;

typedef struct Handle_VTable_s {
    void   (*dtor)(FS_Handle_t *handle);
    size_t (*size)(FS_Handle_t *handle);
    size_t (*read)(FS_Handle_t *handle, void *buffer, size_t bytes_requested);
    bool   (*seek)(FS_Handle_t *handle, long offset, int whence);
    long   (*tell)(FS_Handle_t *handle);
    bool   (*eof) (FS_Handle_t *handle);
} Handle_VTable_t;

struct FS_Mount_s {
    Mount_VTable_t vtable;
};

struct FS_Handle_s {
    Handle_VTable_t vtable;
};

#endif /* TOFU_LIBS_FS_INTERNAL_H */
