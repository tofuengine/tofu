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
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif

static Timer_t *push(Timer_Pool_t *pool, float period, size_t repeats, void *bundle)
{
    Timer_t *timer = malloc(sizeof(Timer_t));
    *timer = (Timer_t){
            .period = period,
            .repeats = repeats,
            .bundle = bundle,
            .age = 0.0f,
            .loops = repeats,
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
    *timer = (Timer_t){}; // Clear in DEBUG mode for easier, umm, debugging.
#endif
    free(timer);
}

static Timer_t *pop_next(Timer_Pool_t *pool, Timer_t *timer)
{
    Timer_t *next = timer->next;
    pop(pool, timer);
    return next;
}

void TimerPool_initialize(Timer_Pool_t *pool, TimerPool_Callback_t update_callback, void *parameters)
{
    *pool = (Timer_Pool_t){
            .timers = NULL,
            .update_callback = update_callback,
            .parameters = parameters
        };
}

void TimerPool_terminate(Timer_Pool_t *pool)
{
    for (Timer_t *timer = pool->timers; timer != NULL; ) {
        Log_write(LOG_LEVELS_DEBUG, "<TIMERPOOL> timer #%p released", timer);
        timer = pop_next(pool, timer);
    }
    *pool = (Timer_Pool_t){};
}

Timer_t *TimerPool_allocate(Timer_Pool_t *pool, float period, size_t repeats, void *bundle)
{
    return push(pool, period, repeats, bundle);
}

void TimerPool_gc(Timer_Pool_t *pool)
{
    for (Timer_t *timer = pool->timers; timer != NULL; ) {
        if (timer->state == TIMER_STATE_FINALIZED) { // Periodically release garbage-collected timers.
            Log_write(LOG_LEVELS_DEBUG, "<TIMERPOOL> timer #%p garbage-collected", timer);
            timer = pop_next(pool, timer);
        } else {
            timer = timer->next;
        }
    }
}

bool TimerPool_update(Timer_Pool_t *pool, float delta_time)
{
    for (Timer_t *timer = pool->timers; timer != NULL; timer = timer->next) {
        if (timer->state != TIMER_STATE_RUNNING) {
            continue;
        }

        timer->age += delta_time;
        while (timer->age >= timer->period) {
            if (timer->state != TIMER_STATE_RUNNING) { // The timer could have been terminated on the previous callback loop (when age is large enough).
                break;
            }

            timer->age -= timer->period;

            if (!pool->update_callback(timer, pool->parameters)) {
                return false;
            }

            if (timer->loops > 0) {
                timer->loops -= 1;
                if (timer->loops == 0) {
                    timer->state = TIMER_STATE_FROZEN;
                }
            }
        }
    }
    return true;
}

void TimerPool_release(Timer_t *timer)
{
    timer->state = TIMER_STATE_FINALIZED; // Mark as to-be-released, it not already done (e.g. on closing)

    Log_write(LOG_LEVELS_DEBUG, "<TIMERPOOL> timer #%p finalized, ready for GC", timer);
}

void TimerPool_reset(Timer_t *timer)
{
    if (timer->state != TIMER_STATE_FINALIZED) {
        timer->age = 0.0f;
        timer->loops = timer->repeats;
        timer->state = TIMER_STATE_RUNNING;

        Log_write(LOG_LEVELS_DEBUG, "<TIMERPOOL> timer #%p reset", timer);
    }
}

void TimerPool_cancel(Timer_t *timer)
{
    if (timer->state == TIMER_STATE_RUNNING) {
        timer->state = TIMER_STATE_FROZEN;

        Log_write(LOG_LEVELS_DEBUG, "<TIMERPOOL> timer #%p frozen", timer);
    }
}
