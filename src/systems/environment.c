/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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
#include <libs/fmath.h>
#include <libs/imath.h>
#define _LOG_TAG "environment"
#include <libs/log.h>
#include <libs/stb.h>

#include <malloc.h>
#if PLATFORM_ID == PLATFORM_WINDOWS
    #include <windows.h>
    #include <psapi.h>
#endif

Environment_t *Environment_create(const Display_t *display)
{
    Environment_t *environment = malloc(sizeof(Environment_t));
    if (!environment) {
        LOG_E("can't allocate environment");
        return NULL;
    }
    LOG_D("environment allocated");

    *environment = (Environment_t){
            .display = display,
            .state = (Environment_State_t){
                .is_active = false,
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

static inline size_t _frame_time_to_fps(float frame_time)
{
    return frame_time > __FLT_EPSILON__ ?  IROUNDF(1.0f / frame_time) : 0;
}

static inline size_t _calculate_fps(float frame_time)
{
#if defined(TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE)
    static float samples[TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE_SAMPLES] = { 0 };
    static size_t index = 0;
    static float sum = 0.0f; // We are storing just a small time interval, float is enough...

    sum -= samples[index];
    samples[index] = frame_time;
    sum += frame_time;
    index = (index + 1) % TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE_SAMPLES;

    return IROUND((float)TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE_SAMPLES / sum);
#else
    static float average = 0.0f;

    average = FLERP(average, frame_time, 0.1); // Smaller values makes the average more "stable".
    return _frame_time_to_fps(average);
#endif
}

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
static inline void _calculate_times(float times[5], const float deltas[5])
{
#if defined(TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE)
    static float samples[5][TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE_SAMPLES] = { 0 };
    static size_t index = 0;
    static float sums[5] = { 0 };

    for (size_t i = 0; i < 5; ++i) {
        const float t = deltas[i] * 1000.0f;
        sums[i] -= samples[i][index];
        samples[i][index] = t;
        sums[i] += t;
        times[i] = sums[i] / (float)TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE_SAMPLES;
    }
    index = (index + 1) % TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE_SAMPLES;
#else
    static float averages[5] = { 0 };

    for (size_t i = 0; i < 5; ++i) {
        const float t = deltas[i] * 1000.0f;
        averages[i] = FLERP(averages[i], t, 0.1f); // Ditto.
        times[i] = averages[i];
    }
#endif
}
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
void Environment_accumulate(Environment_t *environment, float frame_time, const float deltas[5])
#else
void Environment_accumulate(Environment_t *environment, float frame_time)
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
{
    Environment_State_t *state = &environment->state;

    Environment_Stats_t *stats = &state->stats;
    stats->fps = _calculate_fps(frame_time); // We could use `1 / frame_time` but it would be inaccurate due to rounding/representation.

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
    _calculate_times(stats->times, deltas);
#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS_DEBUG)
    static float stats_time = TOFU_ENGINE_PERFORMANCE_STATISTICS_PERIOD;
    stats_time += frame_time;
    while (stats_time > TOFU_ENGINE_PERFORMANCE_STATISTICS_PERIOD) {
        stats_time -= TOFU_ENGINE_PERFORMANCE_STATISTICS_PERIOD;
        LOG_I("currently running at %d FPS (P=%.3fms (%.2f), U=%.3fms (%.2f), R=%.3fms (%.2f), W=%.3fms (%.2f), F=%.3fms)",
            stats->fps,
            stats->times[0], stats->times[0] / stats->times[4],
            stats->times[1], stats->times[1] / stats->times[4],
            stats->times[2], stats->times[2] / stats->times[4],
            stats->times[3], stats->times[3] / stats->times[4],
            stats->times[4]);
    }
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS_DEBUG */
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
}

static inline bool _is_active(const Display_t *display)
{
    return glfwGetWindowAttrib(display->window, GLFW_FOCUSED) == GLFW_TRUE;
}

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

bool Environment_update(Environment_t *environment, float delta_time)
{
    Environment_State_t *state = &environment->state;

    state->time += delta_time;

    state->is_active = _is_active(environment->display);

#if defined(TOFU_ENGINE_HEAP_STATISTICS)
    Environment_Stats_t *stats = &state->stats;
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

    return true;
}
