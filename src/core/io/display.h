/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

// TODO: rename Display to Video?

#include <config.h>
#include <libs/gl/gl.h>
#include <libs/fs/fsaux.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdbool.h>
#include <stddef.h>

#include "display/program.h"

typedef enum _Display_Programs_t {
    Display_Programs_t_First = 0,
    DISPLAY_PROGRAM_PASSTHRU = Display_Programs_t_First,
    DISPLAY_PROGRAM_CUSTOM,
    Display_Programs_t_Last = DISPLAY_PROGRAM_CUSTOM,
    Display_Programs_t_CountOf
} Display_Programs_t;

typedef struct _Display_Configuration_t {
    const char *title;
    File_System_Chunk_t icon;
    size_t icon_size;
    size_t width, height, scale;
    bool fullscreen;
    bool vertical_sync;
    bool hide_cursor;
} Display_Configuration_t;

typedef struct _Display_t {
    Display_Configuration_t configuration;

    GLFWwindow *window;
    size_t window_width, window_height, window_scale;
    size_t physical_width, physical_height;

    GL_Color_t *vram; // Temporary buffer to create the OpenGL texture from `GL_Pixel_t` array.
    size_t vram_size;
    GLuint vram_texture;
    GL_Quad_t vram_destination;
    GL_Point_t vram_offset;

    Program_t programs[Display_Programs_t_CountOf];
    Program_t *active_program;
    GLfloat time;

    GL_Palette_t palette;
    GL_Context_t gl;
} Display_t;

extern bool Display_initialize(Display_t *display, const Display_Configuration_t *configuration);
extern void Display_terminate(Display_t *display);
extern bool Display_should_close(const Display_t *display);
extern void Display_update(Display_t *display, float delta_time);
extern void Display_clear(const Display_t *display);
extern void Display_offset(Display_t *display, GL_Point_t offset);
extern void Display_present(const Display_t *display);

extern void Display_shader(Display_t *display, const char *code);
extern void Display_palette(Display_t *display, const GL_Palette_t *palette);

#endif  /* __DISPLAY_H__ */