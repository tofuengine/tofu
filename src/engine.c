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

#include "engine.h"

#include "config.h"
#include "configuration.h"
#include "file.h"
#include "log.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CONFIGURATION_FILE_NAME    "configuration.json"

static bool update_statistics(Engine_Statistics_t *statistics, double elapsed) {
    static double history[FPS_AVERAGE_SAMPLES] = {};
    static int index = 0;
    static int samples = 0;
    static double sum = 0.0;
    static int count = 0;

    sum -= history[index];
    sum += elapsed;
    history[index] = elapsed;
    index = (index + 1) % FPS_AVERAGE_SAMPLES;
    if (samples < FPS_AVERAGE_SAMPLES) {
        samples += 1;
        return false;
    }
    double fps = (double)FPS_AVERAGE_SAMPLES / sum;
    if (statistics->min_fps > fps) {
        statistics->min_fps = fps;
    }
    if (statistics->max_fps < fps) {
        statistics->max_fps = fps;
    }
    statistics->current_fps = fps;
    if (count == 0) {
        statistics->history[statistics->index] = fps;
        statistics->index = (statistics->index + 1) % STATISTICS_LENGTH;
    }
    count = (count + 1) % FPS_STATISTICS_RESOLUTION;
    return true;
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
            .exit_key_enabled = engine->configuration.exit_key_enabled,
        };
    bool result = Display_initialize(&engine->display, &display_configuration, engine->configuration.title);
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, "<ENGINE> can't initialize display");
        return false;
    }

    Environment_initialize(&engine->environment, base_path, &engine->display); // TODO> add environment configuration.

    engine->environment.timer_pool = &engine->interpreter.timer_pool; // HACK: inject the timer-pool pointer.

    result = Interpreter_initialize(&engine->interpreter, &engine->environment);
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, "<ENGINE> can't initialize interpreter");
        Display_terminate(&engine->display);
        return false;
    }

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
    Log_write(LOG_LEVELS_INFO, "<ENGINE> now running, delta-time is %.3fs w/ %d skippable frames", delta_time, skippable_frames);

    Engine_Statistics_t statistics = (Engine_Statistics_t){
            .delta_time = delta_time,
            .min_fps = __DBL_MAX__,
            .max_fps = 0.0
        };

    double previous = glfwGetTime();
    double lag = 0.0;

    while (Engine_isRunning(engine)) {
        double current = glfwGetTime();
        double elapsed = current - previous;
        previous = current;

        const Engine_Statistics_t *current_statistics = NULL;
        if (engine->configuration.debug) {
            bool ready = update_statistics(&statistics, elapsed);
            current_statistics = ready ? &statistics : NULL;
        }

        Display_processInput(&engine->display);
        Interpreter_input(&engine->interpreter);

        lag += elapsed; // Count a maximum amount of skippable frames in order no to stall on slower machines.
        for (int frames = 0; (frames < skippable_frames) && (lag >= delta_time); ++frames) {
            // TODO: move `TimerPool_update()` here?
            // TODO: Should the `Tofu` class be hardcoded?
            Interpreter_update(&engine->interpreter, delta_time);
            lag -= delta_time;
        }

        Display_renderBegin(&engine->display);
            Interpreter_render(&engine->interpreter, lag / delta_time);
        Display_renderEnd(&engine->display, current, current_statistics);
    }
}
