#include <limits.h>
#include <stdlib.h>

#include <chaiscript/chaiscript.hpp>

namespace raylib {
    #include <raylib/raylib.h>
}

#include "Engine.hpp"

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240
#define WINDOW_TITLE    ".: ZEST :."

typedef struct _Screen_t {
    int width;
    int height;
    int scale;
} Screen_t;

static Screen_t InitializeWindow(const int width, const int height)
{
    raylib::SetConfigFlags(raylib::FLAG_WINDOW_HIDDEN);
    raylib::InitWindow(0, 0, WINDOW_TITLE);

    int displayWidth = raylib::GetScreenWidth();
    int displayHeight = raylib::GetScreenHeight();
    raylib::TraceLog(raylib::LOG_DEBUG, "Display size is %d x %d", displayWidth, displayHeight);

    Screen_t screen = {};
    screen.width = width;
    screen.height = height;
    screen.scale = 1;

    for (int s = 1; ; ++s) {
        int w = width * s;
        int h = height * s;
        if ((w > displayWidth) || (h > displayHeight)) {
            break;
        }
        screen.scale = s;
        screen.width = w;
        screen.height = h;
    }

    raylib::TraceLog(raylib::LOG_DEBUG, "Window size is %d x %d (%dx)", screen.width, screen.height, screen.scale);

    int x = (displayWidth - screen.width) / 2;
    int y = (displayHeight - screen.height) / 2;

    raylib::SetWindowPosition(x, y);
    raylib::SetWindowSize(screen.width, screen.height);
    raylib::ShowWindow();

//    raylib::SetTargetFPS(60);

    return screen;
}

void chai_log(const std::string &message)
{
    raylib::TraceLog(raylib::LOG_DEBUG, message.c_str());
}

static void split_path(const std::string &path, std::string &parent, std::string &child)
{
#ifdef __linux__
    const char PATH_SEPARATOR_CHAR = '/';
    const char *PATH_SEPARATOR_STRING = "/";
#elif _WIN32
    const char PATH_SEPARATOR_CHAR = '\\';
    const char *PATH_SEPARATOR_STRING = "\\";
#endif
    std::string::size_type i = path.rfind(PATH_SEPARATOR_CHAR);
    if (i != std::string::npos) {
        parent = path.substr(0, i);
        child = path.substr(i + 1);
    } else {
        parent = "./";
        child = path;
    }

    char *absolute_path = realpath(parent.c_str(), nullptr);
    parent.assign(absolute_path);
    parent.append(PATH_SEPARATOR_STRING); // ChaiScript need the paths to end with the separator!
    free(absolute_path);

    raylib::TraceLog(raylib::LOG_DEBUG, "path '%s' for file '%s'", parent.c_str(), child.c_str());
}

int main(int argc, char **argv)
{
    // TODO: query main script for configuration (width, height, fps, title, etc...).
    raylib::SetTraceLog(raylib::LOG_DEBUG | raylib::LOG_INFO | raylib::LOG_WARNING);

    Screen_t screen = InitializeWindow(SCREEN_WIDTH, SCREEN_HEIGHT);

    std::string script = "./boot.chai";
    if (argc > 1) {
        script = argv[1];
    }

    std::vector<std::string> modulepaths;
    std::vector<std::string> usepaths;
    std::string parent, child;
    split_path(script, parent, child);
    modulepaths.push_back(parent);
    usepaths.push_back(parent);

    chaiscript::ChaiScript chai(modulepaths, usepaths);
    chai.add(chaiscript::fun(&chai_log), "log");
    chai.eval_file(script);

    raylib::RenderTexture2D offscreen = raylib::LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    raylib::SetTextureFilter(offscreen.texture, raylib::FILTER_POINT); // Nearest-neighbour scaling.

    while (!raylib::WindowShouldClose()) {
        // TODO: Update your variables here
        float dt = raylib::GetFrameTime();

        raylib::BeginTextureMode(offscreen);
            raylib::ClearBackground(BLACK);
            raylib::DrawFPS(0, 0);
            raylib::DrawText(raylib::FormatText("FRAME-TIME: %.3f", dt), 16, 16, 8, LIGHTGRAY);
        raylib::EndTextureMode();

        raylib::BeginDrawing();
            raylib::DrawTexturePro(offscreen.texture,
            (raylib::Rectangle){ 0.0f, 0.0f, (float)offscreen.texture.width, (float)-offscreen.texture.height }, // Y-flip the texture.
            (raylib::Rectangle){ 0.0f, 0.0f, (float)screen.width, (float)screen.height },
            (raylib::Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
        raylib::EndDrawing();
    }

    raylib::UnloadRenderTexture(offscreen);

    raylib::CloseWindow();

    return EXIT_SUCCESS;
}