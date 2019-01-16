#include "display.h"

#include "log.h"

#define UNCAPPED_FPS        0

void Display_initialize(Display_t *display, const int width, const int height, const char *title, bool hide_cursor)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(0, 0, title);

    display->display_fps = true;
    display->width = width;
    display->height = height;
    display->window_width = width;
    display->window_height = height;
    display->window_scale = 1;

    int displayWidth = GetScreenWidth();
    int displayHeight = GetScreenHeight();
    Log_write(LOG_LEVELS_DEBUG, "Display size is %d x %d", displayWidth, displayHeight);

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

    Log_write(LOG_LEVELS_DEBUG, "Window size is %d x %d (%dx)", display->window_width, display->window_height, display->window_scale);

    int x = (displayWidth - display->window_width) / 2;
    int y = (displayHeight - display->window_height) / 2;

    if (hide_cursor) {
        HideCursor();
    }

    SetWindowPosition(x, y);
    SetWindowSize(display->window_width, display->window_height);
    ShowWindow();

    SetTargetFPS(UNCAPPED_FPS);

    display->offscreen = LoadRenderTexture(width, height);
    SetTextureFilter(display->offscreen.texture, FILTER_POINT); // Nearest-neighbour scaling.
}

bool Display_shouldClose(Display_t *display)
{
    return WindowShouldClose();
}

void Display_renderBegin(Display_t *display, void callback(void))
{
    BeginTextureMode(display->offscreen);
        ClearBackground(BLACK);
        // callback();
    EndTextureMode();
}

void Display_renderEnd(Display_t *display, void callback(void), const double fps, const double delta_time)
{
    BeginDrawing();
        // callback();
        DrawTexturePro(display->offscreen.texture,
        (Rectangle){ 0.0f, 0.0f, (float)display->offscreen.texture.width, (float)-display->offscreen.texture.height }, // Y-flip the texture.
        (Rectangle){ 0.0f, 0.0f, (float)display->window_width, (float)display->window_height },
        (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

        if (display->display_fps) {
            DrawText(FormatText("[ %.0f fps | %.3fs ]", fps, delta_time), 0, 0, 10, (Color){ 255, 255, 255, 128 });
        }
    EndDrawing();
}

void Display_terminate(Display_t *display)
{
    UnloadRenderTexture(display->offscreen);
    CloseWindow();
}