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
#include <stddef.h>
//#include <stdint.h>

#include "hal.h"
#include "core/program.h"

#define FRAMEBUFFERS_COUNT      2
#define SHADERS_COUNT           5

#define SHADER_INDEX_PALETTE    0

// Forward declaration.
typedef struct _Engine_Statistics_t Engine_Statistics_t;

typedef struct _Display_Configuration_t {
    int width, height;
    int colors;
    bool fullscreen;
    bool autofit;
    bool hide_cursor;
    bool exit_key_enabled;
} Display_Configuration_t;

typedef struct _Display_t {
    Display_Configuration_t configuration;

    GLFWwindow *window;
    int window_width, window_height, window_scale;

    Program_t program;

    GLuint offscreen;
    Rectangle_t offscreen_source, offscreen_destination;
    Point_t offscreen_origin;

    Palette_t palette;
} Display_t;

extern bool Display_initialize(Display_t *display, const Display_Configuration_t *configuration, const char *title);
extern bool Display_shouldClose(Display_t *display);
extern void Display_processInput(Display_t *display);
extern void Display_renderBegin(Display_t *display);
extern void Display_renderEnd(Display_t *display, double now, const Engine_Statistics_t *statistics);
extern void Display_palette(Display_t *display, const Palette_t *palette);
extern void Display_shader(Display_t *display, size_t index, const char *code);
extern void Display_terminate(Display_t *display);

#endif  /* __DISPLAY_H__ */