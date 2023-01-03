/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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
 */

#include "environment.h"

#include <core/config.h>
#include <core/platform.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <malloc.h>
#if PLATFORM_ID == PLATFORM_WINDOWS
  #include <windows.h>
  #include <psapi.h>
#endif

#define LOG_CONTEXT "environment"

// TODO: http://www.ilikebigbits.com/2017_06_01_float_or_double.html

Environment_t *Environment_create(int argc, const char *argv[], const Display_t *display, const Input_t *input)
{
    Environment_t *environment = malloc(sizeof(Environment_t));
    if (!environment) {
        LOG_E(LOG_CONTEXT, "can't allocate environment");
        return NULL;
    }
    LOG_D(LOG_CONTEXT, "environment allocated");

    const char **args = NULL;
    for (int i = 1; i < argc; ++i) { // Skip executable name, i.e. argument #0.
        arrpush(args, argv[i]); // Note, we don't need to clone/copy/allocate the strings (arguments are persisted)
    }

    *environment = (Environment_t){
        .args = args,
        .display = display,
        .input = input,
        .state = (Environment_State_t){
#ifdef __DISPLAY_FOCUS_SUPPORT__
            .active = { .is = false, .was = false },
#endif
            .controllers = { .previous = -1, .current = 0 },
            .stats = { 0 },
            .time = 0.0
        },
    };

    return environment;
}

void Environment_destroy(Environment_t *environment)
{
    arrfree(environment->args);
    LOG_D(LOG_CONTEXT, "arguments freed");

    free(environment);
    LOG_D(LOG_CONTEXT, "environment freed");
}

const Environment_State_t *Environment_get_state(const Environment_t *environment)
{
    return &environment->state;
}

static inline size_t _calculate_fps(float frame_time) // FIXME: rework this as a reusable function for moving average.
{
    static float samples[FPS_AVERAGE_SAMPLES] = { 0 };
    static size_t index = 0;
    static float sum = 0.0f; // We are storing just a small time interval, float is enough...

    sum -= samples[index];
    samples[index] = frame_time;
    sum += frame_time;
    index = (index + 1) % FPS_AVERAGE_SAMPLES;

    return (size_t)((float)FPS_AVERAGE_SAMPLES / sum + 0.5f); // Fast rounding and truncation to integer.
}

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
static inline void _calculate_times(float times[4], const float deltas[4])
{
    static float samples[4][FPS_AVERAGE_SAMPLES] = { 0 };
    static size_t index = 0;
    static float sums[4] = { 0 };

    for (size_t i = 0; i < 4; ++i) {
        const float t = deltas[i] * 1000.0f;
        sums[i] -= samples[i][index];
        samples[i][index] = t;
        sums[i] += t;
        times[i] = sums[i] / (float)FPS_AVERAGE_SAMPLES;
    }
    index = (index + 1) % FPS_AVERAGE_SAMPLES;
}
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
void Environment_process(Environment_t *environment, float frame_time, const float deltas[4])
#else
void Environment_process(Environment_t *environment, float frame_time)
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */
{
    Environment_State_t *state = &environment->state;
#ifdef __DISPLAY_FOCUS_SUPPORT__
    state->active.was = state->active.is;
    state->active.is = glfwGetWindowAttrib(environment->display->window, GLFW_FOCUSED) == GLFW_TRUE;
#endif
    state->controllers.previous = state->controllers.current;
    state->controllers.current = Input_get_controllers_count(environment->input);

    Environment_Stats_t *stats = &state->stats;
    stats->fps = _calculate_fps(frame_time); // FIXME: ditch this! It's implicit in the frame time!

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
    _calculate_times(stats->times, deltas);
#ifdef __DEBUG_ENGINE_PERFORMANCES__
    static float stats_time = __ENGINE_PERFORMANCES_PERIOD__;
    stats_time += frame_time;
    while (stats_time > __ENGINE_PERFORMANCES_PERIOD__) {
        stats_time -= __ENGINE_PERFORMANCES_PERIOD__;
        LOG_I(LOG_CONTEXT, "currently running at %d FPS (P=%.3fms, U=%.3fms, R=%.3fms, F=%.3fms)",
            stats->fps, stats->times[0], stats->times[1], stats->times[2], stats->times[3]);
    }
#endif  /* __DEBUG_ENGINE_PERFORMANCES__ */
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */

#ifdef __SYSTEM_HEAP_STATISTICS__
    static float heap_time = __SYSTEM_HEAP_PERIOD__;
    heap_time += frame_time;
    while (heap_time > __SYSTEM_HEAP_PERIOD__) {
        heap_time -= __SYSTEM_HEAP_PERIOD__;

#if PLATFORM_ID == PLATFORM_WINDOWS
        PROCESS_MEMORY_COUNTERS pmc = { 0 };
        GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
        stats->memory_usage = pmc.WorkingSetSize;
#elif __GLIBC__ > 2 || __GLIBC_MINOR__ > 33
        // `mallinfo2()` is available only starting from glibc-2.33, superseding `mallinfo()`.
        struct mallinfo2 mi = mallinfo2();
        stats->memory_usage = mi.uordblks;
#else
        struct mallinfo mi = mallinfo();
        stats->memory_usage = mi.uordblks;
#endif
    }
#endif  /* __SYSTEM_HEAP_STATISTICS__ */
}

bool Environment_update(Environment_t *environment, float frame_time)
{
    environment->state.time += frame_time;

    return true;
}
