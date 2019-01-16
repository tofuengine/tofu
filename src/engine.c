#include "engine.h"
#include "configuration.h"
#include "log.h"

#include <raylib/raylib.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_FPS_SAMPLES     128

static double get_time()
{
//    return (double)clock() / (double)CLOCKS_PER_SEC;
    return GetTime(); // Uses RayLib.
}

static double update_fps(const double elapsed) {
    static double ticks[MAX_FPS_SAMPLES] = {};
    static int tick_count = 0;
    static int tick_index = 0;
    static double tick_sum = 0.0;

    tick_sum -= ticks[tick_index];
    tick_sum += elapsed;
    ticks[tick_index] = elapsed;
    if (++tick_index == MAX_FPS_SAMPLES) {
        tick_index = 0;
    }
    if (tick_count < MAX_FPS_SAMPLES) {
        tick_count++;
    }
    return (double)tick_count / tick_sum;
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

    Interpreter_Config_t configuration;
    configuration.base_path = engine->base_path;
    Interpreter_initialize(&engine->interpreter, &configuration);

    Display_initialize(&engine->display, engine->configuration.width, engine->configuration.height, engine->configuration.title, engine->configuration.hide_cursor);

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

    double previous = get_time();
    double lag = 0.0;

    while (!Display_shouldClose(&engine->display)) {
        double current = get_time();
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
