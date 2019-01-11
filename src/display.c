#include "display.h"

void Display_initialize(Display_t *display, const int width, const int height, const char *title)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(0, 0, title);

    display->width = width;
    display->height = height;
    display->window_width = width;
    display->window_height = height;
    display->window_scale = 1;

    int displayWidth = GetScreenWidth();
    int displayHeight = GetScreenHeight();
    TraceLog(LOG_DEBUG, "Display size is %d x %d", displayWidth, displayHeight);

    for (int s = 1; ; ++s) {
        int w = width * s;
        int h = height * s;
        if ((w > displayWidth) || (h > displayHeight)) {
            break;
        }
        display->window_width = w;
        display->window_height = h;
        display->window_scale = s;
    }

    TraceLog(LOG_DEBUG, "Window size is %d x %d (%dx)", display->window_width, display->window_height, display->window_scale);

    int x = (displayWidth - display->window_width) / 2;
    int y = (displayHeight - display->window_height) / 2;

    SetWindowPosition(x, y);
    SetWindowSize(display->window_width, display->window_height);
    ShowWindow();

    SetTargetFPS(60);

    display->offscreen = LoadRenderTexture(width, height);
    SetTextureFilter(display->offscreen.texture, FILTER_POINT); // Nearest-neighbour scaling.
}

bool Display_shouldClose()
{
    return WindowShouldClose();
}

void Display_render(Display_t *display, void callback(void))
{
    float dt = GetFrameTime();

    BeginTextureMode(display->offscreen);
        ClearBackground(BLACK);
//        callback();
        DrawFPS(0, 0);
        DrawText(FormatText("FRAME-TIME: %.3f", dt), 16, 16, 8, LIGHTGRAY);
    EndTextureMode();

    BeginDrawing();
        DrawTexturePro(display->offscreen.texture,
        (Rectangle){ 0.0f, 0.0f, (float)display->offscreen.texture.width, (float)-display->offscreen.texture.height }, // Y-flip the texture.
        (Rectangle){ 0.0f, 0.0f, (float)display->window_width, (float)display->window_height },
        (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    EndDrawing();
}

void Display_terminate(Display_t *display)
{
    UnloadRenderTexture(display->offscreen);
    CloseWindow();
}