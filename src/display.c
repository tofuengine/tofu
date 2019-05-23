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

#include "display.h"

#include "config.h"
#include "engine.h"
#include "hal.h"
#include "log.h"

#include <memory.h>

static const char *vertex_shader = 
    "#version 120\n"
    "\n"
    "varying vec4 color;\n"
    "varying vec2 texture_coords;\n"
    "varying vec2 screen_coords;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "   //gl_FrontColor = gl_Color;\n"
    "\n"
    "   color = gl_Color; // Pass the vertex drawing color.\n"
    "   texture_coords = gl_MultiTexCoord0.st; // Retain texture 2D position.\n"
    "   screen_coords = (gl_Position.xy + vec2(1.0, -1.0)) * vec2(0.5, -0.5); // Mirror and normalize to [0, 1].\n"
    "}\n";
;

static const char *fragment_shader =
    "#version 120\n"
    "\n"
    "varying vec4 color;\n"
    "varying vec2 texture_coords;\n"
    "varying vec2 screen_coords;\n"
    "\n"
    "uniform sampler2D texture0;\n"
    "uniform vec3 palette[256];\n"
    "\n"
    "void main()\n"
    "{\n"
    "    // Texel color fetching from texture sampler\n"
    "    vec4 texel = texture2D(texture0, texture_coords) * color; // Use `GL_color`?\n"
    "\n"
    "    // Convert the (normalized) texel color RED component (GB would work, too)\n"
    "    // to the palette index by scaling up from [0, 1] to [0, 255].\n"
    "    int index = int(floor((texel.r * 255.0) + 0.5));\n"
    "\n"
    "    // Pick the palette color as final fragment color (retain the texel alpha value).\n"
    "    // Note: palette components are pre-normalized in the OpenGL range [0, 1].\n"
    "    gl_FragColor = vec4(palette[index].rgb, texel.a);\n"
    "}\n"
;

static void error_callback(int error, const char *description)
{
    Log_write(LOG_LEVELS_ERROR, "<GLFW> %s", description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    Log_write(LOG_LEVELS_TRACE, "<GLFW> key #%d is %d", scancode, action);
}

static void size_callback(GLFWwindow* window, int width, int height)
{
    // When drawing lines we need to ensure to be in mid-pixel coordinates. Also the length of lines are inclusive
    // (and this need to be taken into account for rectangles/squares). This is due to the "diamond exit rule" in
    // OpenGL rasterization.
    //
    // http://glprogramming.com/red/appendixg.html#name1
    glViewport(0, 0, width, height); // Viewport matches window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)width, (GLdouble)height, 0.0, 0.0, 1.0); // Configure top-left corner at <0, 0>
    glMatrixMode(GL_MODELVIEW); // Reset the model-view matrix.
    glLoadIdentity();
//    glTranslatef(0.375, 0.375, 0.0); // Works both on ATI and NVIDIA cards (0.5 doesn't work).
    /* renders all primitives at integer position */

    glEnable(GL_TEXTURE_2D); // Default, always enabled.
    glDisable(GL_DEPTH_TEST); // We just don't need it!
#ifdef __FAST_TRANSPARENCY__
    glDisable(GL_BLEND); // Trade in proper alpha-blending for faster single color transparency.
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_NOTEQUAL, 0.0f);
#else
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

#ifdef __DEBUG_TRIANGLES_WINDING__
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif

    Log_write(LOG_LEVELS_DEBUG, "<GLFW> viewport size set to %dx%d", width, height);
}

bool Display_initialize(Display_t *display, const Display_Configuration_t *configuration, const char *title)
{
    display->configuration = *configuration;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        Log_write(LOG_LEVELS_FATAL, "<DISPLAY> can't initialize GLFW");
        return false;
    }

#if __GL_VERSION__ >= 0x0303
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
#endif
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    display->window = glfwCreateWindow(1, 1, title, NULL, NULL); // 1x1 window, in order to have a context early.
    if (display->window == NULL) {
        Log_write(LOG_LEVELS_FATAL, "<DISPLAY> can't create window");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(display->window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Log_write(LOG_LEVELS_FATAL, "<DISPLAY> can't initialize GLAD");
        glfwDestroyWindow(display->window);
        glfwTerminate();
        return false;
    }

    Log_write(LOG_LEVELS_INFO, "<DISPLAY> Vendor: %s", glGetString(GL_VENDOR));
    Log_write(LOG_LEVELS_INFO, "<DISPLAY> Renderer: %s", glGetString(GL_RENDERER));
    Log_write(LOG_LEVELS_INFO, "<DISPLAY> Version: %s", glGetString(GL_VERSION));
    Log_write(LOG_LEVELS_INFO, "<DISPLAY> GLSL: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glfwSetFramebufferSizeCallback(display->window, size_callback);
    glfwSetKeyCallback(display->window, key_callback);
    glfwSetInputMode(display->window, GLFW_CURSOR, configuration->hide_cursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    if (!GL_initialize()) {
        Log_write(LOG_LEVELS_FATAL, "<DISPLAY> can't initialize GL");
        glfwDestroyWindow(display->window);
        glfwTerminate();
        return false;
    }

    if (!GL_program_create(&display->program, vertex_shader, fragment_shader)) {
        Log_write(LOG_LEVELS_FATAL, "<DISPLAY> can't create default shader");
        GL_terminate();
        glfwDestroyWindow(display->window);
        glfwTerminate();
        return false;
    }
    GLint id = 0;
    GL_program_send(&display->program, "texture0", GL_PROGRAM_UNIFORM_TEXTURE, 1, &id); // Redundant
    GL_program_use(&display->program);

    int display_width, display_height;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), NULL, NULL, &display_width, &display_height);
    Log_write(LOG_LEVELS_DEBUG, "<DISPLAY> display size is %dx%d", display_width, display_height);

    display->window_width = configuration->width;
    display->window_height = configuration->height;
    display->window_scale = 1;

    if (configuration->autofit) {
        Log_write(LOG_LEVELS_DEBUG, "<DISPLAY> auto-fitting...");

        for (int s = 1; ; ++s) {
            int w = configuration->width * s;
            int h = configuration->height * s;
            if ((w > display_width) || (h > display_height)) {
                break;
            }
            display->window_width = w;
            display->window_height = h;
            display->window_scale = s;
        }
    }

    Log_write(LOG_LEVELS_DEBUG, "<DISPLAY> window size is %dx%d (%dx)", display->window_width, display->window_height,
        display->window_scale);

    if (!configuration->fullscreen) {
        int x = (display_width - display->window_width) / 2;
        int y = (display_height - display->window_height) / 2;

        glfwSetWindowMonitor(display->window, NULL, x, y, display->window_width, display->window_height, GLFW_DONT_CARE);
        glfwShowWindow(display->window);
    } else { // Toggle fullscreen by passing primary monitor!
        glfwSetWindowMonitor(display->window, glfwGetPrimaryMonitor(), 0, 0, display->window_width, display->window_height, GLFW_DONT_CARE);
    }

#if 0
    for (size_t i = 0; i < FRAMEBUFFERS_COUNT; ++i) {
        display->framebuffers[i] = LoadRenderTexture(configuration->width, configuration->height);
        SetTextureFilter(display->framebuffers[i].texture, FILTER_POINT); // Nearest-neighbour scaling.
    }

    display->offscreen_source = (Rectangle){ 0.0f, 0.0f, (float)configuration->width, (float)-configuration->height };
    if (configuration->fullscreen) {
        display->offscreen_destination = (Rectangle){ (float)x, (float)y, (float)display->window_width, (float)display->window_height };
    } else {
        display->offscreen_destination = (Rectangle){ (float)0, (float)0, (float)display->window_width, (float)display->window_height };
    }
    display->offscreen_origin = (Vector2){ 0.0f, 0.0f };

    for (size_t i = 0; i < SHADERS_COUNT; ++i) { // Initialize shader-chain.
        display->shaders[i] = (Shader){};
    }
    Display_shader(display, SHADER_INDEX_PALETTE, palette_shader_code);
#endif
    GL_Palette_t palette; // Initial gray-scale palette.
    GL_palette_greyscale(&palette, GL_MAX_PALETTE_COLORS);
    Display_palette(display, &palette);

    return true;
}

bool Display_shouldClose(Display_t *display)
{
    return glfwWindowShouldClose(display->window);
}

void Display_processInput(Display_t *display)
{
    static const int keys[Display_Keys_t_CountOf] = {
        GLFW_KEY_UP,
        GLFW_KEY_DOWN,
        GLFW_KEY_LEFT,
        GLFW_KEY_RIGHT,
        GLFW_KEY_Z,
        GLFW_KEY_S,
        GLFW_KEY_X,
        GLFW_KEY_D,
        GLFW_KEY_ENTER,
        GLFW_KEY_SPACE
    };

    for (int i = 0; i < Display_Keys_t_CountOf; ++i) {
        Display_Key_State_t *key_state = &display->keys_state[i];
        bool was_down = key_state->down;
        bool is_down = glfwGetKey(display->window, keys[i]) == GLFW_PRESS;
        key_state->down = is_down;
        key_state->pressed = !was_down && is_down;
        key_state->released = was_down && !is_down;
    }

    if (display->configuration.exit_key_enabled) {
        if (glfwGetKey(display->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(display->window, true);
        }
    }
}

void Display_renderBegin(Display_t *display)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Required, to clear previous content. (TODO: configurable color?)
    glClear(GL_COLOR_BUFFER_BIT);
}

void Display_renderEnd(Display_t *display, double now)
{
#if 0
    BeginDrawing();
#ifndef __FAST_FULLSCREEN__
        ClearBackground((Color){ 0, 0, 0, 255 });
#endif
        DrawTexturePro(display->framebuffers[source].texture, os, display->offscreen_destination,
            display->offscreen_origin, 0.0f, (Color){ 255, 255, 255, 255 });

    EndDrawing();
#endif
    glfwSwapBuffers(display->window);
    glfwPollEvents();
}

void Display_palette(Display_t *display, const GL_Palette_t *palette)
{
    GLfloat colors[MAX_PALETTE_COLORS * 3] = {};
    GL_palette_normalize(palette, colors);
    GL_program_send(&display->program, "palette", GL_PROGRAM_UNIFORM_VEC3, MAX_PALETTE_COLORS, colors);
    display->palette = *palette;
}

void Display_shader(Display_t *display, size_t index, const char *code)
{
#if 0
    if (display->shaders[index].id != 0) {
        UnloadShader(display->shaders[index]);
        display->shaders[index].id = 0;

        Log_write(LOG_LEVELS_DEBUG, "Shader %d unloaded", index);
    }

    if (!code || !code[0]) {
        return;
    }

    display->shaders[index] = LoadShaderCode(NULL, (char *)code);

    Log_write(LOG_LEVELS_DEBUG, "Shader %d loaded", index);
#endif
}

void Display_terminate(Display_t *display)
{
#if 0
    for (size_t i = 0; i < SHADERS_COUNT; ++i) {
        if (display->shaders[i].id == 0) {
            continue;
        }
        UnloadShader(display->shaders[i]);
    }
    for (size_t i = 0; i < FRAMEBUFFERS_COUNT; ++i) {
        UnloadRenderTexture(display->framebuffers[i]);
    }
#endif
    GL_program_delete(&display->program);

    GL_terminate();

    glfwDestroyWindow(display->window);
    glfwTerminate();
}