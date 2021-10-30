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

#include "display.h"

#include <config.h>
#include <platform.h>
#include <libs/log.h>
#include <libs/imath.h>
#include <libs/stb.h>

#include <time.h>

#define LOG_CONTEXT "display"

#if PLATFORM_ID == PLATFORM_WINDOWS
  #define PIXEL_FORMAT    GL_BGRA
#else
  #define PIXEL_FORMAT    GL_RGBA
#endif

#ifdef __GRAPHICS_CAPTURE_SUPPORT__
  #define CAPTURE_FRAMES_PER_SECOND     50
  #define CAPTURE_FRAME_TIME            (1.0f / CAPTURE_FRAMES_PER_SECOND)
  #define CAPTURE_FRAME_TIME_100TH      (100 / CAPTURE_FRAMES_PER_SECOND)
#endif

typedef enum Uniforms_t {
    UNIFORM_TEXTURE,
    UNIFORM_TEXTURE_SIZE,
    UNIFORM_SCREEN_SIZE,
    UNIFORM_SCREEN_SCALE,
    UNIFORM_TIME,
    Uniforms_t_CountOf
} Uniforms_t;

// https://www.lighthouse3d.com/tutorials/glsl-12-tutorial/hello-world-in-glsl/
// https://ptgmedia.pearsoncmg.com/images/9780321552624/downloads/0321552628_AppI.pdf
// https://relativity.net.au/gaming/glsl/Built-inVariables.html
// https://www.khronos.org/registry/OpenGL/specs/gl/
// https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.20.pdf
// https://www.khronos.org/opengl/wiki/GLSL_:_common_mistakes

#define VERTEX_SHADER \
    "#version 120\n" \
    "\n" \
    "void main()\n" \
    "{\n" \
    "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n" \
    "   gl_FrontColor = gl_Color; // Pass the vertex drawing color.\n" \
    "\n" \
    "   gl_TexCoord[0] = gl_MultiTexCoord0; // Retain texture #0 coordinates.\n" \
    "}\n" \

#define FRAGMENT_SHADER \
    "#version 120\n" \
    "\n" \
    "uniform sampler2D u_texture0;\n" \
    "uniform vec2 u_texture_size;\n" \
    "uniform vec2 u_screen_size;\n" \
    "uniform vec2 u_screen_scale;\n" \
    "uniform float u_time;\n" \
    "\n" \
    "vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords);\n" \
    "\n" \
    "void main()\n" \
    "{\n" \
    "    gl_FragColor = effect(gl_Color, u_texture0, gl_TexCoord[0].st, gl_FragCoord.xy);\n" \
    "}\n" \
    "\n"

#define EFFECT_PASSTHRU \
    "vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n" \
    "    return texture2D(texture, texture_coords) * color;\n" \
    "}\n"

static const char *_uniforms[Uniforms_t_CountOf] = {
    "u_texture0",
    "u_texture_size",
    "u_screen_size",
    "u_screen_scale",
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
    glfwSetErrorCallback(_error_callback);

    if (!glfwInit()) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize GLFW");
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "GLFW initialized");

    GL_Rectangle_t window_rectangle;
    if (!_compute_size(configuration->window.width, configuration->window.height, configuration->window.scale, configuration->fullscreen, present_area, &window_rectangle, canvas_size)) {
        glfwTerminate();
        return NULL;
    }

#if __GL_VERSION__ >= 0x0303
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#elif __GL_VERSION__ == 0x0300
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
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
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "window %p created (and made current context)", window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize GLAD");
        glfwDestroyWindow(window);
        glfwTerminate();
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "GLAD initialized");

    glfwSetWindowSizeCallback(window, _size_callback); // When resized we recalculate the projection properties.

    if (configuration->icon.pixels) {
        glfwSetWindowIcon(window, 1, &configuration->icon);
    } else {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "icon is missing");
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%s mouse cursor", configuration->hide_cursor ? "hiding" : "showing");
    glfwSetInputMode(window, GLFW_CURSOR, configuration->hide_cursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%sabling vertical synchronization", configuration->vertical_sync ? "en" : "dis");
    glfwSwapInterval(configuration->vertical_sync ? 1 : 0); // Set vertical sync, if required.

    glfwSetWindowSize(window, (int)window_rectangle.width, (int)window_rectangle.height);
    if (!configuration->fullscreen) {
        glfwSetWindowPos(window, window_rectangle.x, window_rectangle.y);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "window position is <%d, %d>", window_rectangle.x, window_rectangle.y);
    }
    glfwShowWindow(window); // This is not required for fullscreen window, but it makes sense anyway.
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "window shown");

    return window;
}

static bool _shader_initialize(Display_t *display, const char *effect)
{
    display->shader = shader_create();
    if (!display->shader) {
        return false;
    }

    if (effect) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "loading custom shader");
    } else {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "loading pass-thru shader");
        effect = EFFECT_PASSTHRU;
    }
    const size_t length = strlen(FRAGMENT_SHADER) + strlen(effect);
    char *code = malloc(sizeof(char) * (length + 1)); // Add null terminator for the string.
    strcpy(code, FRAGMENT_SHADER); // We are safe using `strcpy()` as we pre-allocated the correct buffer length.
    strcat(code, effect);

    if (!shader_attach(display->shader, VERTEX_SHADER, SHADER_TYPE_VERTEX) ||
        !shader_attach(display->shader, code, SHADER_TYPE_FRAGMENT)) {
        shader_destroy(display->shader);
        return false;
    }

    shader_prepare(display->shader, _uniforms, Uniforms_t_CountOf);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shader %p prepared", display->shader);

    shader_use(display->shader);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shader %p active", display->shader);

    shader_send(display->shader, UNIFORM_TEXTURE, SHADER_UNIFORM_TEXTURE, 1, (const int[]){ 0 }); // Redundant
    shader_send(display->shader, UNIFORM_SCREEN_SIZE, SHADER_UNIFORM_VEC2, 1, (const GLfloat[]){
            (GLfloat)display->vram.rectangle.width,
            (GLfloat)display->vram.rectangle.height
        });
    shader_send(display->shader, UNIFORM_TEXTURE_SIZE, SHADER_UNIFORM_VEC2, 1, (const GLfloat[]){
            (GLfloat)display->configuration.window.width,
            (GLfloat)display->configuration.window.height
        });
    shader_send(display->shader, UNIFORM_SCREEN_SCALE, SHADER_UNIFORM_VEC2, 1, (const GLfloat[]){
            (GLfloat)display->vram.rectangle.width / (GLfloat)display->configuration.window.width,
            (GLfloat)display->vram.rectangle.height / (GLfloat)display->configuration.window.height
        });

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shader %p initialized", display->shader);

    free(code);

    return true;
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
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "window %p initialized", display->window);

    display->canvas.surface = GL_surface_create(display->canvas.size.width, display->canvas.size.height);
    if (!display->canvas.surface) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create graphics surface");
        glfwDestroyWindow(display->window);
        glfwTerminate();
        free(display);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "graphics surface %p created", display->canvas.surface);

    // TODO: implement a small boot effect?
    GL_surface_clear(display->canvas.surface, 0);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "graphics surface %p cleared", display->canvas.surface);

    display->canvas.copperlist = GL_copperlist_create();
    if (!display->canvas.copperlist) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create copperlist");
        GL_surface_destroy(display->canvas.surface);
        glfwDestroyWindow(display->window);
        glfwTerminate();
        free(display);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist %p created", display->canvas.copperlist);

    size_t size = sizeof(GL_Color_t) * display->canvas.size.width * display->canvas.size.height;
    display->vram.pixels = malloc(size);
    if (!display->vram.pixels) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't allocate VRAM buffer");
        GL_copperlist_destroy(display->canvas.copperlist);
        GL_surface_destroy(display->canvas.surface);
        glfwDestroyWindow(display->window);
        glfwTerminate();
        free(display);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d bytes VRAM allocated at %p (%dx%d)", size, display->vram.pixels, display->canvas.size.width, display->canvas.size.height);

    glGenTextures(1, &display->vram.texture); //allocate the memory for texture
    if (display->vram.texture == 0) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't allocate VRAM texture");
        free(display->vram.pixels);
        GL_copperlist_destroy(display->canvas.copperlist);
        GL_surface_destroy(display->canvas.surface);
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

    bool shader = _shader_initialize(display, configuration->effect);
    if (!shader) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize shader");
        glDeleteBuffers(1, &display->vram.texture);
        free(display->vram.pixels);
        GL_copperlist_destroy(display->canvas.copperlist);
        GL_surface_destroy(display->canvas.surface);
        glfwDestroyWindow(display->window);
        glfwTerminate();
        free(display);
        return NULL;
    }

#ifdef __GRAPHICS_CAPTURE_SUPPORT__
    display->capture.pixels = malloc(display->vram.rectangle.width * display->vram.rectangle.height * 4);
    if (!display->capture.pixels) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't allocate capture buffer");
        shader_destroy(display->shader);
        glDeleteBuffers(1, &display->vram.texture);
        free(display->vram.pixels);
        GL_copperlist_destroy(display->canvas.copperlist);
        GL_context_destroy(display->canvas.context);
        glfwDestroyWindow(display->window);
        glfwTerminate();
        free(display);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "capture buffer %p allocated", display->capture.pixels);
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */

#ifdef DEBUG
    _has_errors(); // Display pending OpenGL errors.
#endif

    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "GLFW: %s", glfwGetVersionString());
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "vendor: %s", glGetString(GL_VENDOR));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "renderer: %s", glGetString(GL_RENDERER));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "version: %s", glGetString(GL_VERSION));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "GLSL: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    return display;
}

void Display_destroy(Display_t *display)
{
#ifdef __GRAPHICS_CAPTURE_SUPPORT__
    if (GifIsWriting(&display->capture.gif_writer)) {
        GifEnd(&display->capture.gif_writer);
        Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "recording stopped");
    }

    free(display->capture.pixels);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "capture buffer %p freed", display->capture.pixels);
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */

    shader_destroy(display->shader);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shader %p destroyed", display->shader);

    glDeleteBuffers(1, &display->vram.texture);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "texture w/ id #%d deleted", display->vram.texture);

    free(display->vram.pixels);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "VRAM buffer %p freed", display->vram.pixels);

    GL_copperlist_destroy(display->canvas.copperlist);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist %p destroyed", display->canvas.copperlist);

    GL_surface_destroy(display->canvas.surface);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "graphics surface %p destroyed", display->canvas.surface);

    glfwDestroyWindow(display->window);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "window %p destroyed", display->window);

    glfwTerminate();
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "display terminated");

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
    shader_send(display->shader, UNIFORM_TIME, SHADER_UNIFORM_FLOAT, 1, &time);

#ifdef __GRAPHICS_CAPTURE_SUPPORT__
    // Since GIFs' delay is expressed in 100th of seconds, we automatically "auto-sample" at a proper
    // framerate in order to preserve the period (e.g. 25 FPS is fine).
    if (GifIsWriting(&display->capture.gif_writer)) {
        display->capture.time += delta_time;
        while (display->capture.time >= CAPTURE_FRAME_TIME) {
            display->capture.time -= CAPTURE_FRAME_TIME;
            GifWriteFrame(&display->capture.gif_writer, display->capture.pixels, display->vram.rectangle.width, display->vram.rectangle.height, CAPTURE_FRAME_TIME_100TH, 8, false); // Hundredths of seconds.
        }
    }
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */

#ifdef DEBUG
    _has_errors(); // Display pending OpenGL errors.
#endif
}

void Display_present(const Display_t *display)
{
    // It is advisable to clear the colour buffer even if the framebuffer will be
    // fully written (see `glTexSubImage2D()` below)
    glClear(GL_COLOR_BUFFER_BIT);

    // Convert the offscreen surface to a texture. The actual function changes when a copperlist is defined.
    GL_Surface_t *surface = display->canvas.surface;
    GL_Color_t *pixels = display->vram.pixels;

    GL_copperlist_surface_to_rgba(surface, display->canvas.copperlist, pixels);

#ifdef __OPENGL_STATE_CLEANUP__
    glBindTexture(GL_TEXTURE_2D, display->vram.texture);
#endif
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)display->canvas.size.width, (GLsizei)display->canvas.size.height, PIXEL_FORMAT, GL_UNSIGNED_BYTE, pixels);

    const GL_Rectangle_t *vram_rectangle = &display->vram.rectangle;
    const GL_Point_t *vram_offset = &display->vram.offset;

    // Add x/y offset to implement screen-shaking or similar effects.
    const int x0 = vram_rectangle->x + vram_offset->x;
    const int y0 = vram_rectangle->y + vram_offset->y;
    const int x1 = x0 + (int)vram_rectangle->width;
    const int y1 = y0 + (int)vram_rectangle->height;

    // Performance note: passing a stack-located array (and not on the heap) greately increase `glDrawArrays()` throughput!
    const float vertices[] = { // Inspired to https://github.com/emoon/minifb
        0.0f, 0.0f, // CCW strip, top-left is <0,0> (the face direction of the strip is determined by the winding of the first triangle)
        (float)x0, (float)y0,
        0.0f, 1.0f,
        (float)x0, (float)y1,
        1.0f, 0.0f,
        (float)x1, (float)y0,
        1.0f, 1.0f,
        (float)x1, (float)y1
    };

#ifdef __OPENGL_STATE_CLEANUP__
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
    glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), vertices);
    glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), vertices + 2);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#ifdef __GRAPHICS_CAPTURE_SUPPORT__
    // Read the framebuffer data, which include the final stretching and post-processing effects. The call is
    // synchronous in that it waits the previous drawing function to be completed on the texture. But this is fine
    // for us as we need to capture the full frame-buffer. Just disable this feature if not required, as it slows
    // down the rendering *a lot*.
    //
    // https://vec.io/posts/faster-alternatives-to-glreadpixels-and-glteximage2d-in-opengl-es
    // https://www.khronos.org/opengl/wiki/Pixel_Transfer
    // https://www.khronos.org/opengl/wiki/Pixel_Buffer_Object
    glReadPixels(0, 0, vram_rectangle->width, vram_rectangle->height, PIXEL_FORMAT, GL_UNSIGNED_BYTE, display->capture.pixels);
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */

#ifdef __OPENGL_STATE_CLEANUP__
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif

    glfwSwapBuffers(display->window);
}

void Display_reset(Display_t *display)
{
    display->vram.offset = (GL_Point_t){ 0, 0 };
    GL_copperlist_reset(display->canvas.copperlist);
}

void Display_set_offset(Display_t *display, GL_Point_t offset)
{
    display->vram.offset = offset;
}

void Display_set_shifting(Display_t *display, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count)
{
    GL_copperlist_set_shifting(display->canvas.copperlist, from, to, count);
}

void Display_set_program(Display_t *display, const GL_Program_t *program)
{
    GL_copperlist_set_program(display->canvas.copperlist, program);
}

GLFWwindow *Display_get_window(const Display_t *display)
{
    return display->window;
}

float Display_get_scale(const Display_t *display)
{
    return (float)display->vram.rectangle.width / (float)display->canvas.size.width;
}

GL_Surface_t *Display_get_surface(const Display_t *display)
{
    return display->canvas.surface;
}

GL_Palette_t *Display_get_palette(const Display_t *display)
{
    return display->canvas.copperlist->palette;
}

GL_Point_t Display_get_offset(const Display_t *display)
{
    return display->vram.offset;
}

#ifdef __GRAPHICS_CAPTURE_SUPPORT__
void Display_grab_snapshot(const Display_t *display, const char *base_path)
{
    time_t t = time(0);
    struct tm *lt = localtime(&t);

    char path[PLATFORM_PATH_MAX] = { 0 };
    sprintf(path, "%s%csnapshot-%04d%02d%02d%02d%02d%02d.png", base_path, PLATFORM_PATH_SEPARATOR, lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

    stbi_write_png(path, display->vram.rectangle.width, display->vram.rectangle.height, 4, display->capture.pixels, display->vram.rectangle.width * 4);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "capture done to file `%s`", path);
}

void Display_toggle_recording(Display_t *display, const char *base_path)
{
    if (!GifIsWriting(&display->capture.gif_writer)) {
        time_t t = time(0);
        struct tm *lt = localtime(&t);

        char path[PLATFORM_PATH_MAX] = { 0 };
        sprintf(path, "%s%crecord-%04d%02d%02d%02d%02d%02d.gif", base_path, PLATFORM_PATH_SEPARATOR, lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

        GifBegin(&display->capture.gif_writer, path, display->vram.rectangle.width, display->vram.rectangle.height, 0);
        Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "recording started to file `%s`", path);

        display->capture.time = 0.0;
    } else {
        GifEnd(&display->capture.gif_writer);
        Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "recording stopped");
    }
}
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */