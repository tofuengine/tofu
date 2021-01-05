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

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

// TODO: rename Display to Video?

#include <libs/gl/gl.h>

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
    GLFWimage icon;
    struct {
        const char *title;
        size_t width, height, scale;
    } window;
    bool fullscreen;
    bool vertical_sync;
    bool hide_cursor;
} Display_Configuration_t;

typedef struct _Display_t {
    Display_Configuration_t configuration;

    GLFWwindow *window;

    struct {
        GL_Size_t size;
        GL_Context_t *context;
        GL_Palette_t palette;
    } canvas;

    struct {
        GL_Color_t *pixels; // Temporary buffer to create the OpenGL texture from `GL_Pixel_t` array.
        size_t size;
        GLuint texture;
        GL_Rectangle_t rectangle;
        GL_Point_t offset;
    } vram;

    struct {
        Program_t array[Display_Programs_t_CountOf];
        Program_t *active;
    } program;

    double time;
} Display_t;

typedef struct _Display_VRAM_t {
    const void *pixels;
    size_t width, height;
} Display_VRAM_t;

extern Display_t *Display_create(const Display_Configuration_t *configuration); // TODO: rename to `Graphics`?
extern void Display_destroy(Display_t *display);

extern bool Display_should_close(const Display_t *display);
extern void Display_update(Display_t *display, float delta_time);
extern void Display_present(const Display_t *display);

extern void Display_set_palette(Display_t *display, const GL_Palette_t *palette);
extern void Display_set_offset(Display_t *display, GL_Point_t offset);
extern void Display_set_shader(Display_t *display, const char *code);

extern void Display_get_vram(const Display_t *display, Display_VRAM_t *vram);
extern GLFWwindow *Display_get_window(const Display_t *display);
extern float Display_get_scale(const Display_t *display);
extern GL_Context_t *Display_get_context(const Display_t *display);
extern const GL_Palette_t *Display_get_palette(const Display_t *display);
extern GL_Point_t Display_get_offset(const Display_t *display);

#endif  /* __DISPLAY_H__ */
