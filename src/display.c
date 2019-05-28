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
    "varying vec2 v_texture_coords;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "   gl_FrontColor = gl_Color; // Pass the vertex drawing color.\n"
    "\n"
    "   v_texture_coords = gl_MultiTexCoord0.st; // Retain texture 2D position.\n"
    "}\n";
;

static const char *fragment_shader =
    "#version 120\n"
    "\n"
    "varying vec2 v_texture_coords;\n"
    "\n"
    "uniform sampler2D u_texture0;\n"
    "//uniform vec2 u_resolution;\n"
    "//uniform float u_time;\n"
#ifndef __NO_AUTOFIT__
    "uniform int u_mode;\n"
#endif
    "\n"
    "uniform vec3 u_palette[256];\n"
    "\n"
    "vec4 palette(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n"
    "    // Texel color fetching from texture sampler\n"
    "    vec4 texel = texture2D(texture, texture_coords) * color;\n"
    "\n"
    "    // Convert the (normalized) texel color RED component (GB would work, too)\n"
    "    // to the palette index by scaling up from [0, 1] to [0, 255].\n"
    "    int index = int(floor((texel.r * 255.0) + 0.5));\n"
    "\n"
    "    // Pick the palette color as final fragment color (retain the texel alpha value).\n"
    "    // Note: palette components are pre-normalized in the OpenGL range [0, 1].\n"
    "    return vec4(u_palette[index].rgb, texel.a);\n"
    "}\n"
    "\n"
    "vec4 passthru(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n"
    "    return texture2D(texture, texture_coords) * color;\n"
    "}\n"
    "\n"
    "const float amount = 0.5;\n"
    "const float thickness = 1.0;\n"
    "const float spacing = 1.0;\n"
    "\n"
    "vec4 scanlines(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n"
    "    vec4 texel = texture2D(texture, texture_coords) * color;\n"
    "    if (mod(screen_coords.y, round(thickness + spacing)) < round(spacing)) {\n"
	"        return vec4(texel.rgb * (1.0 - amount), texel.a);\n"
    "    }\n"
    "    return texel;\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
#ifndef __NO_AUTOFIT__
    "    gl_FragColor = (u_mode == 0)\n"
    "        ? palette(gl_Color, u_texture0, v_texture_coords, gl_FragCoord.xy)\n"
    "        : passthru(gl_Color, u_texture0, v_texture_coords, gl_FragCoord.xy);\n"
    "//        : scanlines(gl_Color, u_texture0, v_texture_coords, gl_FragCoord.xy);\n"
#else
    "    gl_FragColor = palette(gl_Color, u_texture0, v_texture_coords, gl_FragCoord.xy);\n"
#endif
    "}\n"
;

#ifndef __NO_AUTOFIT__
static const int _mode_palette[] = { 0 };
static const int _mode_passthru[] = { 1 };
#endif

static void error_callback(int error, const char *description)
{
    Log_write(LOG_LEVELS_ERROR, "<GLFW> %s", description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
//    Log_write(LOG_LEVELS_TRACE, "<GLFW> key #%d is %d", scancode, action);
}

static void size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height); // Viewport matches window
    Log_write(LOG_LEVELS_DEBUG, "<GLFW> viewport size set to %dx%d", width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)width, (GLdouble)height, 0.0, 0.0, 1.0); // Configure top-left corner at <0, 0>
    glMatrixMode(GL_MODELVIEW); // Reset the model-view matrix.
    glLoadIdentity();
    Log_write(LOG_LEVELS_DEBUG, "<GLFW> projection/model matrix reset, going otho-2D");

    glEnable(GL_TEXTURE_2D); // Default, always enabled.
    glDisable(GL_DEPTH_TEST); // We just don't need it!
    glDisable(GL_STENCIL_TEST); // Ditto.
#ifdef __FAST_TRANSPARENCY__
    glDisable(GL_BLEND); // Trade in proper alpha-blending for faster single color transparency.
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_NOTEQUAL, 0.0f);
#else
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    Log_write(LOG_LEVELS_DEBUG, "<GLFW> optimizing OpenGL features");

#ifdef __DEBUG_TRIANGLES_WINDING__
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    Log_write(LOG_LEVELS_DEBUG, "<GLFW> enabling OpenGL debug");
#endif
}

#ifndef __NO_AUTOFIT__
static bool initialize_framebuffer(Display_t *display)
{
    GL_texture_create(&display->offscreen_texture, display->configuration.width, display->configuration.height, NULL);

    glGenFramebuffersEXT(1, &display->offscreen_framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, display->offscreen_framebuffer);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, display->offscreen_texture.id, 0);
    GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
        return false;
    }

    return true;
}

void deinitialize_framebuffer(Display_t *display)
{
    GL_texture_delete(&display->offscreen_texture);

    glDeleteFramebuffersEXT(1, &display->offscreen_framebuffer);
}
#endif

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
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // We'll show it after the real-size has been calculated.

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

    int display_width, display_height;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), NULL, NULL, &display_width, &display_height);
    Log_write(LOG_LEVELS_DEBUG, "<DISPLAY> display size is %dx%d", display_width, display_height);

    display->window_width = configuration->width;
    display->window_height = configuration->height;
    display->window_scale = 1;

#ifndef __NO_AUTOFIT__
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
#endif

    Log_write(LOG_LEVELS_DEBUG, "<DISPLAY> window size is %dx%d (%dx)", display->window_width, display->window_height,
        display->window_scale);

    int x = (display_width - display->window_width) / 2;
    int y = (display_height - display->window_height) / 2;
    if (!configuration->fullscreen) {
        display->offscreen_source = (GL_Quad_t){
                0, 0, configuration->width, configuration->height
            };
        display->offscreen_destination = (GL_Quad_t){
                0.0, 0.0, display->window_width, display->window_height
            };
        display->physical_width = display->window_width;
        display->physical_height = display->window_height;
        glfwSetWindowMonitor(display->window, NULL, x, y, display->window_width, display->window_height, GLFW_DONT_CARE);
        glfwShowWindow(display->window);
    } else { // Toggle fullscreen by passing primary monitor!
        display->offscreen_source = (GL_Quad_t){
                0, 0, configuration->width, configuration->height
            };
        display->offscreen_destination = (GL_Quad_t){
                x, y, x + display->window_width, y + display->window_height
            };
        display->physical_width = display_width;
        display->physical_height = display_height;
        glfwSetWindowMonitor(display->window, glfwGetPrimaryMonitor(), 0, 0, display_width, display_height, GLFW_DONT_CARE);
    }

#ifndef __NO_AUTOFIT__
    if (!initialize_framebuffer(display)) {
        Log_write(LOG_LEVELS_FATAL, "<DISPLAY> can't create framebuffer");
        GL_terminate();
        glfwDestroyWindow(display->window);
        glfwTerminate();
    }
#endif

    if (!GL_program_create(&display->program, vertex_shader, fragment_shader)) {
        Log_write(LOG_LEVELS_FATAL, "<DISPLAY> can't create default shader");
#ifndef __NO_AUTOFIT__
        deinitialize_framebuffer(display);
#endif
        GL_terminate();
        glfwDestroyWindow(display->window);
        glfwTerminate();
        return false;
    }
    GLint id = 0;
    GL_program_send(&display->program, "u_texture0", GL_PROGRAM_UNIFORM_TEXTURE, 1, &id); // Redundant
    GL_program_use(&display->program);

    GL_Palette_t palette; // Initial gray-scale palette.
    GL_palette_greyscale(&palette, GL_MAX_PALETTE_COLORS);
    Display_palette(display, &palette);

    return true;
}

bool Display_shouldClose(Display_t *display)
{
    return glfwWindowShouldClose(display->window);
}

void Display_processInput(Display_t *display, const Display_Callback_t callback, void *parameters)
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

    callback(parameters);
}

void Display_render(Display_t *display, const Display_Callback_t callback, void *parameters)
{
    // TODO: we could direct the rendering routines differently in the case `autofit` is disabled.
#ifndef __NO_AUTOFIT__
    const int w = display->configuration.width;
    const int h = display->configuration.height;
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, display->offscreen_framebuffer);
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)w, (GLdouble)h, 0.0, 0.0, 1.0); // Configure top-left corner at <0, 0>
    glMatrixMode(GL_MODELVIEW); // Reset the model-view matrix.
    glLoadIdentity();

#ifdef __FAST_TRANSPARENCY__
    glEnable(GL_ALPHA_TEST);
#else
    glEnable(GL_BLEND);
#endif

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Required, to clear previous content. (TODO: configurable color?)
    glClear(GL_COLOR_BUFFER_BIT);

    GL_program_send(&display->program, "u_mode", GL_PROGRAM_UNIFORM_INT, 1, _mode_palette);
    callback(parameters);
    GL_program_send(&display->program, "u_mode", GL_PROGRAM_UNIFORM_INT, 1, _mode_passthru);

    const int pw = display->physical_width;
    const int ph = display->physical_height;
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glViewport(0, 0, pw, ph);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)pw, 0.0, (GLdouble)ph, 0.0, 1.0); // Configure bottom-left corner at <0, 0> (FOB is inverted)
    glMatrixMode(GL_MODELVIEW); // Reset the model-view matrix.
    glLoadIdentity();

#ifdef __FAST_TRANSPARENCY__
    glDisable(GL_ALPHA_TEST);
#else
    glDisable(GL_BLEND);
#endif

    GL_texture_blit_fast(&display->offscreen_texture, display->offscreen_source, display->offscreen_destination, (GL_Color_t){ 255, 255, 255, 255 });

#else
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Required, to clear previous content. (TODO: configurable color?)
    glClear(GL_COLOR_BUFFER_BIT);

    callback(parameters);
#endif

    glfwSwapBuffers(display->window);
    glfwPollEvents();
}

void Display_palette(Display_t *display, const GL_Palette_t *palette)
{
    GLfloat colors[MAX_PALETTE_COLORS * 3] = {};
    GL_palette_normalize(palette, colors);
    GL_program_send(&display->program, "u_palette", GL_PROGRAM_UNIFORM_VEC3, MAX_PALETTE_COLORS, colors);
    display->palette = *palette;
}

void Display_terminate(Display_t *display)
{
    GL_program_delete(&display->program);

#ifndef __NO_AUTOFIT__
    deinitialize_framebuffer(display);
#endif
    GL_terminate();

    glfwDestroyWindow(display->window);
    glfwTerminate();
}