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

#include "log.h"

#include <memory.h>

#define UNCAPPED_FPS        0
#define FAST_FULLSCREEN     1

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

#ifdef FAST_FULLSCREEN
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

    return true;
}

bool Display_shouldClose(Display_t *display)
{
    return WindowShouldClose();
}

void Display_renderBegin(Display_t *display, void callback(void))
{
    BeginTextureMode(display->offscreen);
        ClearBackground(BLACK);
        if (callback) {
            callback();
        }
        // BeginShaderMode()
}

void Display_renderEnd(Display_t *display, void callback(void), const double fps, const double delta_time)
{
        if (display->configuration.display_fps) {
            const char *text = FormatText("%.0f FPS (%.3fs)", fps, delta_time);
            int width = MeasureText(text, 10);
            DrawRectangle(0, 0, width, 10, (Color){ 0, 0, 0, 128 });
            DrawText(text, 0, 0, 10, (Color){ 255, 255, 255, 128 });
        }
    EndTextureMode();

    BeginDrawing();
#ifndef FAST_FULLSCREEN
        ClearBackground(BLACK);
#endif
        if (callback) {
            callback();
        }
        DrawTexturePro(display->offscreen.texture, display->offscreen_source, display->offscreen_destination,
            display->offscreen_origin, 0.0f, WHITE);
    EndDrawing();
}

void Display_terminate(Display_t *display)
{
    UnloadRenderTexture(display->offscreen);
    CloseWindow();
}