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

#ifndef __HAL_H__
#define __HAL_H__

#include <stdbool.h>
#include <stdint.h>

#include <raylib/raylib.h>

typedef struct _Font_t {
    // char pathfile[PATH_FILE_MAX];
    bool loaded;
    bool is_default;
    Font font;
} Font_t;

typedef struct _Bank_t { // TODO: rename to `Sheet`?
    // char pathfile[PATH_FILE_MAX];
    bool loaded;
    int cell_width, cell_height;
    Texture2D atlas;
} Bank_t;

typedef struct _Map_t {
    // char pathfile[PATH_FILE_MAX];
    bool loaded;
    int width, height;
    Bank_t bank;
    uint16_t *cells; // Only the lowest 8 bits are used to access the bank?
} Map_t;

extern Bank_t load_bank(const char *pathfile, int cell_width, int cell_height, const Color *palette, int colors);
extern void unload_bank(Bank_t *bank);
extern Font_t load_font(const char *pathfile);
extern void unload_font(Font_t *font);
extern Map_t load_map(const char *pathfile);
extern void unload_map(Map_t *map);

#endif  /* __HAL_H__*/