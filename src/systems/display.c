/*
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

#include "display.h"

#include <core/config.h>
#include <core/platform.h>
#define _LOG_TAG "display"
#include <libs/log.h>
#include <libs/imath.h>
#include <libs/stb.h>

#include <time.h>

// Value for setting the "zero time" of the engine. This will trick the system
// and get the consistent precision of an integer, with the convenient units
// of a double, as the exponent will remain constant for ~136 years (since the
// time unit is represented in seconds).
//
// See: `Four billion dollar question`, here https://randomascii.wordpress.com/2012/02/13/dont-store-that-in-a-float/
#define _ENGINE_EPOCH 4294967296.0

#if PLATFORM_ID == PLATFORM_WINDOWS
    #define _PIXEL_FORMAT GL_BGRA
#else
    #define _PIXEL_FORMAT GL_RGBA
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

#define _VERTEX_SHADER \
    "#version 120\n" \
    "\n" \
    "void main()\n" \
    "{\n" \
    "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n" \
    "   gl_FrontColor = gl_Color; // Pass the vertex drawing color.\n" \
    "\n" \
    "   gl_TexCoord[0] = gl_MultiTexCoord0; // Retain texture #0 coordinates.\n" \
    "}\n" \

#define _FRAGMENT_SHADER \
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

static const char *_uniforms[Uniforms_t_CountOf] = {
    "u_texture0",
    "u_texture_size",
    "u_screen_size",
    "u_screen_scale",
    "u_time",
};

#if defined(DEBUG)
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
        LOG_E("OpenGL error #%04x: `GL_%s`", code, message);

        result = true;
    }
    return result;
}
#endif

static void _error_callback(int error, const char *description)
{
    LOG_E("%s", description);
}

static void _size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height); // Viewport matches window
    LOG_D("viewport size set to %dx%d", width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)width, (GLdouble)height, 0.0, 0.0, 1.0); // Configure top-left corner at <0, 0>
    glMatrixMode(GL_MODELVIEW); // Reset the model-view matrix.
    glLoadIdentity();
    LOG_D("projection/model matrix reset, going otho-2D");

    glEnable(GL_TEXTURE_2D); // Default, always enabled.
    glDisable(GL_DEPTH_TEST); // We just don't need it!
    glDisable(GL_STENCIL_TEST); // Ditto.
    glDisable(GL_BLEND); // Blending is disabled.
    glDisable(GL_ALPHA_TEST);
    LOG_D("optimizing OpenGL features");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // TODO: configurable?
    LOG_D("setting OpenGL clear-color");

#if defined(TOFU_GRAPHICS_DEBUG_TRIANGLES_WINDING)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    LOG_D("enabling OpenGL debug");
#endif
}

static void _close_callback(GLFWwindow *window)
{
    const Display_Configuration_t *configuration = (const Display_Configuration_t *)glfwGetWindowUserPointer(window);

    // The close flag has been set before this callback is called, so we can override it.
    glfwSetWindowShouldClose(window, configuration->quit_on_close ? GLFW_TRUE : GLFW_FALSE);
    LOG_D("closing flag set to `%s`", configuration->quit_on_close ? "true" : "false");
}

static bool _compute_size(size_t width, size_t height, size_t scale, bool fullscreen, GL_Rectangle_t *present_area, GL_Rectangle_t *window_area, GL_Size_t *canvas_size)
{
    int display_width, display_height;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), NULL, NULL, &display_width, &display_height);
    LOG_D("display size is %dx%d", display_width, display_height);

    canvas_size->width = width > 0 ? width : (size_t)display_width; // width/height set to `0` means fit the display
    canvas_size->height = height > 0 ? height : (size_t)display_height;

    LOG_D("canvas size is %dx%d", canvas_size->width, canvas_size->height);

    const size_t max_scale = (size_t)imin(display_width / (int)canvas_size->width, display_height / (int)canvas_size->height);
    if (max_scale == 0) {
        LOG_F("requested display size can't fit display!");
        return false;
    }

    const size_t window_scale = scale > max_scale ? max_scale : (scale > 0 ? scale : max_scale);
    const size_t window_width = canvas_size->width * window_scale;
    const size_t window_height = canvas_size->height * window_scale;

    LOG_D("window size is %dx%d (%dx)", window_width, window_height, window_scale);

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

static GLFWwindow *_window_create(const Display_Configuration_t *configuration, GL_Rectangle_t *present_area, GL_Size_t *canvas_size)
{
    GL_Rectangle_t window_rectangle;
    if (!_compute_size(configuration->window.width, configuration->window.height, configuration->window.scale, configuration->fullscreen, present_area, &window_rectangle, canvas_size)) {
        return NULL;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API); // On Wayland it's the same as `GLFW_EGL_CONTEXT_API`.
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
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Initially 1x1 invisible, we will be resizing and repositioning it.
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    // Note: technically starting from GLFW v3.4 we could display the window from the very first moment at the
    //       correct position with the correct size. However, we find useful to leverage the "size callback" to set
    //       OpenGL up. Otherwise we would call it directly. Also, we prefer to set everything up (e.g. the icon) and
    //       then display the window.
    GLFWwindow *window = glfwCreateWindow(1, 1, configuration->window.title, configuration->fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (!window) {
        LOG_F("can't create window");
        return NULL;
    }
    glfwMakeContextCurrent(window); // We are running on a single thread, no need to calling this in the `present()` function.
    LOG_D("window %p created (and made current context)", window);

    glfwSetWindowUserPointer(window, (void *)configuration);
    // glfwSetWindowFocusCallback(window, window_focus_callback)
    glfwSetWindowSizeCallback(window, _size_callback); // When resized we recalculate the projection properties.
    glfwSetWindowCloseCallback(window, _close_callback); // Override close button, according to configuration.

    if (configuration->icon.pixels) {
        glfwSetWindowIcon(window, 1, &configuration->icon);
    } else {
        LOG_W("icon is missing");
    }

    LOG_D("%sabling vertical synchronization", configuration->vertical_sync ? "en" : "dis");
    glfwSwapInterval(configuration->vertical_sync ? 1 : 0); // Set vertical sync, if required.

    glfwSetWindowSize(window, (int)window_rectangle.width, (int)window_rectangle.height);
    if (!configuration->fullscreen) {
        glfwSetWindowPos(window, window_rectangle.x, window_rectangle.y);
        LOG_D("window position is <%d, %d>", window_rectangle.x, window_rectangle.y);
    }
    glfwShowWindow(window); // This is not required for fullscreen window, but it makes sense anyway.
    LOG_D("window shown");

    return window;
}

static void _window_destroy(GLFWwindow *window)
{
    glfwDestroyWindow(window);
}

static bool _shader_initialize(Display_t *display, const char *effect)
{
    if (!effect) {
        LOG_E("shader is null");
        return false;
    }

    display->shader = shader_create();
    if (!display->shader) {
        return false;
    }

    LOG_D("loading shader %p", display->shader);

    const size_t length = strlen(_FRAGMENT_SHADER) + strlen(effect);
    char *code = malloc(sizeof(char) * (length + 1)); // Add null terminator for the string.
    strcpy(code, _FRAGMENT_SHADER); // We are safe using `strcpy()` as we pre-allocated the correct buffer length.
    strcat(code, effect);

    if (!shader_attach(display->shader, _VERTEX_SHADER, SHADER_TYPE_VERTEX) ||
        !shader_attach(display->shader, code, SHADER_TYPE_FRAGMENT)) {
        shader_destroy(display->shader);
        return false;
    }

    shader_prepare(display->shader, _uniforms, Uniforms_t_CountOf);
    LOG_D("shader %p prepared", display->shader);

    shader_use(display->shader);
    LOG_D("shader %p active", display->shader);

    shader_send(display->shader, UNIFORM_TEXTURE, SHADER_UNIFORM_TEXTURE, 1, (const int[]){ 0 }); // Redundant
    shader_send(display->shader, UNIFORM_SCREEN_SIZE, SHADER_UNIFORM_VEC2, 1, (const GLfloat[]){
            (GLfloat)display->vram.size.width,
            (GLfloat)display->vram.size.height
        });
    shader_send(display->shader, UNIFORM_TEXTURE_SIZE, SHADER_UNIFORM_VEC2, 1, (const GLfloat[]){
            (GLfloat)display->configuration.window.width,
            (GLfloat)display->configuration.window.height
        });
    shader_send(display->shader, UNIFORM_SCREEN_SCALE, SHADER_UNIFORM_VEC2, 1, (const GLfloat[]){
            (GLfloat)display->vram.size.width / (GLfloat)display->configuration.window.width,
            (GLfloat)display->vram.size.height / (GLfloat)display->configuration.window.height
        });

    LOG_D("shader %p initialized", display->shader);

    free(code);

    return true;
}

static void *_allocate(size_t size, void *user)
{
    return malloc(size);
}

static void _deallocate(void* block, void *user)
{
    free(block);
}

static void *_reallocate(void* block, size_t size, void *user)
{
    return realloc(block, size);
}

Display_t *Display_create(const Display_Configuration_t *configuration)
{
    Display_t *display = malloc(sizeof(Display_t));
    if (!display) {
        LOG_E("can't allocate display");
        return NULL;
    }

    *display = (Display_t){
            .configuration = *configuration
        };

    glfwSetErrorCallback(_error_callback);

    if (!glfwInit()) {
        LOG_F("can't initialize GLFW");
        goto error_free;
    }
    LOG_D("GLFW initialized");

    glfwInitAllocator(&(GLFWallocator){
            .allocate = _allocate,
            .deallocate = _deallocate,
            .reallocate = _reallocate,
            .user = NULL
        });
    LOG_D("GLFW allocator set");

    glfwSetTime(_ENGINE_EPOCH);
    LOG_D("time initialized");

    GL_Rectangle_t vram_rectangle;
    display->window = _window_create(&display->configuration, &vram_rectangle, &display->canvas.size);
    if (!display->window) {
        LOG_F("can't initialize window");
        goto error_terminate_glfw;
    }
    LOG_D("window %p initialized", display->window);

#if defined(GLAD_OPTION_GL_ON_DEMAND)
    gladSetGLOnDemandLoader((GLADloadfunc)glfwGetProcAddress);
    LOG_D("GLAD on-demand loader set (using generator %s)", GLAD_GENERATOR_VERSION);
#elif defined(GLAD_OPTION_GL_LOADER)
    int glad_version = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    if (!glad_version) {
        LOG_F("can't initialize GLAD");
        goto error_destroy_window;
    }
    LOG_D("GLAD initialized (using generator %s)", GLAD_GENERATOR_VERSION);
#endif  /* GLAD_OPTION_GL_ON_DEMAND || GLAD_OPTION_GL_LOADER */

    display->vram.position = (GL_Point_t){ .x = vram_rectangle.x, .y = vram_rectangle.y };
    display->vram.size = (GL_Size_t){ .width = vram_rectangle.width, .height = vram_rectangle.height };

    display->canvas.surface = GL_surface_create(display->canvas.size.width, display->canvas.size.height);
    if (!display->canvas.surface) {
        LOG_F("can't create graphics surface");
        goto error_destroy_window;
    }
    LOG_D("graphics surface %p created", display->canvas.surface);

    // TODO: implement a small boot effect?
    GL_surface_clear(display->canvas.surface, 0);
    LOG_D("graphics surface %p cleared", display->canvas.surface);

    display->canvas.processor = GL_processor_create();
    if (!display->canvas.processor) {
        LOG_F("can't create processor");
        goto error_destroy_surface;
    }
    LOG_D("processor %p created", display->canvas.processor);

    size_t size = sizeof(GL_Color_t) * display->canvas.size.width * display->canvas.size.height;
    display->vram.pixels = malloc(size);
    if (!display->vram.pixels) {
        LOG_F("can't allocate VRAM buffer");
        goto error_destroy_processor;
    }
    LOG_D("%d bytes VRAM allocated at %p (%dx%d)", size, display->vram.pixels, display->canvas.size.width, display->canvas.size.height);

    glGenTextures(1, &display->vram.texture); //allocate the memory for texture
    if (display->vram.texture == 0) {
        LOG_F("can't allocate VRAM texture");
        goto error_free_vram;
    }
    glBindTexture(GL_TEXTURE_2D, display->vram.texture); // The VRAM texture is always the active and bound one.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Faster when not-power-of-two, which is the common case.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0); // Disable mip-mapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)display->canvas.size.width, (GLsizei)display->canvas.size.height, 0, _PIXEL_FORMAT, GL_UNSIGNED_BYTE, NULL); // Create the storage
    LOG_D("texture created w/ id #%d (%dx%d)", display->vram.texture, display->canvas.size.width, display->canvas.size.height);

#if defined(TOFU_DISPLAY_OPENGL_STATE_CLEANUP)
    glBindTexture(GL_TEXTURE_2D, 0);
#else
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

    bool shader = _shader_initialize(display, configuration->effect);
    if (!shader) {
        LOG_F("can't initialize shader");
        goto error_delete_buffers;
    }

#if defined(DEBUG)
    _has_errors(); // Display pending OpenGL errors.
#endif

    LOG_I("GLFW: %s", glfwGetVersionString());
#if !defined(GLAD_OPTION_GL_ON_DEMAND)
    LOG_I("GLAD: %d.%d", GLAD_VERSION_MAJOR(glad_version), GLAD_VERSION_MINOR(glad_version));
#endif  /* GLAD_OPTION_GL_ON_DEMAND */
    LOG_I("vendor: %s", glGetString(GL_VENDOR));
    LOG_I("renderer: %s", glGetString(GL_RENDERER));
    LOG_I("version: %s", glGetString(GL_VERSION));
    LOG_I("GLSL: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    return display;

error_delete_buffers:
    glDeleteBuffers(1, &display->vram.texture);
error_free_vram:
    free(display->vram.pixels);
error_destroy_processor:
    GL_processor_destroy(display->canvas.processor);
error_destroy_surface:
    GL_surface_destroy(display->canvas.surface);
error_destroy_window:
    _window_destroy(display->window);
error_terminate_glfw:
    glfwTerminate();
error_free:
    free(display);
    return NULL;
}

void Display_destroy(Display_t *display)
{
    shader_destroy(display->shader);
    LOG_D("shader %p destroyed", display->shader);

    glDeleteBuffers(1, &display->vram.texture);
    LOG_D("texture w/ id #%d deleted", display->vram.texture);

    free(display->vram.pixels);
    LOG_D("VRAM buffer %p freed", display->vram.pixels);

    GL_processor_destroy(display->canvas.processor);
    LOG_D("processor %p destroyed", display->canvas.processor);

    GL_surface_destroy(display->canvas.surface);
    LOG_D("graphics surface %p destroyed", display->canvas.surface);

#if defined(GLAD_OPTION_GL_LOADER)
    gladLoaderUnloadGL();
    LOG_D("GLAD unloaded");
#endif  /* GLAD_OPTION_GL_LOADER */

    _window_destroy(display->window);
    LOG_D("window %p destroyed", display->window);

    glfwTerminate();
    LOG_D("display terminated");

    free(display);
    LOG_D("display freed");
}

void Display_close(Display_t *display)
{
    glfwSetWindowShouldClose(display->window, true);
}

bool Display_should_close(const Display_t *display)
{
    return glfwWindowShouldClose(display->window);
}

bool Display_update(Display_t *display, float delta_time)
{
    display->time += delta_time;

    GLfloat time = (GLfloat)display->time;
    shader_send(display->shader, UNIFORM_TIME, SHADER_UNIFORM_FLOAT, 1, &time);

#if defined(DEBUG)
    _has_errors(); // Display pending OpenGL errors.
#endif

    return true;
}

void Display_present(const Display_t *display)
{
    // It is advisable to clear the colour buffer even if the framebuffer will be
    // fully written (see `glTexSubImage2D()` below)
    glClear(GL_COLOR_BUFFER_BIT);

    // Convert the offscreen surface to a texture. The actual function changes when a processor is defined.
    const GL_Surface_t *surface = display->canvas.surface;
    GL_Color_t *pixels = display->vram.pixels;

    GL_processor_surface_to_rgba(display->canvas.processor, surface, pixels);

#if defined(TOFU_DISPLAY_OPENGL_STATE_CLEANUP)
    glBindTexture(GL_TEXTURE_2D, display->vram.texture);
#endif
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)display->canvas.size.width, (GLsizei)display->canvas.size.height, _PIXEL_FORMAT, GL_UNSIGNED_BYTE, pixels);

    const GL_Point_t *vram_position = &display->vram.position;
    const GL_Size_t *vram_size = &display->vram.size;
    const GL_Point_t *vram_offset = &display->vram.offset;

    // Add x/y offset to implement screen-shaking or similar effects.
    const int x0 = vram_position->x + vram_offset->x;
    const int y0 = vram_position->y + vram_offset->y;
    const int x1 = x0 + (int)vram_size->width;
    const int y1 = y0 + (int)vram_size->height;

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

#if defined(TOFU_DISPLAY_OPENGL_STATE_CLEANUP)
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
    glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), vertices);
    glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), vertices + 2);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#if defined(TOFU_DISPLAY_OPENGL_STATE_CLEANUP)
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif

    glfwSwapBuffers(display->window);
}

void Display_reset(Display_t *display)
{
    display->vram.offset = (GL_Point_t){ 0, 0 };
    GL_processor_reset(display->canvas.processor);
}

void Display_set_offset(Display_t *display, GL_Point_t offset)
{
    display->vram.offset = offset;
}

void Display_set_palette(Display_t *display, const GL_Color_t *palette)
{
    GL_processor_set_palette(display->canvas.processor, palette);
}

void Display_set_shifting(Display_t *display, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count)
{
    GL_processor_set_shifting(display->canvas.processor, from, to, count);
}

void Display_set_program(Display_t *display, const GL_Program_t *program)
{
    GL_processor_set_program(display->canvas.processor, program);
}

GLFWwindow *Display_get_window(const Display_t *display)
{
    return display->window;
}

GL_Size_t Display_get_virtual_size(const Display_t *display)
{
    return display->canvas.size;
}

GL_Size_t Display_get_physical_size(const Display_t *display)
{
    return display->vram.size;
}

GL_Surface_t *Display_get_surface(const Display_t *display)
{
    return display->canvas.surface;
}

const GL_Color_t *Display_get_palette(const Display_t *display)
{
    return GL_processor_get_palette(display->canvas.processor);
}

GL_Point_t Display_get_offset(const Display_t *display)
{
    return display->vram.offset;
}
