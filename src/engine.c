#include "engine.h"
#include "configuration.h"
#include "file.h"
#include "log.h"

#include <raylib/raylib.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CONFIGURATION_FILE_NAME    "configuration.json"

#define MAX_FPS_SAMPLES     256

static double update_fps(const double elapsed) {
    static double history[MAX_FPS_SAMPLES] = {};
    static int index = 0;
    static double sum = 0.0;

    sum -= history[index];
    sum += elapsed;
    history[index] = elapsed;
    index = (index + 1) % MAX_FPS_SAMPLES;
    return (double)MAX_FPS_SAMPLES / sum;
}

bool Engine_initialize(Engine_t *engine, const char *base_path)
{
    char filename[PATH_FILE_MAX];
    strcpy(filename, base_path);
    strcat(filename, CONFIGURATION_FILE_NAME);

    Log_initialize();

    Configuration_initialize(&engine->configuration);
    Configuration_load(&engine->configuration, filename);

    Log_configure(engine->configuration.debug);

    Display_Configuration_t display_configuration = {
            .width = engine->configuration.width,
            .height = engine->configuration.height,
            .colors = engine->configuration.debug,
            .fullscreen = engine->configuration.fullscreen,
            .autofit = engine->configuration.autofit,
            .hide_cursor = engine->configuration.hide_cursor,
            .display_fps = engine->configuration.debug
        };
    bool result = Display_initialize(&engine->display, &display_configuration, engine->configuration.title);
    if (!result) {
        Log_write(LOG_LEVELS_ERROR, "Can't initialize display!");
        return false;
    }

    Environment_initialize(&engine->environment, base_path, engine->configuration.width, engine->configuration.height); // TODO> add environment configuration.

    result = Interpreter_initialize(&engine->interpreter, &engine->environment);
    if (!result) {
        Log_write(LOG_LEVELS_ERROR, "Can't initialize interpreter!");
        Display_terminate(&engine->display);
        return false;
    }

    SetExitKey(engine->configuration.exit_key_enabled ? KEY_ESCAPE : -1); // TODO: move to display.

    return true;
}

void Engine_terminate(Engine_t *engine)
{
    Interpreter_terminate(&engine->interpreter);
    Environment_terminate(&engine->environment);
    Display_terminate(&engine->display);
}

bool Engine_isRunning(Engine_t *engine)
{
    return !engine->environment.should_close && !Display_shouldClose(&engine->display);
}

void Engine_run(Engine_t *engine)
{
    const double delta_time = 1.0 / (double)engine->configuration.fps;
    const int skippable_frames = engine->configuration.skippable_frames;
    Log_write(LOG_LEVELS_INFO, "Engine is nor running, delta-time is %.3fs w/ %d skippable frames", delta_time, skippable_frames);

    double previous = GetTime();
    double lag = 0.0;

    while (Engine_isRunning(engine)) {
        double current = GetTime();
        double elapsed = current - previous;
        previous = current;

        double fps = update_fps(elapsed); // Calculate frame statistics.

        Interpreter_input(&engine->interpreter);

        lag += elapsed; // Count a maximum amount of skippable frames in order no to stall on slower machines.
        for (int frames = 0; (frames < skippable_frames) && (lag >= delta_time); ++frames) {
            Interpreter_update(&engine->interpreter, delta_time);
            lag -= delta_time;
        }

        Display_renderBegin(&engine->display, NULL);
            Interpreter_render(&engine->interpreter, lag / delta_time);
        Display_renderEnd(&engine->display, NULL, fps, delta_time);
    }
}
