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

#include "timerpool.h"

#include "../log.h"

void TimerPool_initialize(Timer_Pool_t *pool, size_t initial_capacity)
{
    pool->capacity = initial_capacity;
    pool->timers = malloc(pool->capacity * sizeof(Timer_t));
    for (size_t i = 0; i < pool->capacity; ++i) {
        pool->timers[i].state = TIMER_STATE_FREE;
    }
}

void TimerPool_terminate(Timer_Pool_t *pool, TimerPool_Callback_t callback, void *parameters)
{
    for (size_t i = 0; i < pool->capacity; ++i) {
        Timer_t *timer = &pool->timers[i];

        if (timer->state != TIMER_STATE_FREE) { // Dispose only non-freed timers.
            callback(timer, parameters);

            timer->state = TIMER_STATE_FREE; // Mark as dead, to avoid "zombification" in the timer finalizer.

            Log_write(LOG_LEVELS_DEBUG, "[TOFU] Timer #%d released", i);
        }
    }

    free(pool->timers);
}

size_t TimerPool_allocate(Timer_Pool_t *pool, float period, int repeats, WrenHandle *callback)
{
    bool done = false;
    while (!done) {
        for (size_t i = 0; i < pool->capacity; ++i) {
            if (pool->timers[i].state == TIMER_STATE_FREE) {
                pool->timers[i] = (Timer_t){
                        .period = period,
                        .repeats = repeats,
                        .callback = callback,
                        .age = 0.0f,
                        .state = TIMER_STATE_RUNNING
                    };
                return i;
            }
        }
        if (!done) {
            size_t was_capacity = pool->capacity;
            size_t capacity = pool->capacity * 2;
            pool->timers = realloc(pool->timers, capacity * sizeof(Timer_t));
            for (size_t i = was_capacity; i < capacity; ++i) {
                pool->timers[i].state = TIMER_STATE_FREE;
            }
            pool->capacity = capacity;
        }
    }
    return __SIZE_MAX__;
}

void TimerPool_update(Timer_Pool_t *pool, double delta_time, TimerPool_Callback_t callback, void *parameters)
{
    for (size_t i = 0; i < pool->capacity; ++i) {
        Timer_t *timer = &pool->timers[i];

        if (timer->state != TIMER_STATE_RUNNING) {
            continue;
        }

        timer->age += delta_time;
        while (timer->age >= timer->period) { // TODO: should check if still alive when looping.
            timer->age -= timer->period;

            callback(timer, parameters);

            if (timer->repeats > 0) {
                timer->repeats -= 1;
                if (timer->repeats == 0) {
                    timer->state = TIMER_STATE_FROZEN;
                    break;
                }
            }
        }
    }
}

void TimerPool_gc(Timer_Pool_t *pool, TimerPool_Callback_t callback, void *parameters)
{
    for (size_t i = 0; i < pool->capacity; ++i) {
        Timer_t *timer = &pool->timers[i];

        if (timer->state == TIMER_STATE_FINALIZED) { // Periodically release garbage-collected timers.
            callback(timer, parameters);

            timer->state = TIMER_STATE_FREE;

            Log_write(LOG_LEVELS_DEBUG, "[TOFU] Timer #%d garbage-collected, slot freed", i);
        }
    }

    // TODO: Repack the array, no dead holes.
}

void TimerPool_release(Timer_Pool_t *pool, int slot)
{
    if (pool->timers[slot].state != TIMER_STATE_FREE) {
        pool->timers[slot].state = TIMER_STATE_FINALIZED; // Mark as to-be-released, it not already done (e.g. on closing)

        Log_write(LOG_LEVELS_DEBUG, "[TOFU] Timer #%d finalized, ready for GC", slot);
    }
}

void TimerPool_cancel(Timer_Pool_t *pool, int slot)
{
    if (pool->timers[slot].state == TIMER_STATE_RUNNING) {
        pool->timers[slot].state = TIMER_STATE_FROZEN;

        Log_write(LOG_LEVELS_DEBUG, "[TOFU] Timer #%d frozen", slot);
    }
}
