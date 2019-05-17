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

#ifndef GLfloat
typedef float GLfloat;
#endif

#define VALUES_PER_COLOR            3

#define UNCAPPED_FPS                0

#define FPS_HISTOGRAM_HEIGHT        30
#define FPS_TEXT_HEIGHT             10
#define FPS_MAX_VALUE               90

static const char *palette_shader_code = 
    "#version 330\n"
    "\n"
    "const int colors = 256;\n"
    "\n"
    "// Input fragment attributes (from fragment shader)\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "\n"
    "// Input uniform values\n"
    "uniform sampler2D texture0;\n"
    "uniform vec3 palette[colors];\n"
    "\n"
    "// Output fragment color\n"
    "out vec4 finalColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    // Texel color fetching from texture sampler\n"
    "    vec4 texelColor = texture(texture0, fragTexCoord) * fragColor;\n"
    "\n"
    "    // Convert the (normalized) texel color RED component (GB would work, too)\n"
    "    // to the palette index by scaling up from [0, 1] to [0, 255].\n"
    "    int index = int(floor((texelColor.r * 255.0) + 0.5));\n"
    "\n"
    "    // Pick the palette color as final fragment color (retain the texel alpha value).\n"
    "    // Note: palette components are pre-normalized in the OpenGL range [0, 1].\n"
    "    finalColor = vec4(palette[index].rgb, texelColor.a);\n"
    "}\n"
;

static void draw_statistics(const Engine_Statistics_t *statistics)
{
    DrawRectangle(0, 0, STATISTICS_LENGTH, FPS_HISTOGRAM_HEIGHT + FPS_TEXT_HEIGHT, (Color){ 63, 63, 63, 191 });
    for (int i = 0; i < STATISTICS_LENGTH; ++i) {
        int index = (statistics->index + i) % STATISTICS_LENGTH;
        double fps = statistics->history[index];
        int height = (int)((fps / (double)FPS_MAX_VALUE) * (double)FPS_HISTOGRAM_HEIGHT);
        if (height > FPS_HISTOGRAM_HEIGHT) {
            height = FPS_HISTOGRAM_HEIGHT;
        }
        Color color;
        if (fps >= 60.0) {  // We are safe to do pretty much anything.
            color = (Color){   0, 255,   0, 191 };
        } else
        if (fps >= 45.0) {  // Fluid enough for some complicate 2D game.
            color = (Color){ 255, 255,   0, 191 };
        } else
        if (fps >= 30.0) {  // Enough for a simple 2D game.
            color = (Color){ 255, 127,   0, 191 };
        } else {            // Bad, very bad...
            color = (Color){ 255,   0,   0, 191 };
        }
        DrawLine(i, FPS_HISTOGRAM_HEIGHT - height, i, FPS_HISTOGRAM_HEIGHT, color);
    }

    const char *text = FormatText("%.0f FPS (%.0f - %.0f)", statistics->current_fps, statistics->min_fps, statistics->max_fps);
    int width = MeasureText(text, FPS_TEXT_HEIGHT);
    DrawText(text, (STATISTICS_LENGTH - width) / 2, FPS_HISTOGRAM_HEIGHT, FPS_TEXT_HEIGHT, (Color){ 0, 255, 0, 191 });
}

bool Display_initialize(Display_t *display, const Display_Configuration_t *configuration, const char *title)
{
    display->configuration = *configuration;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        Log_write(LOG_LEVELS_FATAL, "<DISPLAY> can't initialize GLFW");
        return false;
    }

    int display_width, display_height;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), NULL, NULL, &display_width, &display_height);

    display->window_width = configuration->width;
    display->window_height = configuration->height;
    display->window_scale = 1;

    if (configuration->autofit) {
        Log_write(LOG_LEVELS_DEBUG, "<DISPLAY> display size is %d x %d", display_width, display_height);

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

    Log_write(LOG_LEVELS_DEBUG, "<DISPLAY> window size is %d x %d (%dx)", display->window_width, display->window_height,
        display->window_scale);

    int x = (display_width - display->window_width) / 2;
    int y = (display_height - display->window_height) / 2;

    if (configuration->hide_cursor) {
        // HideCursor();
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    display->window = glfwCreateWindow(display->window_width, display->window_height, title, glfwGetPrimaryMonitor(), NULL);
    if (display->window == NULL) {
        Log_write(LOG_LEVELS_FATAL, "<DISPLAY> can't create window");
        glfwTerminate();
        return false;
    }

    glfwSetKeyCallback(display->window, configuration->exit_key_enabled ? key_callback : NULL);

#ifdef __FAST_FULLSCREEN__
    SetWindowPosition(x, y); // This will enforce a "clipping region" for the fullscreen, clear won't be needed.
    SetWindowSize(display->window_width, display->window_height);
#else
    if (configuration->fullscreen) {
        SetWindowPosition(0, 0);
        SetWindowSize(display_width, display_height);
    } else {
        SetWindowPosition(x, y);
        SetWindowSize(display->window_width, display->window_height);
    }
#endif
    UnhideWindow();
    if (configuration->fullscreen) {
        ToggleFullscreen();
    }

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

    Palette_t palette; // Initial gray-scale palette.
    for (size_t i = 0; i < MAX_PALETTE_COLORS; ++i) {
        unsigned char v = (unsigned char)(((double)i / (double)(MAX_PALETTE_COLORS - 1)) * 255.0);
        palette.colors[i] = (Color){ v, v, v, 255 };
    }
    palette.count = MAX_PALETTE_COLORS;
    Display_palette(display, &palette);

    return true;
}

bool Display_shouldClose(Display_t *display)
{
    return WindowShouldClose();
}

void Display_renderBegin(Display_t *display)
{
    BeginTextureMode(display->framebuffers[0]);
        ClearBackground((Color){ 0, 0, 0, 255 }); // Required, to clear previous content. (TODO: configurable color?)
}

void Display_renderEnd(Display_t *display, double now, const Engine_Statistics_t *statistics)
{
    EndTextureMode();

    Rectangle os = display->offscreen_source;
    size_t source = 0, target = 1;
    for (size_t i = 0; i < SHADERS_COUNT; ++i) { // Post-processing filters chain. Shader #0 is the palette shader.
        if (display->shaders[i].id == 0) {
            break;
        }

        int uniform_location = GetShaderLocation(display->shaders[i], "time"); // Send current time to shader.
        if (uniform_location != -1) {
            GLfloat time = (float)now;
            SetShaderValue(display->shaders[i], uniform_location, &time, UNIFORM_FLOAT);
        }

        BeginTextureMode(display->framebuffers[target]);
            BeginShaderMode(display->shaders[i]);
                DrawTexture(display->framebuffers[source].texture, 0, 0, (Color){ 255, 255, 255, 255 });
            EndShaderMode();
        EndTextureMode();

        size_t aux = target;
        target = source;
        source = aux;

        os.height *= -1.0; // Y-flip on even number of shaders.
    }

    BeginDrawing();
#ifndef __FAST_FULLSCREEN__
        ClearBackground((Color){ 0, 0, 0, 255 });
#endif
        DrawTexturePro(display->framebuffers[source].texture, os, display->offscreen_destination,
            display->offscreen_origin, 0.0f, (Color){ 255, 255, 255, 255 });

        if (statistics) {
            draw_statistics(statistics);
        }
    EndDrawing();
}

void Display_palette(Display_t *display, const Palette_t *palette)
{
    GLfloat colors[MAX_PALETTE_COLORS * VALUES_PER_COLOR] = {};
    for (size_t i = 0; i < palette->count; ++i) {
        int j = i * VALUES_PER_COLOR;
        colors[j    ] = (GLfloat)palette->colors[i].r / (GLfloat)255.0;
        colors[j + 1] = (GLfloat)palette->colors[i].g / (GLfloat)255.0;
        colors[j + 2] = (GLfloat)palette->colors[i].b / (GLfloat)255.0;
    }
    display->palette = *palette;
    int uniform_location = GetShaderLocation(display->shaders[SHADER_INDEX_PALETTE], "palette");
    if (uniform_location == -1) {
        return;
    }
    SetShaderValueV(display->shaders[SHADER_INDEX_PALETTE], uniform_location, colors, UNIFORM_VEC3, MAX_PALETTE_COLORS);
}

void Display_shader(Display_t *display, size_t index, const char *code)
{
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
}

void Display_terminate(Display_t *display)
{
    for (size_t i = 0; i < SHADERS_COUNT; ++i) {
        if (display->shaders[i].id == 0) {
            continue;
        }
        UnloadShader(display->shaders[i]);
    }
    for (size_t i = 0; i < FRAMEBUFFERS_COUNT; ++i) {
        UnloadRenderTexture(display->framebuffers[i]);
    }
    CloseWindow();
}