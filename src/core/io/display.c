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

#include "display.h"

#include <config.h>
#include <platform.h>
#include <libs/log.h>
#include <libs/imath.h>
#include <libs/stb.h>

#define LOG_CONTEXT "display"

#if PLATFORM_ID == PLATFORM_WINDOWS
  #define PIXEL_FORMAT    GL_BGRA
#else
  #define PIXEL_FORMAT    GL_RGBA
#endif

typedef struct _Program_Data_t {
    const char *vertex_shader;
    const char *fragment_shader;
} Program_Data_t;

typedef enum Uniforms_t {
    UNIFORM_TEXTURE,
    UNIFORM_RESOLUTION,
    UNIFORM_TIME,
    Uniforms_t_CountOf
} Uniforms_t;

#define VERTEX_SHADER \
    "#version 120\n" \
    "\n" \
    "varying vec2 v_texture_coords;\n" \
    "\n" \
    "void main()\n" \
    "{\n" \
    "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n" \
    "   gl_FrontColor = gl_Color; // Pass the vertex drawing color.\n" \
    "\n" \
    "   v_texture_coords = gl_MultiTexCoord0.st; // Retain texture 2D position.\n" \
    "}\n" \

#define FRAGMENT_SHADER_PASSTHRU \
    "#version 120\n" \
    "\n" \
    "varying vec2 v_texture_coords;\n" \
    "\n" \
    "uniform sampler2D u_texture0;\n" \
    "uniform vec2 u_resolution;\n" \
    "uniform float u_time;\n" \
    "\n" \
    "vec4 passthru(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n" \
    "    return texture2D(texture, texture_coords) * color;\n" \
    "}\n" \
    "\n" \
    "void main()\n" \
    "{\n" \
    "    gl_FragColor = passthru(gl_Color, u_texture0, v_texture_coords, gl_FragCoord.xy);\n" \
    "}\n"

#define FRAGMENT_SHADER_CUSTOM \
    "#version 120\n" \
    "\n" \
    "varying vec2 v_texture_coords;\n" \
    "\n" \
    "uniform sampler2D u_texture0;\n" \
    "uniform vec2 u_resolution;\n" \
    "uniform float u_time;\n" \
    "\n" \
    "vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords);\n" \
    "\n" \
    "void main()\n" \
    "{\n" \
    "    gl_FragColor = effect(gl_Color, u_texture0, v_texture_coords, gl_FragCoord.xy);\n" \
    "}\n" \
    "\n"

static const Program_Data_t _programs_data[Display_Programs_t_CountOf] = {
    { VERTEX_SHADER, FRAGMENT_SHADER_PASSTHRU },
    { NULL, NULL }
};

static const int _texture_id_0 = 0;

static const char *_uniforms[Uniforms_t_CountOf] = {
    "u_texture0",
    "u_resolution",
    "u_time",
};

#ifdef DEBUG
static bool _has_errors(void)
{
    bool result = false;
    for (GLenum code = glGetError(); code != GL_NO_ERROR; code = glGetError()) {
        const char *message;
        switch (code) {
            case GL_INVALID_ENUM: { message = "INVALID_ENUM"; } break;
            case GL_INVALID_VALUE: { message = "INVALID_VALUE"; } break;
            case GL_INVALID_OPERATION: { message = "INVALID_OPERATION"; } break;
            case 0x506: { message = "INVALID_FRAMEBUFFER_OPERATION"; } break;
            case GL_OUT_OF_MEMORY: { message = "OUT_OF_MEMORY"; } break;
            case GL_STACK_UNDERFLOW: { message = "STACK_UNDERFLOW"; } break;
            case GL_STACK_OVERFLOW: { message = "STACK_OVERFLOW"; } break;
            default: { message = "UNKNOWN"; } break;
        }
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "OpenGL error #%04x: `GL_%s`", code, message);

        result = true;
    }
    return result;
}
#endif

static void _error_callback(int error, const char *description)
{
    Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "%s", description);
}

static void _size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height); // Viewport matches window
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "viewport size set to %dx%d", width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)width, (GLdouble)height, 0.0, 0.0, 1.0); // Configure top-left corner at <0, 0>
    glMatrixMode(GL_MODELVIEW); // Reset the model-view matrix.
    glLoadIdentity();
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "projection/model matrix reset, going otho-2D");

    glEnable(GL_TEXTURE_2D); // Default, always enabled.
    glDisable(GL_DEPTH_TEST); // We just don't need it!
    glDisable(GL_STENCIL_TEST); // Ditto.
    glDisable(GL_BLEND); // Blending is disabled.
    glDisable(GL_ALPHA_TEST);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "optimizing OpenGL features");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // TODO: configurable?
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "setting OpenGL clear-color");

#ifdef __DEBUG_TRIANGLES_WINDING__
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "enabling OpenGL debug");
#endif
}

static bool _compute_size(size_t width, size_t height, size_t scale, bool fullscreen, GL_Rectangle_t *present_area, GL_Rectangle_t *window_area, GL_Size_t *canvas_size)
{
    int display_width, display_height;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), NULL, NULL, &display_width, &display_height);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "display size is %dx%d", display_width, display_height);

    canvas_size->width = width > 0 ? width : (size_t)display_width; // width/height set to `0` means fit the display
    canvas_size->height = height > 0 ? height : (size_t)display_height;

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas size is %dx%d", canvas_size->width, canvas_size->height);

    const size_t max_scale = (size_t)imin(display_width / (int)canvas_size->width, display_height / (int)canvas_size->height);
    if (max_scale == 0) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "requested display size can't fit display!");
        return false;
    }

    const size_t window_scale = scale > max_scale ? max_scale : (scale > 0 ? scale : max_scale);
    const size_t window_width = canvas_size->width * window_scale;
    const size_t window_height = canvas_size->height * window_scale;

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "window size is %dx%d (%dx)", window_width, window_height, window_scale);

    const int x = (display_width - (int)window_width) / 2;
    const int y = (display_height - (int)window_height) / 2;
    if (!fullscreen) {
        *present_area = (GL_Rectangle_t){ // This is the vram rectangle, where the screen blit is done.
                .x = 0, .y = 0,
                .width = window_width, .height = window_height
            };
        *window_area = (GL_Rectangle_t){ // This is the windows rectangle, that is the size and position of the window.
                .x = x, .y = y,
                .width = window_width, .height = window_height
            };
    } else {
        *present_area = (GL_Rectangle_t){
                .x = x, .y = y,
                .width = window_width, .height = window_height
            };
        *window_area = (GL_Rectangle_t){
                .x = 0, .y = 0,
                .width = (size_t)display_width, .height = (size_t)display_height
            };
    }

    return true;
}

static GLFWwindow *_window_initialize(const Display_Configuration_t *configuration, GL_Rectangle_t *present_area, GL_Size_t *canvas_size)
{
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "GLFW: %s", glfwGetVersionString());

    glfwSetErrorCallback(_error_callback);

    if (!glfwInit()) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize GLFW");
        return NULL;
    }

    GL_Rectangle_t window_rectangle;
    if (!_compute_size(configuration->window.width, configuration->window.height, configuration->window.scale, configuration->fullscreen, present_area, &window_rectangle, canvas_size)) {
        glfwTerminate();
        return NULL;
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
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Initially 1x1 invisible, we will be resizing and repositioning it.

    GLFWwindow *window = glfwCreateWindow(1, 1, configuration->window.title, configuration->fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (window == NULL) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create window");
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window); // We are running on a single thread, no need to calling this in the `present()` function.

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize GLAD");
        glfwDestroyWindow(window);
        glfwTerminate();
        return NULL;
    }

    glfwSetWindowSizeCallback(window, _size_callback); // When resized we recalculate the projection properties.

    glfwSetWindowIcon(window, 1, &configuration->icon);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%s mouse cursor", configuration->hide_cursor ? "hiding" : "showing");
    glfwSetInputMode(window, GLFW_CURSOR, configuration->hide_cursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%sabling vertical synchronization", configuration->vertical_sync ? "en" : "dis");
    glfwSwapInterval(configuration->vertical_sync ? 1 : 0); // Set vertical sync, if required.

    glfwSetWindowSize(window, window_rectangle.width, window_rectangle.height);
    if (!configuration->fullscreen) {
        glfwSetWindowPos(window, window_rectangle.x, window_rectangle.y);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "window position is <%d, %d>", window_rectangle.x, window_rectangle.y);
    }
    glfwShowWindow(window); // This is not required for fullscreen window, but it makes sense anyway.
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "window shown");

    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "vendor: %s", glGetString(GL_VENDOR));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "renderer: %s", glGetString(GL_RENDERER));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "version: %s", glGetString(GL_VERSION));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "GLSL: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    return window;
}

Display_t *Display_create(const Display_Configuration_t *configuration)
{
    Display_t *display = malloc(sizeof(Display_t));
    if (!display) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate display");
        return NULL;
    }

    *display = (Display_t){
            .configuration = *configuration
        };

    display->window = _window_initialize(configuration, &display->vram.rectangle, &display->canvas.size);
    if (!display->window) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize window");
        free(display);
        return NULL;
    }

    display->canvas.context = GL_context_create(display->canvas.size.width, display->canvas.size.height);
    if (!display->canvas.context) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create graphics context");
        glfwDestroyWindow(display->window);
        glfwTerminate();
        free(display);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "graphics context %p created", display->canvas.context);

    GL_palette_generate_greyscale(&display->canvas.palette, GL_MAX_PALETTE_COLORS);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "loaded greyscale palette of %d entries", GL_MAX_PALETTE_COLORS);

    display->vram.size = display->canvas.size.width * display->canvas.size.width * sizeof(GL_Color_t);
    display->vram.pixels = malloc(display->vram.size);
    if (!display->vram.pixels) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't allocate VRAM buffer");
        GL_context_destroy(display->canvas.context);
        glfwDestroyWindow(display->window);
        glfwTerminate();
        free(display);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes VRAM allocated at %p (%dx%d)", display->vram.size, display->vram.pixels, display->canvas.size.width, display->canvas.size.height);

    glGenTextures(1, &display->vram.texture); //allocate the memory for texture
    if (display->vram.texture == 0) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't allocate VRAM texture");
        free(display->vram.pixels);
        GL_context_destroy(display->canvas.context);
        glfwDestroyWindow(display->window);
        glfwTerminate();
        free(display);
        return NULL;
    }
    glBindTexture(GL_TEXTURE_2D, display->vram.texture); // The VRAM texture is always the active and bound one.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Faster when not-power-of-two, which is the common case.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0); // Disable mip-mapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)display->canvas.size.width, (GLsizei)display->canvas.size.height, 0, PIXEL_FORMAT, GL_UNSIGNED_BYTE, NULL); // Create the storage
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "texture created w/ id #%d (%dx%d)", display->vram.texture, display->canvas.size.width, display->canvas.size.height);

#ifdef __OPENGL_STATE_CLEANUP__
    glBindTexture(GL_TEXTURE_2D, 0);
#else
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

    for (size_t i = 0; i < Display_Programs_t_CountOf; ++i) {
        const Program_Data_t *data = &_programs_data[i];
        if (!data->vertex_shader || !data->fragment_shader) {
            continue;
        }
        Program_t *program= &display->program.array[i];
        if (!program_create(program) ||
            !program_attach(program, data->vertex_shader, PROGRAM_SHADER_VERTEX) ||
            !program_attach(program, data->fragment_shader, PROGRAM_SHADER_FRAGMENT)) {
            Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize shaders");
            for (size_t j = 0; j < i; ++j) {
                program_delete(&display->program.array[j]);
            }
            glDeleteBuffers(1, &display->vram.texture);
            free(display->vram.pixels);
            GL_context_destroy(display->canvas.context);
            glfwDestroyWindow(display->window);
            glfwTerminate();
            free(display);
            return NULL;
        }

        program_prepare(program, _uniforms, Uniforms_t_CountOf);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program %p prepared w/ id #%d", program, program->id);
    }

    Display_set_shader(display, NULL); // Use pass-through shader at the beginning.

#ifdef DEBUG
    _has_errors(); // Display pending OpenGL errors.
#endif

    return display;
}

void Display_destroy(Display_t *display)
{
    for (size_t i = 0; i < Display_Programs_t_CountOf; ++i) {
        if (display->program.array[i].id == 0) {
            continue;
        }
        program_delete(&display->program.array[i]);
    }

    glDeleteBuffers(1, &display->vram.texture);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "texture w/ id #%d deleted", display->vram.texture);

    free(display->vram.pixels);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "VRAM buffer %p freed", display->vram.pixels);

    GL_context_destroy(display->canvas.context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "graphics context %p destroyed", display->canvas.context);

    glfwDestroyWindow(display->window);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "window %p destroyed", display->window);

    glfwTerminate();
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "terminated");

    free(display);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "display freed");
}

bool Display_should_close(const Display_t *display)
{
    return glfwWindowShouldClose(display->window);
}

void Display_update(Display_t *display, float delta_time)
{
    display->time += delta_time;

    GLfloat time = (GLfloat)display->time;
    program_send(display->program.active, UNIFORM_TIME, PROGRAM_UNIFORM_FLOAT, 1, &time);

#ifdef DEBUG
    _has_errors(); // Display pending OpenGL errors.
#endif
}

#ifdef PROFILE
static inline void _to_display(GLFWwindow *window, const GL_Surface_t *surface, GL_Color_t *pixels, const GL_Rectangle_t *rectangle, const GL_Point_t *offset)
{
#ifdef __OPENGL_STATE_CLEANUP__
    glBindTexture(GL_TEXTURE_2D, display->vram.texture);
#endif
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)surface->width, (GLsizei)surface->height, PIXEL_FORMAT, GL_UNSIGNED_BYTE, pixels);

    const int x0 = rectangle->x + offset->x;
    const int y0 = rectangle->y + offset->y;
    const int x1 = x0 + rectangle->width;
    const int y1 = y0 + rectangle->height;

    const float vertices[] = {
        0.0f, 0.0f, // CCW strip, top-left is <0,0> (the face direction of the strip is determined by the winding of the first triangle)
        x0, y0,
        0.0f, 1.0f,
        x0, y1,
        1.0f, 0.0f,
        x1, y0,
        1.0f, 1.0f,
        x1, y1
    };

#ifdef __OPENGL_STATE_CLEANUP__
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
    glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), vertices);
    glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), vertices + 2);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#ifdef __OPENGL_STATE_CLEANUP__
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif

    glfwSwapBuffers(window);
}
#endif

void Display_present(const Display_t *display)
{
    // It is advisable to clear the colour buffer even if the framebuffer will be
    // fully written (see `glTexSubImage2D()` below)
    glClear(GL_COLOR_BUFFER_BIT);

    // Convert the offscreen surface to a texture.
    const GL_Surface_t *surface = display->canvas.context->surface;
    GL_Color_t *pixels = display->vram.pixels;

    GL_surface_to_rgba(surface, &display->canvas.palette, pixels);

#ifdef PROFILE
    _to_display(display->window, surface, vram, &display->vram.rectangle, &display->vram_offset);
#else
#ifdef __OPENGL_STATE_CLEANUP__
    glBindTexture(GL_TEXTURE_2D, display->vram.texture);
#endif
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)surface->width, (GLsizei)surface->height, PIXEL_FORMAT, GL_UNSIGNED_BYTE, pixels);

    const GL_Rectangle_t *vram_rectangle = &display->vram.rectangle;
    const GL_Point_t *vram_offset = &display->vram.offset;

    // Add x/y offset to implement screen-shaking or similar effects.
    const int x0 = vram_rectangle->x + vram_offset->x;
    const int y0 = vram_rectangle->y + vram_offset->y;
    const int x1 = x0 + vram_rectangle->width;
    const int y1 = y0 + vram_rectangle->height;

    const float vertices[] = { // Inspired to https://github.com/emoon/minifb
        0.0f, 0.0f, // CCW strip, top-left is <0,0> (the face direction of the strip is determined by the winding of the first triangle)
        x0, y0,
        0.0f, 1.0f,
        x0, y1,
        1.0f, 0.0f,
        x1, y0,
        1.0f, 1.0f,
        x1, y1
    };

#ifdef __OPENGL_STATE_CLEANUP__
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
    glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), vertices);
    glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), vertices + 2);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#ifdef __OPENGL_STATE_CLEANUP__
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif

    glfwSwapBuffers(display->window);
#endif
}

void Display_set_palette(Display_t *display, const GL_Palette_t *palette)
{
    display->canvas.palette = *palette;
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "palette updated");
}

void Display_set_offset(Display_t *display, GL_Point_t offset)
{
    display->vram.offset = offset;
}

void Display_set_shader(Display_t *display, const char *effect)
{
    bool is_passthru = display->program.active == &display->program.array[DISPLAY_PROGRAM_PASSTHRU];

    if (!is_passthru) {
        if (display->program.active) {
            program_delete(display->program.active);
        }
    } else
    if (!effect) {
        Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "pass-thru shader already active, bailing out");
        return;
    }

    if (!effect) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "loading pass-thru shader");
        program_delete(&display->program.array[DISPLAY_PROGRAM_CUSTOM]);
        display->program.active = &display->program.array[DISPLAY_PROGRAM_PASSTHRU];
    } else {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "loading custom shader");
        const size_t length = strlen(FRAGMENT_SHADER_CUSTOM) + strlen(effect);
        char *code = malloc((length + 1) * sizeof(char)); // Add null terminator for the string.
        strcpy(code, FRAGMENT_SHADER_CUSTOM);
        strcat(code, effect);

        Program_t *program = &display->program.array[DISPLAY_PROGRAM_CUSTOM];

        if (program_create(program) &&
            program_attach(program, VERTEX_SHADER, PROGRAM_SHADER_VERTEX) &&
            program_attach(program, code, PROGRAM_SHADER_FRAGMENT)) {
            program_prepare(program, _uniforms, Uniforms_t_CountOf);
            display->program.active = program;
        } else {
            program_delete(program);
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "can't load custom shader");
        }

        free(code);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "switched to program %p", display->program.active);

    program_use(display->program.active);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program %p active", display->program.active);

    program_send(display->program.active, UNIFORM_TEXTURE, PROGRAM_UNIFORM_TEXTURE, 1, &_texture_id_0); // Redundant
    GLfloat resolution[] = {(GLfloat)display->vram.rectangle.width, (GLfloat)display->vram.rectangle.height };
    program_send(display->program.active, UNIFORM_RESOLUTION, PROGRAM_UNIFORM_VEC2, 1, resolution);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program %p initialized", display->program.active);
}

GLFWwindow *Display_get_window(const Display_t *display)
{
    return display->window;
}

float Display_get_scale(const Display_t *display)
{
    return (float)display->vram.rectangle.width / (float)display->canvas.size.width;
}

GL_Context_t *Display_get_context(const Display_t *display)
{
    return display->canvas.context;
}

const GL_Palette_t *Display_get_palette(const Display_t *display)
{
    return &display->canvas.palette;
}

GL_Point_t Display_get_offset(const Display_t *display)
{
    return display->vram.offset;
}
