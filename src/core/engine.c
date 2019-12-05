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

#include <config.h>
#include <core/configuration.h>
#include <core/platform.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#if PLATFORM_ID == PLATFORM_LINUX
  #include <unistd.h>
#elif PLATFORM_ID == PLATFORM_WINDOWS
  #include <windows.h>
#endif

static inline void wait_for(float seconds)
{
#if PLATFORM_ID == PLATFORM_LINUX
    int nanos = (int)(seconds * 1000000.0f);
    if (nanos == 0) {
        sched_yield();
    } else {
        usleep(nanos); // usleep takes sleep time in us (1 millionth of a second)
    }
#elif PLATFORM_ID == PLATFORM_WINDOWS
    int millis = (int)(seconds * 1000.0f);
    if (millis == 0) {
        YieldProcessor();
     } else {
        Sleep(millis);
    }
#else
    int nanos = (int)(seconds * 1000000.0f);
    struct timespec ts;
    ts.tv_sec = nanos / 1000000;
    ts.tv_nsec = nanos;
    nanosleep(&ts, NULL);
#endif
}

static inline size_t calculate_fps(float elapsed)
{
    static float samples[FPS_AVERAGE_SAMPLES] = { 0 };
    static size_t index = 0;
    static double sum = 0.0; // Need to store the game life...

    sum -= samples[index];
    sum += elapsed;
    samples[index] = elapsed;
    index = (index + 1) % FPS_AVERAGE_SAMPLES;

    return (size_t)round((double)FPS_AVERAGE_SAMPLES / sum);
}

bool Engine_initialize(Engine_t *engine, const char *base_path)
{
    *engine = (Engine_t){ 0 }; // Ensure is cleared at first.

    Log_initialize();
    Environment_initialize(&engine->environment);

    // The interpreter is the first to be loaded, since it also manages the configuration. Later on, we will call to
    // initialization function once the sub-systems are ready.
    const void *userdatas[] = {
            &engine->interpreter,
            &engine->environment,
            &engine->display,
            &engine->input,
            NULL
        };
    bool result = Interpreter_initialize(&engine->interpreter, base_path, &engine->configuration, userdatas);
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, "<ENGINE> can't initialize interpreter");
        return false;
    }

    Log_configure(engine->configuration.debug);

    Display_Configuration_t display_configuration = { // TODO: reorganize configuration.
            .title = engine->configuration.title,
            .width = engine->configuration.width,
            .height = engine->configuration.height,
            .fullscreen = engine->configuration.fullscreen,
            .vertical_sync = engine->configuration.vertical_sync,
            .scale = engine->configuration.scale,
            .hide_cursor = engine->configuration.hide_cursor
        };
    result = Display_initialize(&engine->display, &display_configuration);
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, "<ENGINE> can't initialize display");
        Interpreter_terminate(&engine->interpreter);
        return false;
    }

    Input_Configuration_t input_configuration = {
            .exit_key_enabled = engine->configuration.exit_key_enabled,
        };
    result = Input_initialize(&engine->input, &input_configuration, engine->display.window);
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, "<ENGINE> can't initialize input");
        Interpreter_terminate(&engine->interpreter);
        Display_terminate(&engine->display);
        return false;
    }

    result = Audio_initialize(&engine->audio, &(Audio_Configuration_t){ .channels = 2, .sample_rate = 44100, .voices = 8 });
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, "<ENGINE> can't initialize audio");
        Interpreter_terminate(&engine->interpreter);
        Display_terminate(&engine->display);
        Input_terminate(&engine->input);
        return false;
    }

    return true;
}

void Engine_terminate(Engine_t *engine)
{
    Interpreter_terminate(&engine->interpreter); // Terminate the interpreter to unlock all resources.
    Display_terminate(&engine->display);
    Input_terminate(&engine->input);
    Audio_terminate(&engine->audio);

    Environment_terminate(&engine->environment);
#if DEBUG
    stb_leakcheck_dumpmem();
#endif
}

void Engine_run(Engine_t *engine)
{
    const float delta_time = 1.0f / (float)engine->configuration.fps;
    const size_t skippable_frames = engine->configuration.skippable_frames;
    const float reference_time = 1.0f / engine->configuration.fps_cap;
    Log_write(LOG_LEVELS_INFO, "<ENGINE> now running, update-time is %.6fs w/ %d skippable frames, reference-time is %.6fs", delta_time, skippable_frames, reference_time);

    // Track time using double to keep the min resolution consistent over time!
    // https://randomascii.wordpress.com/2012/02/13/dont-store-that-in-a-float/
    double previous = glfwGetTime();
    float lag = 0.0f;

    // https://nkga.github.io/post/frame-pacing-analysis-of-the-game-loop/
    for (bool running = true; running && !engine->environment.quit && !Display_should_close(&engine->display); ) {
        const double current = glfwGetTime();
        const float elapsed = (float)(current - previous);
        previous = current;

        engine->environment.fps = calculate_fps(elapsed);
#ifdef __DEBUG_ENGINE_FPS__
        static size_t count = 0;
        if (++count == 250) {
            Log_write(LOG_LEVELS_INFO, "<ENGINE> currently running at %.0f FPS", engine->environment.fps);
            count = 0;
        }
#endif

        Input_process(&engine->input);
        running = running && Interpreter_input(&engine->interpreter); // Lazy evaluate `running`, will avoid calls when error.

        lag += elapsed; // Count a maximum amount of skippable frames in order no to stall on slower machines.
        for (size_t frames = 0; (frames < skippable_frames) && (lag >= delta_time); ++frames) {
            engine->environment.time += delta_time;
            running = running && Interpreter_update(&engine->interpreter, delta_time); // Fixed update.
            lag -= delta_time;
        }

//        running = running && Interpreter_update_variable(&engine->interpreter, elapsed); // Variable update.
        Audio_update(&engine->audio, elapsed); // Update the subsystems w/ regard to the variable time.
        Input_update(&engine->input, elapsed);
        Display_update(&engine->display, elapsed);

        running = running && Interpreter_render(&engine->interpreter, lag / delta_time);
        Display_present(&engine->display);

        const float frame_time = (float)(glfwGetTime() - current);
        const float leftover = reference_time - frame_time;
        if (leftover > 0.0f) {
            wait_for(leftover);
        }
    }
}
