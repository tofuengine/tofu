#include <chaiscript/chaiscript.hpp>

namespace raylib {
    #include <raylib/raylib.h>
}

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240
#define WINDOW_TITLE    ".: MODE 13h - 2D ENGINE :."

typedef struct _Screen_t {
    int width;
    int height;
    int scale;
} Screen_t;

static Screen_t FitToDisplay(const int width, const int height)
{
    int screenWidth = raylib::GetScreenWidth();
    int screenHeight = raylib::GetScreenHeight();
    raylib::TraceLog(raylib::LOG_DEBUG, "Screen size is %dx%d", screenWidth, screenHeight);

    Screen_t screen = {};
    screen.width = width;
    screen.height = height;
    screen.scale = 1;

    for (int s = 1; ; ++s) {
        int w = width * s;
        int h = height * s;
        if ((w > screenWidth) || (h > screenHeight)) {
            break;
        }
        screen.scale = s;
        screen.width = w;
        screen.height = h;
    }

    int x = (screenWidth - screen.width) / 2;
    int y = (screenHeight - screen.height) / 2;
    raylib::SetWindowPosition(x, y);
    raylib::SetWindowSize(screen.width, screen.height);

    raylib::TraceLog(raylib::LOG_DEBUG, "Window size is %dx%d (%dx)", screen.width, screen.height, screen.scale);
    return screen;
}

int main(int argc, char **argv)
{
    // TODO: query main script for configuration (width, height, fps, title, etc...).
    raylib::SetTraceLog(raylib::LOG_DEBUG | raylib::LOG_INFO | raylib::LOG_WARNING);
//    raylib::ShowLogo();
    raylib::InitWindow(0, 0, WINDOW_TITLE); // Mostly a hack. Initialize to screen size, move offscreen.
    raylib::SetWindowPosition(9999, 9999);

    Screen_t screen = FitToDisplay(SCREEN_WIDTH, SCREEN_HEIGHT);

    raylib::SetTargetFPS(60);

    while (!raylib::WindowShouldClose()) {
        // TODO: Update your variables here
        float dt = raylib::GetFrameTime();

        raylib::BeginDrawing();
        raylib::ClearBackground(RAYWHITE);
        raylib::DrawFPS(0, 0);
        raylib::DrawText(raylib::FormatText("%.3f", dt), 0, 32, 20, LIGHTGRAY);
        raylib::EndDrawing();
    }

    raylib::CloseWindow();

    return EXIT_SUCCESS;
}