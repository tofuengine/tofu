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

#ifndef TOFU_SYSTEMS_DISPLAY_H
#define TOFU_SYSTEMS_DISPLAY_H

// TODO: rename Display to Video?

#include <core/config.h>
#include <libs/gl/gl.h>
#include <libs/shader.h>

#include <cglm/cglm.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdbool.h>
#include <stddef.h>

typedef struct Display_Configuration_s {
    GLFWimage icon;
    struct {
        const char *title;
        size_t width, height, scale;
    } window;
    bool fullscreen;
    bool vertical_sync;
    bool quit_on_close;
    const char *effect;
} Display_Configuration_t;

typedef struct Display_s {
    Display_Configuration_t configuration;

    GLFWwindow *window;

    Shader_t *shader;
    GLuint vbo;
    GLuint vao;
#if defined(TOFU_GRAPHICS_SAVE_MVP_MATRIX)
    mat4 mvp;
#endif

    struct {
        GL_Size_t size;
        GL_Surface_t *surface;
        GL_Processor_t *processor; // The processor holds the display-wise palette and shifting logic.
    } canvas;

    struct {
        GLuint texture;
        GL_Color_t *pixels; // Temporary buffer to create the OpenGL texture from `GL_Pixel_t` array.
        GL_Point_t position; // Destination position, scaled to the final screen size.
        GL_Size_t size; // Duplicates rectangle, for faster return of size.
        GL_Point_t offset;
    } vram;

    double time;
} Display_t;

extern Display_t *Display_create(const Display_Configuration_t *configuration); // TODO: rename to `Graphics`?
extern void Display_destroy(Display_t *display);

extern void Display_close(Display_t *display);
extern bool Display_should_close(const Display_t *display);

extern bool Display_update(Display_t *display, float delta_time);

extern void Display_present(const Display_t *display);

extern void Display_reset(Display_t *display); // FIXME: remove these six, and access the `processor` field directly?

extern void Display_set_offset(Display_t *display, GL_Point_t offset);
extern void Display_set_palette(Display_t *display, const GL_Color_t *palette);
extern void Display_set_shifting(Display_t *display, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count);
extern void Display_set_program(Display_t *display, const GL_Program_t *program);

extern GLFWwindow *Display_get_window(const Display_t *display);
extern GL_Size_t Display_get_virtual_size(const Display_t *display);
extern GL_Size_t Display_get_physical_size(const Display_t *display);
extern GL_Surface_t *Display_get_surface(const Display_t *display);
extern const GL_Color_t *Display_get_palette(const Display_t *display);
extern GL_Point_t Display_get_offset(const Display_t *display);

#endif  /* TOFU_SYSTEMS_DISPLAY_H */
