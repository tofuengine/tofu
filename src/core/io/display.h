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

#include <config.h>
#include <libs/gl/gl.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#ifdef __GRAPHICS_CAPTURE_SUPPORT__
  #include <gif-h/gif.h>
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */

#include <stdbool.h>
#include <stddef.h>

#include "display/program.h"

typedef struct _Display_Configuration_t {
    GLFWimage icon;
    struct {
        const char *title;
        size_t width, height, scale;
    } window;
    bool fullscreen;
    bool vertical_sync;
    bool hide_cursor;
    const char *effect;
} Display_Configuration_t;

typedef struct _Display_t {
    Display_Configuration_t configuration;

    GLFWwindow *window;

    Program_t program;

    struct {
        GL_Size_t size;
        GL_Context_t *context;
        GL_CopperList_t *copperlist;
    } canvas;

    struct {
        GLuint texture;
        GL_Color_t *pixels; // Temporary buffer to create the OpenGL texture from `GL_Pixel_t` array.
        GL_Rectangle_t rectangle; // Destination rectangle, scaled to the final screen size.
        GL_Point_t offset;
    } vram;

#ifdef __GRAPHICS_CAPTURE_SUPPORT__
    struct {
        void *pixels;
        GifWriter gif_writer;
        size_t index;
        double time;
    } capture;
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */

    double time;
} Display_t;

extern Display_t *Display_create(const Display_Configuration_t *configuration); // TODO: rename to `Graphics`?
extern void Display_destroy(Display_t *display);

extern bool Display_should_close(const Display_t *display);
extern void Display_update(Display_t *display, float delta_time);
extern void Display_present(const Display_t *display);

extern void Display_reset(Display_t *display); // FIXME: remove these six, and access the `copperlist` field directly?

extern void Display_set_offset(Display_t *display, GL_Point_t offset);
extern void Display_set_palette(Display_t *display, const GL_Palette_t *palette);
extern void Display_set_bias(Display_t *display, int bias);
extern void Display_set_shifting(Display_t *display, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count);
extern void Display_set_copperlist(Display_t *display, const GL_CopperList_Entry_t *program, size_t length);

extern GLFWwindow *Display_get_window(const Display_t *display);
extern float Display_get_scale(const Display_t *display);
extern GL_Context_t *Display_get_context(const Display_t *display);
extern const GL_Palette_t *Display_get_palette(const Display_t *display);
extern GL_Point_t Display_get_offset(const Display_t *display);

#ifdef __GRAPHICS_CAPTURE_SUPPORT__
extern void Display_grab_snapshot(const Display_t *display, const char *base_path);
extern void Display_toggle_recording(Display_t *display, const char *base_path);
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */

#endif  /* __DISPLAY_H__ */
