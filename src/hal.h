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
#include <stddef.h>
#include <stdint.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "config.h"

typedef struct _Point_t {
    double x, y;
} Point_t;

typedef struct _Rectangle_t {
    double x, y;
    double width, height;
} Rectangle_t;

typedef struct _Texture_t {
    GLuint id;
    size_t width, height;
} Texture_t;

typedef struct _Color_t {
    GLbyte r, g, b, a;
} Color_t;

typedef struct _Palette_t {
    Color_t colors[MAX_PALETTE_COLORS];
    size_t count;
} Palette_t;

typedef struct _Font_t {
    // char pathfile[PATH_FILE_MAX];
    bool loaded;
    Texture_t atlas;
} Font_t;

typedef struct _Bank_t { // TODO: rename to `Sheet`?
    // char pathfile[PATH_FILE_MAX];
    bool loaded;
    int cell_width, cell_height;
    Point_t origin;
    Texture_t atlas;
} Bank_t;

typedef void (*Texture_Callback_t)(void *parameters, void *data, int width, int height);

extern Texture_t load_texture(const char *pathfile, Texture_Callback_t callback, void *parameters);
extern void unload_texture(Texture_t *texture);
extern Bank_t load_bank(const char *pathfile, int cell_width, int cell_height, const Palette_t *palette);
extern void unload_bank(Bank_t *bank);
extern Font_t load_font(const char *pathfile);
extern void unload_font(Font_t *font);

#endif  /* __HAL_H__*/