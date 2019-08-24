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

#include "config.h"

#include <stdbool.h>
#include <stddef.h>

#include "hal.h"
#include "gl/gl.h"

typedef enum _Display_Keys_t {
    Display_Keys_t_First = 0,
    DISPLAY_KEY_UP = Display_Keys_t_First,
    DISPLAY_KEY_DOWN,
    DISPLAY_KEY_LEFT,
    DISPLAY_KEY_RIGHT,
    DISPLAY_KEY_Y,
    DISPLAY_KEY_X,
    DISPLAY_KEY_B,
    DISPLAY_KEY_A,
    DISPLAY_KEY_SELECT,
    DISPLAY_KEY_START,
    Display_Keys_t_Last = DISPLAY_KEY_START,
    Display_Keys_t_CountOf
} Display_Keys_t;

typedef enum _Display_Programs_t {
    Display_Programs_t_First = 0,
    DISPLAY_PROGRAM_PALETTE = Display_Programs_t_First,
    DISPLAY_PROGRAM_PASSTHRU,
    DISPLAY_PROGRAM_CUSTOM,
    Display_Programs_t_Last = DISPLAY_PROGRAM_CUSTOM,
    Display_Programs_t_CountOf
} Display_Programs_t;

typedef struct _Display_Configuration_t {
    int width, height;
    bool fullscreen;
#ifndef __NO_AUTOFIT__
    bool autofit;
#endif
    bool hide_cursor;
    bool exit_key_enabled;
} Display_Configuration_t;

typedef struct _Display_Key_State_t { // TODO: use explicit masks?
    uint8_t down : 1;
    uint8_t pressed : 1;
    uint8_t released : 1;
    uint8_t : 5;
} Display_Key_State_t;

typedef struct _Display_t {
    Display_Configuration_t configuration;

    Display_Key_State_t keys_state[Display_Keys_t_CountOf];

    GLFWwindow *window;
    int window_width, window_height, window_scale;
#ifndef __NO_AUTOFIT__
    int physical_width, physical_height;
    GL_Quad_t offscreen_source;
    GL_Quad_t offscreen_destination;
    GL_Texture_t offscreen_texture;
    GLuint offscreen_framebuffer;
#endif

    GL_Program_t programs[Display_Programs_t_CountOf];
    size_t program_index;

    GL_Palette_t palette;
    int background_index;
    GLfloat background_rgba[4];
} Display_t;

extern bool Display_initialize(Display_t *display, const Display_Configuration_t *configuration, const char *title);
extern bool Display_shouldClose(Display_t *display);
extern void Display_processInput(Display_t *display);
extern void Display_renderBegin(Display_t *display);
extern void Display_renderEnd(Display_t *display);
extern void Display_palette(Display_t *display, const GL_Palette_t *palette);
extern void Display_background(Display_t *display, const size_t color);
extern void Display_shader(Display_t *display, const char *code);extern void Display_screen_of_death(Display_t *display);
extern void Display_terminate(Display_t *display);

#endif  /* __DISPLAY_H__ */