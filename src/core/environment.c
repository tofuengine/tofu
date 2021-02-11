/*
 * MIT License
 * 
 * Copyright (c) 2019-2021 Marco Lizza
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

#include <config.h>
#include <libs/log.h>
#include <libs/stb.h>

#define LOG_CONTEXT "environment"

// TODO: http://www.ilikebigbits.com/2017_06_01_float_or_double.html

Environment_t *Environment_create(int argc, const char *argv[], const Display_t *display)
{
    Environment_t *environment = malloc(sizeof(Environment_t));
    if (!environment) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate environment");
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "environment allocated");

    const char **args = NULL;
    for (int i = 1; i < argc; ++i) { // Skip executable name, i.e. argument #0.
        arrpush(args, argv[i]);
    }

    *environment = (Environment_t){
        .args = args,
        .display = display,
        .quit = false,
        .time = 0.0,
        .stats = { 0 }
    };

    return environment;
}

void Environment_destroy(Environment_t *environment)
{
    arrfree(environment->args);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "arguments freed");

    free(environment);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "environment freed");
}

void Environment_quit(Environment_t *environment)
{
    environment->quit = true;
}

bool Environment_should_quit(const Environment_t *environment)
{
    return environment->quit || Display_should_close(environment->display);
}

double Environment_get_time(const Environment_t *environment)
{
    return environment->time;
}

const Environment_Stats_t *Environment_get_stats(const Environment_t *environment)
{
    return &environment->stats;
}

bool Environment_is_active(const Environment_t *environment)
{
    return environment->is_active;
}

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
static inline float _calculate_fps(float frame_time) // FIXME: rework this as a reusable function for moving average.
{
    static float samples[FPS_AVERAGE_SAMPLES] = { 0 };
    static size_t index = 0;
    static float sum = 0.0f; // We are storing just a small time interval, float is enough...

    sum -= samples[index];
    samples[index] = frame_time;
    sum += frame_time;
    index = (index + 1) % FPS_AVERAGE_SAMPLES;

    return (float)FPS_AVERAGE_SAMPLES / sum;
}

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
#ifdef __ENGINE_PERFORMANCE_STATISTICS__
    environment->stats.fps = _calculate_fps(frame_time);
    _calculate_times(environment->stats.times, deltas);
#ifdef __DEBUG_ENGINE_PERFORMANCES__
    static size_t count = 0;
    if (++count == (FPS_AVERAGE_SAMPLES * 2)) {
        Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "currently running at %.2f FPS (P=%.3fms, U=%.3fms, R=%.3fms, F=%.3fms)",
            environment->stats.fps, environment->stats.times[0], environment->stats.times[1], environment->stats.times[2], environment->stats.times[3]);
        count = 0;
    }
#endif  /* __DEBUG_ENGINE_PERFORMANCES__ */
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */
#ifdef __DISPLAY_FOCUS_SUPPORT__
    environment->is_active = glfwGetWindowAttrib(environment->display->window, GLFW_FOCUSED) == GLFW_TRUE;
#endif
}

void Environment_update(Environment_t *environment, float frame_time)
{
    environment->time += frame_time;
}
