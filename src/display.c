#include "display.h"

#include "log.h"

#include <memory.h>

#define UNCAPPED_FPS        0

void Display_initialize(Display_t *display, const Display_Configuration_t *configuration, const char *title)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(0, 0, title);

    memcpy(&display->configuration, configuration, sizeof(Display_Configuration_t));

    display->window_width = configuration->width;
    display->window_height = configuration->height;
    display->window_scale = 1;

    int display_width = GetScreenWidth();
    int display_height = GetScreenHeight();
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

    Log_write(LOG_LEVELS_DEBUG, "Window size is %d x %d (%dx)", display->window_width, display->window_height, display->window_scale);

    int x = (display_width - display->window_width) / 2;
    int y = (display_height - display->window_height) / 2;

    if (configuration->hide_cursor) {
        HideCursor();
    }

    SetWindowPosition(x, y);
    SetWindowSize(display->window_width, display->window_height);
    ShowWindow();

    SetTargetFPS(UNCAPPED_FPS);

    display->offscreen = LoadRenderTexture(configuration->width, configuration->height);
    SetTextureFilter(display->offscreen.texture, FILTER_POINT); // Nearest-neighbour scaling.

    display->a = (Rectangle){ 0.0f, 0.0f, (float)display->offscreen.texture.width, (float)-display->offscreen.texture.height };
    display->b = (Rectangle){ 0.0f, 0.0f, (float)display->window_width, (float)display->window_height };
    display->c = (Vector2){ 0.0f, 0.0f };
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
#if 0
        DrawTexturePro(display->offscreen.texture,
            (Rectangle){ 0.0f, 0.0f, (float)display->offscreen.texture.width, (float)-display->offscreen.texture.height }, // Y-flip the texture.
            (Rectangle){ 0.0f, 0.0f, (float)display->window_width, (float)display->window_height },
            (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
#else
        DrawTexturePro(display->offscreen.texture, display->a, display->b, display->c, 0.0f, WHITE);
#endif
        if (display->configuration.display_fps) {
            DrawText(FormatText("[ %.0f fps | %.3fs ]", fps, delta_time), 0, 0, 10, (Color){ 255, 255, 255, 128 });
        }
    EndDrawing();
}

void Display_terminate(Display_t *display)
{
    UnloadRenderTexture(display->offscreen);
    CloseWindow();
}