#include "engine.h"
#include "configuration.h"
#include "log.h"

#include <raylib/raylib.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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
    strcpy(engine->base_path, base_path);

    char filename[PATH_MAX];
    strcpy(filename, base_path);
    strcat(filename, "configuration.json");

    Log_initialize();

    Configuration_initialize(&engine->configuration);
    Configuration_load(&engine->configuration, filename);

    Log_configure(engine->configuration.debug);

    Interpreter_Config_t interpreter_configuration;
    interpreter_configuration.base_path = engine->base_path;
    Interpreter_initialize(&engine->interpreter, &interpreter_configuration);

    Display_Configuration_t display_configuration;
    display_configuration.width = engine->configuration.width;
    display_configuration.height = engine->configuration.height;
    display_configuration.colors = engine->configuration.debug;
    display_configuration.hide_cursor = engine->configuration.hide_cursor;
    display_configuration.display_fps = engine->configuration.debug;
    Display_initialize(&engine->display, &display_configuration, engine->configuration.title);

    return true;
}

void Engine_terminate(Engine_t *engine)
{
    Display_terminate(&engine->display);
    Interpreter_terminate(&engine->interpreter);
}

void Engine_run(Engine_t *engine)
{   
    Log_write(LOG_LEVELS_INFO, "Engine is now running");

    const double seconds_per_update = 1.0 / (double)engine->configuration.fps;
    Log_write(LOG_LEVELS_DEBUG, "Engine update timeslice is %.3fs", seconds_per_update);

    double previous = GetTime();
    double lag = 0.0;

    while (!Display_shouldClose(&engine->display)) {
        double current = GetTime();
        double elapsed = current - previous;
        previous = current;
        lag += elapsed;

        double fps = update_fps(elapsed);

        Interpreter_handle(&engine->interpreter);

        while (lag >= seconds_per_update) {
            Interpreter_update(&engine->interpreter, seconds_per_update);
            lag -= seconds_per_update;
        }

        Display_renderBegin(&engine->display, NULL);
            Interpreter_render(&engine->interpreter, lag / seconds_per_update);
        Display_renderEnd(&engine->display, NULL, fps, seconds_per_update);
    }
}
