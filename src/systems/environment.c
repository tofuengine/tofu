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
 * Copyright (c) 2019-2026 Marco Lizza
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
#include <libs/fmath.h>
#if defined(TOFU_ENGINE_HEAP_STATISTICS)
#include <libs/heap.h>
#endif  /* defined(TOFU_ENGINE_HEAP_STATISTICS) */
#include <libs/imath.h>
#define _LOG_TAG "environment"
#include <libs/log.h>
#include <libs/stb.h>

#define _EMA_ALPHA 0.1f

Environment_t *Environment_create(const Environment_Configuration_t *configuration, const Display_t *display, const Interpreter_t *interpreter)
{
    Environment_t *environment = malloc(sizeof(Environment_t));
    if (!environment) {
        LOG_E("can't allocate environment");
        return NULL;
    }
    LOG_D("environment allocated");

    *environment = (Environment_t){
            .configuration = *configuration,
            .display = display,
            .interpreter = interpreter,
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
    return frame_time > FLT_EPSILON ? IROUNDF(1.0f / frame_time) : 0;
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
#else   /* defined(TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE) */
    static float average = 0.0f;

    average = FLERP(average, frame_time, 0.1); // Smaller values makes the average more "stable".
    return _frame_time_to_fps(average);
#endif  /* defined(TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE) */
}

// TODO: ditch moving average, and write a devlog about it! :)
#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
static inline void _calculate_times(float times[Environment_Index_t_CountOf], const float deltas[Environment_Index_t_CountOf])
{
#if defined(TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE)
    static float samples[Environment_Index_t_CountOf][TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE_SAMPLES] = { 0 };
    static size_t index = 0;
    static float sums[Environment_Index_t_CountOf] = { 0 };

    for (size_t i = 0; i < Environment_Index_t_CountOf; ++i) {
        const float t = deltas[i] * 1000.0f;
        sums[i] -= samples[i][index];
        samples[i][index] = t;
        sums[i] += t;
        times[i] = sums[i] / (float)TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE_SAMPLES;
    }
    index = (index + 1) % TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE_SAMPLES;
#else   /* defined(TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE) */
    static float averages[Environment_Index_t_CountOf] = { 0 };

    for (size_t i = 0; i < Environment_Index_t_CountOf; ++i) {
        const float t = deltas[i] * 1000.0f;
        averages[i] = FLERP(averages[i], t, _EMA_ALPHA); // Ditto.
        times[i] = averages[i];
    }
#endif  /* defined(TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE) */
}
#endif  /* defined(TOFU_ENGINE_PERFORMANCE_STATISTICS) */

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
void Environment_accumulate(Environment_t *environment, float frame_time, const float deltas[Environment_Index_t_CountOf])
#else   /* defined(TOFU_ENGINE_PERFORMANCE_STATISTICS) */
void Environment_accumulate(Environment_t *environment, float frame_time)
#endif  /* defined(TOFU_ENGINE_PERFORMANCE_STATISTICS) */
{
    Environment_State_t *state = &environment->state;

    Environment_Stats_t *stats = &state->stats;
    stats->fps = _calculate_fps(frame_time); // We could use `1 / frame_time` but it would be inaccurate due to rounding/representation.

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
    _calculate_times(stats->times, deltas);
#endif  /* defined(TOFU_ENGINE_PERFORMANCE_STATISTICS) */

#if defined(TOFU_ENGINE_HEAP_STATISTICS)
    stats->memory_usage = FLERP(stats->memory_usage, heap_usage(), _EMA_ALPHA);
    stats->vm_memory_usage = FLERP(stats->memory_usage, Interpreter_stats(environment->interpreter), _EMA_ALPHA);
#endif  /* defined(TOFU_ENGINE_HEAP_STATISTICS) */
}

static inline bool _is_active(const Display_t *display)
{
    return glfwGetWindowAttrib(display->window, GLFW_FOCUSED) == GLFW_TRUE;
}

bool Environment_update(Environment_t *environment, float delta_time)
{
    Environment_State_t *state = &environment->state;

    state->time += delta_time;

    state->is_active = _is_active(environment->display);

#if defined(TOFU_ENGINE_STATISTICS_DEBUG)
    Environment_Stats_t *stats = &state->stats;

    static float stats_time = 0.0f;
    stats_time += delta_time;
    while (stats_time > TOFU_ENGINE_STATISTICS_PERIOD) {
        stats_time -= TOFU_ENGINE_STATISTICS_PERIOD;
        LOG_I("uptime is %0.2fs",
            state->time);
#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        LOG_I("running at %d FPS (P=%.3fms (%.2f), U=%.3fms (%.2f), R=%.3fms (%.2f), W=%.3fms (%.2f), F=%.3fms)",
            stats->fps,
            stats->times[ENVIRONMENT_INDEX_PROCESS], stats->times[ENVIRONMENT_INDEX_PROCESS] / stats->times[ENVIRONMENT_INDEX_FRAME],
            stats->times[ENVIRONMENT_INDEX_UPDATE ], stats->times[ENVIRONMENT_INDEX_UPDATE ] / stats->times[ENVIRONMENT_INDEX_FRAME],
            stats->times[ENVIRONMENT_INDEX_RENDER ], stats->times[ENVIRONMENT_INDEX_RENDER ] / stats->times[ENVIRONMENT_INDEX_FRAME],
            stats->times[ENVIRONMENT_INDEX_WAIT   ], stats->times[ENVIRONMENT_INDEX_WAIT   ] / stats->times[ENVIRONMENT_INDEX_FRAME],
            stats->times[ENVIRONMENT_INDEX_FRAME  ]);
#endif  /* defined(TOFU_ENGINE_PERFORMANCE_STATISTICS) */
#if defined(TOFU_ENGINE_HEAP_STATISTICS)
        LOG_I("heap-usage %.0fKiB, vm heap-usage %.0fKiB",
            stats->memory_usage / 1024.0f,
            stats->vm_memory_usage / 1024.0f);
#endif  /* defined(TOFU_ENGINE_HEAP_STATISTICS) */
    }
#endif  /* defined(TOFU_ENGINE_STATISTICS_DEBUG) */

    return true;
}
