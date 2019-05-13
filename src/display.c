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

#define UNCAPPED_FPS                0

#define FPS_HISTOGRAM_HEIGHT        30
#define FPS_TEXT_HEIGHT             10
#define FPS_MAX_VALUE               90

static const char *palette_shader_code = 
    "#version 330\n"
    "\n"
    "const int colors = 64;\n"
    "\n"
    "// Input fragment attributes (from fragment shader)\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "\n"
    "// Input uniform values\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 palette[colors];\n"
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
    "    int index = int(texelColor.r * 255.0);\n"
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

    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(0, 0, title); // Initially fit the screen-size automatically.

    int display_width = GetScreenWidth();
    int display_height = GetScreenHeight();

    display->window_width = configuration->width;
    display->window_height = configuration->height;
    display->window_scale = 1;

    if (configuration->autofit) {
        Log_write(LOG_LEVELS_DEBUG, "Display size is %d x %d", display_width, display_height);

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

    Log_write(LOG_LEVELS_DEBUG, "Window size is %d x %d (%dx)", display->window_width, display->window_height,
        display->window_scale);

    int x = (display_width - display->window_width) / 2;
    int y = (display_height - display->window_height) / 2;

    if (configuration->hide_cursor) {
        HideCursor();
    }

    SetTargetFPS(UNCAPPED_FPS);

    SetExitKey(configuration->exit_key_enabled ? KEY_ESCAPE : -1);

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

    display->offscreen = LoadRenderTexture(configuration->width, configuration->height);
    SetTextureFilter(display->offscreen.texture, FILTER_POINT); // Nearest-neighbour scaling.

    display->offscreen_source = (Rectangle){ 0.0f, 0.0f, (float)display->offscreen.texture.width, (float)-display->offscreen.texture.height };
    if (configuration->fullscreen) {
        display->offscreen_destination = (Rectangle){ (float)x, (float)y, (float)display->window_width, (float)display->window_height };
    } else {
        display->offscreen_destination = (Rectangle){ (float)0, (float)0, (float)display->window_width, (float)display->window_height };
    }
    display->offscreen_origin = (Vector2){ 0.0f, 0.0f };

    display->palette_shader = LoadShaderCode(NULL, (char *)palette_shader_code);

    Palette_t palette; // Initial gray-scale palette.
    for (size_t i = 0; i < MAX_PALETTE_COLORS; ++i) {
        unsigned char v = ((float)i / (float)(MAX_PALETTE_COLORS - 1)) * 255;
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
    BeginTextureMode(display->offscreen);
        BeginShaderMode(display->palette_shader);
            ClearBackground((Color){ 0, 0, 0, 255 }); // TODO: configurable background color?
}

void Display_renderEnd(Display_t *display, const Engine_Statistics_t *statistics)
{
        EndShaderMode();
    EndTextureMode();

    BeginDrawing();
#ifndef __FAST_FULLSCREEN__
        ClearBackground((Color){ 0, 0, 0, 255 }); // TODO: configurable background color?
#endif
//        BeginShaderMode(display->palette_shader); // TODO: for real alpha, the shader should be applied to each draw-call!
            DrawTexturePro(display->offscreen.texture, display->offscreen_source, display->offscreen_destination,
                display->offscreen_origin, 0.0f, (Color){ 255, 255, 255, 255 });
//        EndShaderMode();

        if (statistics) {
            draw_statistics(statistics);
        }
    EndDrawing();
}

void Display_palette(Display_t *display, const Palette_t *palette)
{
    float colors[MAX_PALETTE_COLORS * VALUES_PER_COLOR] = {};
    for (size_t i = 0; i < palette->count; ++i) {
        int j = i * VALUES_PER_COLOR;
        colors[j    ] = (float)palette->colors[i].r / 255.0f;
        colors[j + 1] = (float)palette->colors[i].g / 255.0f;
        colors[j + 2] = (float)palette->colors[i].b / 255.0f;
        colors[j + 3] = (float)palette->colors[i].a / 255.0f;
    }
    display->palette = *palette;
    int uniform_location = GetShaderLocation(display->palette_shader, "palette");
    if (uniform_location == -1) {
        return;
    }
    SetShaderValueV(display->palette_shader, uniform_location, colors, UNIFORM_VEC4, MAX_PALETTE_COLORS);
}

void Display_terminate(Display_t *display)
{
    UnloadShader(display->palette_shader);
    UnloadRenderTexture(display->offscreen);
    CloseWindow();
}