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

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdbool.h>

#include <raylib/raylib.h>

#include <stdlib.h>

#define MAX_GRAPHIC_BANKS       4

#define MAX_PALETTE_COLORS      16
#define VALUES_PER_COLOR        4

typedef struct _Display_Configuration_t {
    int width, height;
    int colors;
    bool fullscreen;
    bool autofit;
    bool hide_cursor;
    bool display_fps;
    bool exit_key_enabled;
} Display_Configuration_t;

typedef struct _Bank_t {
    // char file[PATH_FILE_MAX];
    Texture2D atlas;
    int cell_width, cell_height;
} Bank_t;

typedef struct _Display_t {
    Display_Configuration_t configuration;

    int window_width, window_height, window_scale;

    RenderTexture2D offscreen;
    Rectangle offscreen_source, offscreen_destination;
    Vector2 offscreen_origin;

    Color palette[MAX_PALETTE_COLORS];
    Shader palette_shader;
    int palette_shader_palette_location;

    Bank_t banks[MAX_GRAPHIC_BANKS];
} Display_t;

extern bool Display_initialize(Display_t *display, const Display_Configuration_t *configuration, const char *title);
extern bool Display_shouldClose(Display_t *display);
extern void Display_renderBegin(Display_t *display);
extern void Display_renderEnd(Display_t *display, const double fps, const double delta_time);
extern void Display_palette(Display_t *display, const Color *palette, size_t count);
extern void Display_terminate(Display_t *display);

#endif  /* __DISPLAY_H__ */