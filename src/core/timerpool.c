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

#include "../config.h"

#include "../log.h"

#include <memory.h>

static Timer_t *push(Timer_Pool_t *pool, const Timer_Value_t value)
{
    Timer_t *timer = malloc(sizeof(Timer_t));
    *timer = (Timer_t){
            .value = value,
            .age = 0.0f,
            .loops = value.repeats,
            .state = TIMER_STATE_RUNNING,
            .prev = NULL,
            .next = pool->timers
        };

    if (pool->timers != NULL) { // Update head not, if present, prior adding to the front of the list.
        pool->timers->prev = timer;
    }
    pool->timers = timer;

    return timer;
}

static void pop(Timer_Pool_t *pool, Timer_t *timer)
{
    if (pool->timers == timer) { // Update head pointer if we are releasing it.
        pool->timers = timer->next;
    }

    if (timer->prev != NULL) { // Unlink node from the list.
        timer->prev->next = timer->next;
    }
    if (timer->next != NULL) {
        timer->next->prev = timer->prev;
    }

#if DEBUG
    memset(timer, 0, sizeof(Timer_t)); // Clear in DEBUG mode for easier, umm, debugging.
#endif
    free(timer);
}

static Timer_t *pop_next(Timer_Pool_t *pool, Timer_t *timer)
{
    Timer_t *next = timer->next;
    pop(pool, timer);
    return next;
}

static bool contains(Timer_Pool_t *pool, Timer_t *timer)
{
    for (Timer_t *current = pool->timers; current != NULL; current = current->next) {
        if (current == timer) {
            return true;
        }
    }
    return false;
}

void TimerPool_initialize(Timer_Pool_t *pool, size_t initial_capacity)
{
    pool->timers = NULL;
}

void TimerPool_terminate(Timer_Pool_t *pool, TimerPool_Callback_t callback, void *parameters)
{
    for (Timer_t *timer = pool->timers; timer != NULL; ) {
        callback(timer, parameters);
        Log_write(LOG_LEVELS_DEBUG, "<TIMERPOOL> timer #%p released", timer);
        timer = pop_next(pool, timer);
    }
}

Timer_t *TimerPool_allocate(Timer_Pool_t *pool, const Timer_Value_t value)
{
    return push(pool, value);
}

void TimerPool_gc(Timer_Pool_t *pool, TimerPool_Callback_t callback, void *parameters)
{
    for (Timer_t *timer = pool->timers; timer != NULL; ) {
        if (timer->state == TIMER_STATE_FINALIZED) { // Periodically release garbage-collected timers.
            callback(timer, parameters);
            Log_write(LOG_LEVELS_DEBUG, "<TIMERPOOL> timer #%p garbage-collected", timer);
            timer = pop_next(pool, timer);
        } else {
            timer = timer->next;
        }
    }
}

void TimerPool_update(Timer_Pool_t *pool, double delta_time, TimerPool_Callback_t callback, void *parameters)
{
    for (Timer_t *timer = pool->timers; timer != NULL; timer = timer->next) {
        if (timer->state != TIMER_STATE_RUNNING) {
            continue;
        }

        timer->age += delta_time;
        while (timer->age >= timer->value.period) {
            if (timer->state != TIMER_STATE_RUNNING) { // The timer could have been terminated
                break;
            }

            timer->age -= timer->value.period;

            callback(timer, parameters);

            if (timer->loops > 0) {
                timer->loops -= 1;
                if (timer->loops == 0) {
                    timer->state = TIMER_STATE_FROZEN;
                }
            }
        }
    }
}

void TimerPool_release(Timer_Pool_t *pool, Timer_t *timer)
{
    if (!contains(pool, timer)) { // Check if the timer is still present in the pool (could have been freed by `TimerPool_terminate()`).
        return;
    }

    timer->state = TIMER_STATE_FINALIZED; // Mark as to-be-released, it not already done (e.g. on closing)

    Log_write(LOG_LEVELS_DEBUG, "<TIMERPOOL> timer #%p finalized, ready for GC", timer);
}

void TimerPool_reset(Timer_Pool_t *pool, Timer_t *timer)
{
    if (!contains(pool, timer)) {
        return;
    }

    if (timer->state != TIMER_STATE_FINALIZED) {
        timer->age = 0.0;
        timer->loops = timer->value.repeats;
        timer->state = TIMER_STATE_RUNNING;

        Log_write(LOG_LEVELS_DEBUG, "<TIMERPOOL> timer #%p reset", timer);
    }
}

void TimerPool_cancel(Timer_Pool_t *pool, Timer_t *timer)
{
    if (!contains(pool, timer)) {
        return;
    }

    if (timer->state == TIMER_STATE_RUNNING) {
        timer->state = TIMER_STATE_FROZEN;

        Log_write(LOG_LEVELS_DEBUG, "<TIMERPOOL> timer #%p frozen", timer);
    }
}
