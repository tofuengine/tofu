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

#include "util.h"

#include "../config.h"
#include "../environment.h"
#include "../log.h"

const char util_wren[] =
    "foreign class Timer {\n"
    "\n"
    "    construct new(period, repeats, callback) {}\n"
    "\n"
    "    foreign cancel()\n"
    "\n"
    "}\n"
;

void util_timer_allocate(WrenVM *vm)
{
    float period = (float)wrenGetSlotDouble(vm, 1);
    int repeats = (int)wrenGetSlotDouble(vm, 2);
    WrenHandle *callback = wrenGetSlotHandle(vm, 3); // NOTE! This need to be released when the timer is detached!
#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Timer.new() -> %f, %d, %p", period, repeats, callback);
#endif

    Timer_t *timer = (Timer_t *)wrenSetSlotNewForeign(vm, 0, 0, sizeof(Timer_t)); // `0, 0` since we are in the allocate callback.

    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    // Scan the environment timers-pool and find an empty slot.
    int slot = -1;
    for (size_t i = 0; i < environment->timers_capacity; ++i) {
        if (environment->timers[i] == NULL) {
            environment->timers[i] = timer;
            slot = i;
            break;
        }
    }
    // TODO: if `slot` is minus while, resize and retry.

    *timer = (Timer_t){
            .period = period,
            .repeats = repeats,
            .callback = callback,
            .slot = slot,
            .age = 0.0f,
            .state = TIMER_STATE_ALIVE
        };
}

void util_timer_finalize(void *userData, void *data)
{
    Environment_t *environment = (Environment_t *)userData;
    Timer_t *timer = (Timer_t *)data;

    environment->timers[timer->slot]->state = TIMER_STATE_DEAD;
}

void util_timer_cancel(WrenVM *vm)
{
#ifdef DEBUG
    Log_write(LOG_LEVELS_DEBUG, "Timer.cancel()");
#endif

    Timer_t *timer = (Timer_t *)wrenGetSlotForeign(vm, 0);

    timer->state = TIMER_STATE_ZOMBIE;
}
