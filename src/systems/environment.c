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
#define _LOG_TAG "environment"
#include <libs/log.h>
#include <libs/stb.h>

#include <malloc.h>
#if PLATFORM_ID == PLATFORM_WINDOWS
    #include <windows.h>
    #include <psapi.h>
#endif

// TODO: http://www.ilikebigbits.com/2017_06_01_float_or_double.html

Environment_t *Environment_create(const Display_t *display, const Input_t *input)
{
    Environment_t *environment = malloc(sizeof(Environment_t));
    if (!environment) {
        LOG_E("can't allocate environment");
        return NULL;
    }
    LOG_D("environment allocated");

    *environment = (Environment_t){
            .display = display,
            .input = input,
            .state = (Environment_State_t){
#if defined(TOFU_EVENTS_FOCUS_SUPPORT)
                .active = { .is = false, .was = false },
#endif  /* TOFU_EVENTS_FOCUS_SUPPORT */
#if defined(TOFU_EVENTS_CONTROLLER_SUPPORT)
                .controllers = { .previous = -1, .current = -1 },
#endif  /* TOFU_EVENTS_CONTROLLER_SUPPORT */
                .stats = { 0 },
                .time = 0.0
            }
        };

    return environment;
}

void Environment_destroy(Environment_t *environment)
{
    free(environment);
    LOG_D("environment freed");
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

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
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
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

#if defined(TOFU_ENGINE_HEAP_STATISTICS)
static inline size_t _heap_usage(void)
{
#if PLATFORM_ID == PLATFORM_WINDOWS
    PROCESS_MEMORY_COUNTERS pmc = { 0 };
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.WorkingSetSize;
#elif __GLIBC__ > 2 || __GLIBC_MINOR__ > 33
    // `mallinfo2()` is available only starting from glibc-2.33, superseding `mallinfo()`.
    struct mallinfo2 mi = mallinfo2();
    return mi.uordblks;
#else
    struct mallinfo mi = mallinfo();
    return mi.uordblks;
#endif
}
#endif  /* TOFU_ENGINE_HEAP_STATISTICS */

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
void Environment_process(Environment_t *environment, float frame_time, const float deltas[4])
#else
void Environment_process(Environment_t *environment, float frame_time)
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
{
    Environment_State_t *state = &environment->state;
#if defined(TOFU_EVENTS_FOCUS_SUPPORT)
    state->active.was = state->active.is;
    state->active.is = glfwGetWindowAttrib(environment->display->window, GLFW_FOCUSED) == GLFW_TRUE;
#endif  /* TOFU_EVENTS_FOCUS_SUPPORT */
#if defined(TOFU_EVENTS_CONTROLLER_SUPPORT)
    state->controllers.previous = state->controllers.current;
    state->controllers.current = Input_get_controllers_count(environment->input);
#endif  /* TOFU_EVENTS_CONTROLLER_SUPPORT */

    Environment_Stats_t *stats = &state->stats;
    stats->fps = _calculate_fps(frame_time); // FIXME: ditch this! It's implicit in the frame time!

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
    _calculate_times(stats->times, deltas);
#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS_DEBUG)
    static float stats_time = TOFU_ENGINE_PERFORMANCE_STATISTICS_PERIOD;
    stats_time += frame_time;
    while (stats_time > TOFU_ENGINE_PERFORMANCE_STATISTICS_PERIOD) {
        stats_time -= TOFU_ENGINE_PERFORMANCE_STATISTICS_PERIOD;
        LOG_I("currently running at %d FPS (P=%.3fms, U=%.3fms, R=%.3fms, F=%.3fms)",
            stats->fps, stats->times[0], stats->times[1], stats->times[2], stats->times[3]);
    }
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS_DEBUG */
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

#if defined(TOFU_ENGINE_HEAP_STATISTICS)
    stats->memory_usage = _heap_usage();
#if defined(TOFU_ENGINE_HEAP_STATISTICS_DEBUG)
    static float heap_time = TOFU_ENGINE_HEAP_STATISTICS_PERIOD;
    heap_time += frame_time;
    while (heap_time > TOFU_ENGINE_HEAP_STATISTICS_PERIOD) {
        heap_time -= TOFU_ENGINE_HEAP_STATISTICS_PERIOD;
        LOG_I("currently using %u byte(s)", stats->memory_usage);
    }
#endif  /* TOFU_ENGINE_HEAP_STATISTICS_DEBUG */
#endif  /* TOFU_ENGINE_HEAP_STATISTICS */
}

bool Environment_update(Environment_t *environment, float frame_time)
{
    environment->state.time += frame_time;

    return true;
}
