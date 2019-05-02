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
    pool->timers = calloc(pool->capacity, sizeof(Timer_t));
}

void TimerPool_terminate(Timer_Pool_t *pool, TimerPool_Callback_t callback, void *parameters)
{
    for (size_t i = 0; i < pool->capacity; ++i) {
        TimerPool_release(pool, i);
    }

    TimerPool_free(pool, callback, parameters);

    free(pool->timers);
}

size_t TimerPool_allocate(Timer_Pool_t *pool, float period, int repeats, WrenHandle *callback)
{
    bool done = false;
    while (!done) {
        for (size_t i = 0; i < pool->capacity; ++i) {
            if (pool->timers[i].state == TIMER_STATE_DEAD) {
                pool->timers[i] = (Timer_t){
                        .period = period,
                        .repeats = repeats,
                        .callback = callback,
                        .age = 0.0f,
                        .state = TIMER_STATE_ALIVE
                    };
                return i;
            }
        }
        if (!done) {
            size_t was_capacity = pool->capacity;
            size_t capacity = pool->capacity * 2;
            pool->timers = realloc(pool->timers, capacity);
            for (size_t i = was_capacity; i < capacity; ++i) {
                pool->timers[i].state = TIMER_STATE_DEAD;
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

        if (timer->state != TIMER_STATE_ALIVE) {
            continue;
        }

        timer->age += delta_time;
        while (timer->age >= timer->period) {
            timer->age -= timer->period;

            callback(timer, parameters);

            if (timer->repeats > 0) {
                timer->repeats -= 1;
                if (timer->repeats == 0) {
                    timer->state = TIMER_STATE_ZOMBIE;
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

        if (timer->state == TIMER_STATE_ZOMBIE) { // Periodically release garbage-collected timers.
            callback(timer, parameters);

            timer->state = TIMER_STATE_DEAD;

            Log_write(LOG_LEVELS_DEBUG, "[TOFU] Timer #%d is now dead", i);
        }
    }

    // TODO: Repack the array, no dead holes.
}

void TimerPool_release(Timer_Pool_t *pool, int slot)
{
    if (pool->timers[slot].state == TIMER_STATE_ALIVE) { // TODO: check if the timer-id hasn't changed? Use a disposable linked-list?
        pool->timers[slot].state = TIMER_STATE_ZOMBIE; // Mark as to-be-released, it not already done (e.g. on closing)
    }
}

void TimerPool_free(Timer_Pool_t *pool, TimerPool_Callback_t callback, void *parameters)
{
    for (size_t i = 0; i < pool->capacity; ++i) {
        Timer_t *timer = &pool->timers[i];

        if (timer->state != TIMER_STATE_DEAD) { // Dispose noth ALIVE and ZOMBIE timers.
            callback(timer, parameters);

            timer->state = TIMER_STATE_DEAD; // Mark as dead, to avoid "zombification" in the timer finalizer.

            Log_write(LOG_LEVELS_DEBUG, "[TOFU] Timer #%d released", i);
        }
    }
}