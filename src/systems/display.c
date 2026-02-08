/*
 * MIT License
 *
 * Copyright (c) 2019-2026 Marco Lizza
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

#if TOFU_GRAPHICS_PIXEL_FORMAT == PIXEL_FORMAT_RGBA8888
#if PLATFORM_ID == PLATFORM_WINDOWS
    #define _PIXEL_FORMAT GL_BGRA
#else
    #define _PIXEL_FORMAT GL_RGBA
#endif
    #define _PIXEL_TYPE   GL_UNSIGNED_BYTE
    #define _PIXEL_BPP    4
#elif TOFU_GRAPHICS_PIXEL_FORMAT == PIXEL_FORMAT_RGB565
    #define _PIXEL_FORMAT GL_RGB
    #define _PIXEL_TYPE   GL_UNSIGNED_SHORT_5_6_5
    #define _PIXEL_BPP    2
#else
    #error "unsupported TOFU_GRAPHICS_PIXEL_FORMAT"
#endif

typedef enum Uniforms_t {
    UNIFORM_MVP,
    UNIFORM_TEXTURE,
    UNIFORM_TEXTURE_SIZE,
    UNIFORM_SCREEN_SIZE,
    UNIFORM_SCREEN_SCALE,
    UNIFORM_SCREEN_OFFSET,
    UNIFORM_COLOR,
    UNIFORM_TIME,
    Uniforms_t_CountOf
} Uniforms_t;

// https://antongerdelan.net/opengl/vertexbuffers.html
// https://open.gl/drawing
// https://learnopengl.com/Getting-started/Hello-Triangle
// https://paroj.github.io/gltut/
// https://relativity.net.au/gaming/glsl/Built-inVariables.html
// https://www.khronos.org/registry/OpenGL/specs/gl/
// https://www.khronos.org/opengl/wiki/GLSL_:_common_mistakes

#define VERTEX_LOCATION_POSITION        0
#define VERTEX_LOCATION_TEXTURE_COORDS  1

#define FRAGMENT_LOCATION_COLOR         0

// TODO: move shaders to kernal?

// We are implementing the display offset (e.g. to implement shaking) by moving
// the framebuffer texture destination quad. This requires to calculate the
// position of only four points, instead of moving every texture pixel in the
// fragment-shader. Also, this ensure the background to be "black".
#define _VERTEX_SHADER \
    "#version 330 core\n" \
    "\n" \
    "layout (location = 0) in vec2 i_position;\n" \
    "layout (location = 1) in vec2 i_texture_coords;\n" \
    "\n" \
    "out vec2 v_texture_coords;\n" \
    "\n" \
    "uniform vec2 u_screen_offset;\n" \
    "uniform mat4 u_mvp;\n" \
    "\n" \
    "void main() {\n" \
    "   v_texture_coords = i_texture_coords;\n" \
    "\n" \
    "   gl_Position = u_mvp * vec4(i_position + u_screen_offset, 0.0, 1.0);\n" \
    "}\n"

#define _FRAGMENT_SHADER \
    "#version 330 core\n" \
    "\n" \
    "layout (origin_upper_left) in vec4 gl_FragCoord;\n" \
    "\n" \
    "in vec2 v_texture_coords;\n" \
    "\n" \
    "layout (location = 0) out vec4 o_color;\n" \
    "\n" \
    "uniform sampler2D u_texture0;\n" \
    "uniform vec2 u_texture_size;\n" \
    "uniform vec2 u_screen_size;\n" \
    "uniform vec2 u_screen_scale;\n" \
    "uniform vec4 u_color;\n" \
    "uniform float u_time;\n" \
    "\n" \
    "vec4 effect(sampler2D texture, vec2 texture_coords, vec2 screen_coords);\n" \
    "\n" \
    "void main() {\n" \
    "    vec2 screen_coords = gl_FragCoord.xy;\n" \
    "\n" \
    "    o_color = effect(u_texture0, v_texture_coords, screen_coords) * u_color;\n" \
    "}\n"

static const char *_uniforms[Uniforms_t_CountOf] = {
    "u_mvp", // The model-view-projection matrix, precomputed.
    "u_texture0", // The current texture ID.
    "u_texture_size", // The size of the target (on-screen) area.
    "u_screen_size", // Represents the size of the pixel canvas (1:1 pixel size).
    "u_screen_scale", // The scaling factor between the (offscreen) drawing buffer and the displaying window/screen.
    "u_screen_offset", // Expressed in pixels of the drawing QUAD, scaled during the setting process to preserve the original pixels' size.
    "u_color",
    "u_time",
};

// Important Note
// ==============
//
// In order to help OpenGL debugging we purposely keep the global state to an "empty" condition. For this
// reason we change OpenGL's state only temporarily and revert it back to "blank" when finished to reduce
// the state dependencies.

#if defined(TOFU_GRAPHICS_REPORT_OPENGL_ERRORS)
#if 0
static void _opengl_debug_proc(GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar *message,
            const void *userParam)
{
    LOG_E("OpenGL error #%04x: %s", code, message);
}
#endif

static bool _has_errors(void)
{
    bool result = false;
    for (GLenum code = glGetError(); code != GL_NO_ERROR; code = glGetError()) {
        const char *message;
        switch (code) {
            case GL_INVALID_ENUM: { message = "INVALID_ENUM"; } break;
            case GL_INVALID_VALUE: { message = "INVALID_VALUE"; } break;
            case GL_INVALID_OPERATION: { message = "INVALID_OPERATION"; } break;
            case GL_OUT_OF_MEMORY: { message = "OUT_OF_MEMORY"; } break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: { message = "INVALID_FRAMEBUFFER_OPERATION"; } break;
            default: { message = "UNKNOWN"; } break;
        }
        LOG_E("OpenGL error #%04x: `GL_%s`", code, message);

        result = true;
    }
    return result;
}
#endif

/**
 * void glOrtho(double l, double r, double b, double t, double n, double f);
 *
 * produces this matrix
 *
 *     | 2 / (r − l)        0              0         - (r + l) / (r - l) |
 *     |      0        2 / (t − b)         0         - (t + b) / (t - b) |
 *     |      0             0       - 2 / (f − n)    - (f + n) / (f - n) |
 *     |      0             0              0                   1         |
 *
 * that in our case reduces to
 *
 *     | 2 / w      0      0    - 1 |
 *     |   0    - 2 / h    0      1 |
 *     |   0        0    - 2    - 1 |
 *     |   0        0      0      1 |
 *
 */
static void _size_callback(GLFWwindow *window, int width, int height)
{
    // Note: the size-callback function is called from within the message-pump loop,
    //       and for that reason we are safe to assume that when we reach here (for
    //       the first time) everything have been initialized. Most notably, the shader
    //       that we can send data to.
#if defined(TOFU_GRAPHICS_SAVE_MVP_MATRIX)
    Display_t *display = (Display_t *)glfwGetWindowUserPointer(window);
#else
    const Display_t *display = (const Display_t *)glfwGetWindowUserPointer(window);
#endif

    glViewport(0, 0, width, height); // Viewport matches window
    LOG_D("viewport size set to %dx%d", width, height);

    // With legacy/immediate mode, we used
    // - an `glOrtho` built matrix as PROJECTION,
    // - an identity matrix as MODEL-VIEW.
    //
    // This translates into an orthographic MVP matrix, which can be calculated with a single call.
    mat4 mvp;
    glm_ortho(0.0, (float)width, (float)height, 0.0, 0.0, 1.0, mvp);
    LOG_D("orthographic (2D) model/view/projection matrix generated");
#if defined(TOFU_GRAPHICS_SAVE_MVP_MATRIX)
    memcpy(display->mvp, mvp, sizeof(mat4)); // There's no need to store it, we are sending right away to the shader.
    LOG_D("model/view/projection matrix stored");
#endif
    shader_use(display->shader);
    shader_send(display->shader, UNIFORM_MVP, SHADER_UNIFORM_MAT4, 1, mvp);
    shader_use(NULL);
    LOG_D("model/view/projection matrix sent to the shader");

    // On OpenGL core profile `GL_TEXTURE_2D` is not a valid argument to `glEnable()` as it can't be disable. There's
    // no fixed-function running in the pipeline as the color of the pixel is solely determined by the fragment shader.
    glDisable(GL_DEPTH_TEST); // We just don't need it!
    glDisable(GL_STENCIL_TEST); // Ditto.
    glDisable(GL_BLEND); // Blending is disabled.
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
    const Display_t *display = (const Display_t *)glfwGetWindowUserPointer(window);
    const Display_Configuration_t *configuration = &display->configuration;

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

    const size_t window_scale = scale > 0 && scale <= max_scale ? scale : max_scale;
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

static GLFWwindow *_window_create(const Display_t *display, GL_Rectangle_t *present_area, GL_Size_t *canvas_size)
{
    const Display_Configuration_t *configuration = &display->configuration;

    GL_Rectangle_t window_rectangle;
    if (!_compute_size(configuration->window.width, configuration->window.height, configuration->window.scale, configuration->fullscreen, present_area, &window_rectangle, canvas_size)) {
        return NULL;
    }

    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API); // On Wayland it's the same as `GLFW_EGL_CONTEXT_API`.
#if defined(TOFU_CORE_OPENGL_ES)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.3 is the first "version unified" OpenGL.
#endif
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Initially 1x1 invisible, we will be resizing and repositioning it.
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    // Note: technically, starting from GLFW v3.4 we could display the window from the very first moment at the
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

    glfwSetWindowUserPointer(window, (void *)display);
    // glfwSetWindowFocusCallback(window, window_focus_callback)
    glfwSetWindowSizeCallback(window, _size_callback); // When resized we recalculate the projection properties.
    glfwSetWindowCloseCallback(window, _close_callback); // Override close button, according to configuration.

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
#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (!effect) {
        LOG_E("shader effect is null");
        goto error_exit;
    }
#endif /* TOFU_CORE_DEFENSIVE_CHECKS */

    const size_t length = strlen(_FRAGMENT_SHADER) + strlen(effect);
    char *shader_code = malloc(sizeof(char) * (length + 1)); // Add null terminator for the string.
    if (!shader_code) {
        LOG_E("can' allocate memory for the fragment shader code");
        goto error_exit;
    }
    strcpy(shader_code, _FRAGMENT_SHADER); // We are safe using `strcpy()` as we pre-allocated the correct buffer length.
    strcat(shader_code, effect);

    display->shader = shader_create(_VERTEX_SHADER, shader_code, _uniforms, Uniforms_t_CountOf);
    if (!display->shader) {
        LOG_E("can' create the shader");
        goto error_free_shader_code;
    }

    LOG_D("shader %p created", display->shader);

    shader_use(display->shader);

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
    shader_send(display->shader, UNIFORM_SCREEN_OFFSET, SHADER_UNIFORM_VEC2, 1, (const GLfloat[]){
            (GLfloat)0.0f, (GLfloat)0.0f
        });
    shader_send(display->shader, UNIFORM_COLOR, SHADER_UNIFORM_VEC4, 1, (const GLfloat[]){ // TODO: configurable?
            (GLfloat)1.0f, (GLfloat)1.0f, (GLfloat)1.0f, (GLfloat)1.0f
        });

    shader_use(NULL);

    LOG_D("shader %p initialized", display->shader);

    free(shader_code);

    return true;

error_free_shader_code:
    free(shader_code);
error_exit:
    return false;
}

/*
 * An OpenGL VAO can be loosely thought as follows:
 *
 *     struct VertexAttrib {
 *       GLint size;           // set by gVertexAttrib(I)Pointer
 *       GLenum type;          // set by gVertexAttrib(I)Pointer
 *       GLboolean normalize;  // set by gVertexAttrib(I)Pointer
 *       GLsizei stride;       // set by gVertexAttrib(I)Pointer
 *       GLint buffer;         // set by gVertexAttrib(I)Pointer (indirectly)
 *       void* pointer;        // set by gVertexAttrib(I)Pointer
 *       GLint divisor;        // set by gVertexAttribDivisor
 *       GLboolean enabled;    // set by gEnable/DisableVertexAttribArray
 *     };
 *     
 *     struct VertexArrayObject {
 *       std::vector<VertexAttrib> attribs;
 *       GLuint element_array_buffer;  // set by glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ..)
 *     };
 *
 * See: https://webgl2fundamentals.org/webgl/lessons/resources/webgl-state-diagram.html
 */
#pragma pack(push, 1)
typedef struct _Vertex_s {
    GLfloat position[2];
    GLfloat texture_coords[2];
} _Vertex_t;
#pragma pack(pop)

static bool _initialize_vertices(Display_t *display)
{
    const GL_Point_t *vram_position = &display->vram.position;
    const GL_Size_t *vram_size = &display->vram.size;
    // Note: x/y offset are passed through the shader!

    const int x = vram_position->x;
    const int y = vram_position->y;

    const int x0 = x;
    const int y0 = y;
    const int x1 = x + (int)vram_size->width;
    const int y1 = y + (int)vram_size->height;

    // CCW strip, top-left is <0,0> (the face direction of the strip is determined by the winding of the first triangle)
    const _Vertex_t vertices[] = {
        { { (GLfloat)x0, (GLfloat)y0 }, { 0.0f, 0.0f } },
        { { (GLfloat)x0, (GLfloat)y1 }, { 0.0f, 1.0f } },
        { { (GLfloat)x1, (GLfloat)y0 }, { 1.0f, 0.0f } },
        { { (GLfloat)x1, (GLfloat)y1 }, { 1.0f, 1.0f } }
    };

    glGenVertexArrays(1, &display->vao);
    if (display->vao == 0) {
        goto error_exit;
    }

    glGenBuffers(1, &display->vbo);
    if (display->vbo == 0) {
        goto error_delete_vertex_array;
    }

    glBindVertexArray(display->vao);
    glBindBuffer(GL_ARRAY_BUFFER, display->vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(VERTEX_LOCATION_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(_Vertex_t), 0); // These two calls make the VAO (indirectly) store the current VBO!
    glVertexAttribPointer(VERTEX_LOCATION_TEXTURE_COORDS, 2, GL_FLOAT, GL_FALSE, sizeof(_Vertex_t), (void *)(offsetof(_Vertex_t, texture_coords))); // (they change the VAO state)
    glEnableVertexAttribArray(VERTEX_LOCATION_POSITION);
    glEnableVertexAttribArray(VERTEX_LOCATION_TEXTURE_COORDS);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;

error_delete_vertex_array:
    glDeleteVertexArrays(1, &display->vao);
error_exit:
    return false;
}

Display_t *Display_create(const Display_Configuration_t *configuration)
{
    Display_t *display = malloc(sizeof(Display_t));
    if (!display) {
        LOG_E("can't allocate display");
        goto error_exit;
    }

    *display = (Display_t){
            .configuration = *configuration
        };

    GL_Rectangle_t vram_rectangle;
    display->window = _window_create(display, &vram_rectangle, &display->canvas.size);
    if (!display->window) {
        LOG_F("can't initialize window");
        goto error_free_display;
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
    display->vram.pixels.count = sizeof(GL_Color_t) * display->canvas.size.width * display->canvas.size.height;
    display->vram.pixels.bytes = malloc(display->vram.pixels.count);
    if (!display->vram.pixels.bytes) {
        LOG_F("can't allocate VRAM buffer");
        goto error_unload_glad;
    }
    LOG_D("%d bytes VRAM allocated at %p (%dx%d)", display->vram.pixels.count, display->vram.pixels.bytes, display->canvas.size.width, display->canvas.size.height);

    glGenTextures(DISPLAY_BUFFERS_COUNT, display->vram.textures.ids);
    if (display->vram.textures.ids[DISPLAY_BUFFERS_COUNT - 1] == 0) { // Check the last one, to be safe :>
        LOG_F("can't allocate VRAM texture buffers");
        goto error_free_vram;
    }

    for (int i = 0; i < DISPLAY_BUFFERS_COUNT; ++i) {
        glBindTexture(GL_TEXTURE_2D, display->vram.textures.ids[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Faster when not-power-of-two, which is the common case.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0); // Disable mip-mapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexImage2D(GL_TEXTURE_2D,
                0, // Mipmap level, 0 is the base level.
                GL_RGB,
                (GLsizei)display->canvas.size.width, (GLsizei)display->canvas.size.height,
                0, // Border, must be 0.
                _PIXEL_FORMAT, _PIXEL_TYPE,
                NULL); // Create the storage
        LOG_D("texture buffer w/ id #%d created (%dx%d)", display->vram.textures.ids[i], display->canvas.size.width, display->canvas.size.height);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

#if defined(TOFU_GRAPHICS_ASYNC_UPLOAD)
    glGenBuffers(DISPLAY_PBO_RING_SIZE, display->vram.pbo.buffers);
//    if (display->vram.pbo.buffers[DISPLAY_PBO_RING_SIZE - 1] == 0) { // Check the last one, to be safe :>
//        LOG_F("can't allocate VRAM texture buffers");
//        goto error_free_texture_buffers;
//    }
    for (int i = 0; i < DISPLAY_PBO_RING_SIZE; ++i) {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, display->vram.pbo.buffers[i]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, display->vram.pixels.count, NULL, GL_STREAM_DRAW);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
#endif  /* TOFU_GRAPHICS_ASYNC_UPLOAD */

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Tight packing (no padding), we are only transferring data (one time set)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    LOG_D("pixel unpack alignment and row-length set to 1/0");

    bool shader = _shader_initialize(display, configuration->effect);
    if (!shader) {
        LOG_F("can't initialize shader");
        goto error_delete_buffers;
    }

    bool vertices = _initialize_vertices(display);
    if (!vertices) {
        LOG_F("can't initialize vertices");
        goto error_destroy_shader;
    }

    display->canvas.surface = GL_surface_create(display->canvas.size.width, display->canvas.size.height);
    if (!display->canvas.surface) {
        LOG_F("can't create graphics surface");
        goto error_destroy_vertices;
    }
    LOG_D("graphics surface %p created", display->canvas.surface);

    display->canvas.clear_index = configuration->clear_index;
    LOG_D("display clear index set to %d", display->canvas.clear_index);

    display->canvas.processor = GL_processor_create();
    if (!display->canvas.processor) {
        LOG_F("can't create processor");
        goto error_destroy_surface;
    }
    LOG_D("processor %p created", display->canvas.processor);

    if (configuration->palette) {
        GL_processor_set_palette(display->canvas.processor, configuration->palette);
        LOG_D("processor %p palette set", display->canvas.processor);
    }

    LOG_I("GLFW: %s", glfwGetVersionString());
    LOG_I("GLFW platform: %d", glfwGetPlatform());
#if !defined(GLAD_OPTION_GL_ON_DEMAND)
    LOG_I("GLAD: %d.%d", GLAD_VERSION_MAJOR(glad_version), GLAD_VERSION_MINOR(glad_version));
#endif  /* GLAD_OPTION_GL_ON_DEMAND */
    LOG_I("vendor: %s", glGetString(GL_VENDOR));
    LOG_I("renderer: %s", glGetString(GL_RENDERER));
    LOG_I("version: %s", glGetString(GL_VERSION));
    LOG_I("GLSL: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

#if 0
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    bool is_debug = (flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0;
    is_debug = is_debug;

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // opzionale: può costare; valutalo
    glDebugMessageCallback(_opengl_debug_proc, NULL);
#endif

#if defined(TOFU_GRAPHICS_REPORT_OPENGL_ERRORS)
    _has_errors(); // Display pending OpenGL errors.
#endif

    LOG_D("display %p created", display);

    return display;

error_destroy_surface:
    GL_surface_destroy(display->canvas.surface);
error_destroy_vertices:
    glDeleteBuffers(1, &display->vbo);
    glDeleteVertexArrays(1, &display->vao);
error_destroy_shader:
    shader_destroy(display->shader);
error_delete_buffers:
#if defined(TOFU_GRAPHICS_ASYNC_UPLOAD)
    glDeleteBuffers(DISPLAY_PBO_RING_SIZE, display->vram.pbo.buffers);
#endif  /* TOFU_GRAPHICS_ASYNC_UPLOAD */
    glDeleteBuffers(DISPLAY_BUFFERS_COUNT, display->vram.textures.ids);
error_free_vram:
    free(display->vram.pixels.bytes);
error_unload_glad:
#if defined(GLAD_OPTION_GL_LOADER)
    gladLoaderUnloadGL();
#endif  /* GLAD_OPTION_GL_LOADER */
error_destroy_window:
    _window_destroy(display->window);
error_free_display:
    free(display);
error_exit:
    return NULL;
}

void Display_destroy(Display_t *display)
{
    LOG_D("destroying display %p", display);

    GL_processor_destroy(display->canvas.processor);
    LOG_D("processor %p destroyed", display->canvas.processor);

    GL_surface_destroy(display->canvas.surface);
    LOG_D("graphics surface %p destroyed", display->canvas.surface);

    glDeleteBuffers(1, &display->vbo);
    glDeleteVertexArrays(1, &display->vao);
    LOG_D("VAO/VBO deleted");

    shader_destroy(display->shader);
    LOG_D("shader %p destroyed", display->shader);

#if defined(TOFU_GRAPHICS_ASYNC_UPLOAD)
    glDeleteBuffers(DISPLAY_PBO_RING_SIZE, display->vram.pbo.buffers);
#endif  /* TOFU_GRAPHICS_ASYNC_UPLOAD */
    glDeleteBuffers(DISPLAY_BUFFERS_COUNT, display->vram.textures.ids);
    LOG_D("texture buffers deleted");

    free(display->vram.pixels.bytes);
    LOG_D("VRAM buffer %p freed", display->vram.pixels.bytes);

#if defined(GLAD_OPTION_GL_LOADER)
    gladLoaderUnloadGL();
    LOG_D("GLAD unloaded");
#endif  /* GLAD_OPTION_GL_LOADER */

    _window_destroy(display->window);
    LOG_D("window %p destroyed", display->window);

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

    shader_use(display->shader);

    GLfloat time = (GLfloat)display->time;
    shader_send(display->shader, UNIFORM_TIME, SHADER_UNIFORM_FLOAT, 1, &time);

    shader_use(NULL);

    return true;
}

// TODO: adopt a `clear_function_t` to remove the branch?
void Display_clear(Display_t *display)
{
    if (display->canvas.clear_index < 0) {
        return; // No clear required.
    }
    GL_surface_clear(display->canvas.surface, display->canvas.clear_index);
}

static inline void _cycle_textures(Display_t *display, GLint *writing_texture, GLint *reading_texture)
{
    int index = display->vram.textures.index;
#if DISPLAY_BUFFERS_COUNT == 2
    *reading_texture = display->vram.textures.ids[index];
    index = 1 - index;
    *writing_texture = display->vram.textures.ids[index];
#else
    *reading_texture = display->vram.textures.ids[index];
    index = (index + 1) % DISPLAY_BUFFERS_COUNT;
    *writing_texture = display->vram.textures.ids[index];
#endif
    display->vram.textures.index = index;
}

void Display_present(Display_t *display)
{
    GLint writing_texture, reading_texture;
    _cycle_textures(display, &writing_texture, &reading_texture);

    // It is advisable to clear the colour buffer even if the framebuffer will be
    // fully written (see `glTexSubImage2D()` below)
    glClear(GL_COLOR_BUFFER_BIT);

    // --------------------------------
    // ----------:: UPLOAD ::----------
    // --------------------------------
    glBindTexture(GL_TEXTURE_2D, writing_texture);

    // Convert the offscreen surface to a texture. The actual function changes
    // when a processor is defined.
    //
    // Note: we might be interested in converting to a temporary `pixels`
    //       buffer since the conversion is not trivial. The memory pointer
    //       returned for the PBO need to be accessed as quick as possible,
    //       and a straight `memcpy()` call is the best option we have. So, the
    //       suggested sequence is
    //          CPU buffer -> memcpy -> PBO -> TexSubImage2D
    //       Also, this way we can control the PBO usage with a macro. :)
    const GL_Surface_t *surface = display->canvas.surface;
    GL_Color_t *pixels = display->vram.pixels.bytes;

    GL_processor_surface_to_pixels(display->canvas.processor, surface, pixels);

#if defined(TOFU_GRAPHICS_ASYNC_UPLOAD)
    int pbo_index = display->vram.pbo.index; // Cycle through the ring buffer
    const GLuint pbo = display->vram.pbo.buffers[pbo_index];
    display->vram.pbo.index = (pbo_index + 1) % DISPLAY_PBO_RING_SIZE;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

    void *staging_buffer = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, display->vram.pixels.count,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
#if defined(TOFU_GRAPHICS_ASYNC_UPLOAD_ORPHAN_RECOVERING)
    if (!staging_buffer) {
        // In the RARE case the driver can't give us a pointer to the buffer,
        // we recover by asking for a new one (discarding the previous one for
        // good). We keep this ONLY just as recovery strategy since "orphaning"
        // (when performed as per-frame basis) would just add unneeded overhead
        // with not benefits). We leave the decision to the driver
        //
        // NOTE: relying on a per-frame orphaning and using a single PBO won't
        //       solve sync issues as it would work on "most" drivers, but not
        //       always. Also the continue requests to orphan stress the driver
        //       and make it stall. With a PBO ring this is unlikely to happen.
        LOG_W("orphaning the PBO and recovering...");

        glBufferData(GL_PIXEL_UNPACK_BUFFER, display->vram.pixels.count, NULL, GL_STREAM_DRAW); // Orphaning (i.e. discard the previous buffer)!
        staging_buffer = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, display->vram.pixels.count,
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    }
#endif  /* TOFU_GRAPHICS_ASYNC_UPLOAD_ORPHAN_RECOVERING */
    if (staging_buffer) {
        memcpy(staging_buffer, pixels, display->vram.pixels.count); // Stage from the CPU buffer, as quick as possible!
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    } else {
        LOG_E("can't retrieve the PBO upload pointer");
    }

    glTexSubImage2D(GL_TEXTURE_2D, // Asynchronously transfer from the staging are to the texture.
            0,
            0, 0,
            display->canvas.size.width, display->canvas.size.height,
            _PIXEL_FORMAT, _PIXEL_TYPE,
            NULL); // Transferring from the PBO!

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
#else   /* TOFU_GRAPHICS_ASYNC_UPLOAD */
    glTexSubImage2D(GL_TEXTURE_2D,
            0, // Level of detail
            0, 0, // Note: the size of the texture is the same as the canvas, so we can update the whole texture with a single call.
            (GLsizei)display->canvas.size.width, (GLsizei)display->canvas.size.height,
            _PIXEL_FORMAT, _PIXEL_TYPE,
            pixels);
#endif  /* TOFU_GRAPHICS_ASYNC_UPLOAD */

    // --------------------------------
    // -----------:: DRAW ::-----------
    // --------------------------------
    glBindTexture(GL_TEXTURE_2D, reading_texture);

    // We need to restore the drawing state, which includes (1) the shader
    // program, (2) the vertices attributes, and (3) the texture to be drawn.
    shader_use(display->shader);
    glBindVertexArray(display->vao);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // The VAO knows everything ;)

    glBindVertexArray(0);
    shader_use(NULL);

    glBindTexture(GL_TEXTURE_2D, 0);

    glfwSwapBuffers(display->window);
}

void Display_reset(Display_t *display)
{
    Display_set_offset(display, (GL_Point_t){ 0, 0 });

    GL_processor_reset(display->canvas.processor);

    display->canvas.clear_index = display->configuration.clear_index;
}

void Display_set_clear_index(Display_t *display, int index)
{
    display->canvas.clear_index = index;
}

void Display_set_offset(Display_t *display, GL_Point_t offset)
{
    display->vram.offset = offset;

    // We need to scale the offset as it is expressed in pixels of the offscreen canvas, which
    // can be smaller of the VRAM rendering window (if scaled)!
    const size_t scale = display->configuration.window.scale;

    const GLfloat ox = (GLfloat)offset.x * (GLfloat)scale; // The scale is the same on both axes!
    const GLfloat oy = (GLfloat)offset.y * (GLfloat)scale;

    shader_use(display->shader);
    shader_send(display->shader, UNIFORM_SCREEN_OFFSET, SHADER_UNIFORM_VEC2, 1, (const GLfloat[]){ ox, oy });
    shader_use(NULL);
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

GL_Point_t Display_get_offset(const Display_t *display)
{
    return display->vram.offset;
}

const GL_Color_t *Display_get_palette(const Display_t *display)
{
    return GL_processor_get_palette(display->canvas.processor);
}
