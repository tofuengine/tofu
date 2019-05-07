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

#include "../core/timerpool.h"

#include "../config.h"
#include "../environment.h"
#include "../log.h"

typedef struct _Timer_Class_t {
    Timer_Pool_t *timer_pool;
    Timer_t *timer;
} Timer_Class_t;

const char util_wren[] =
    "foreign class Timer {\n"
    "\n"
    "    construct new(period, repeats, callback) {}\n"
    "\n"
    "    foreign reset()\n"
    "    foreign cancel()\n"
    "\n"
    "}\n"
    "\n"
    "foreign class Math {\n"
    "\n"
    "    static max(a, b) {\n"
    "        if (a > b) {\n"
    "            return a\n"
    "        }\n"
    "        return b\n"
    "    }\n"
    "\n"
    "    static min(a, b) {\n"
    "        if (a < b) {\n"
    "            return a\n"
    "        }\n"
    "        return b\n"
    "    }\n"
    "\n"
    "}\n"
;

void util_timer_allocate(WrenVM *vm)
{
    float period = (float)wrenGetSlotDouble(vm, 1);
    int repeats = (int)wrenGetSlotDouble(vm, 2);
    WrenHandle *callback = wrenGetSlotHandle(vm, 3); // NOTE! This need to be released when the timer is detached!
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.new() -> %f, %d, %p", period, repeats, callback);
#endif

    Timer_Class_t *instance = (Timer_Class_t *)wrenSetSlotNewForeign(vm, 0, 0, sizeof(Timer_Class_t)); // `0, 0` since we are in the allocate callback.

    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);
    Timer_Pool_t *timer_pool = environment->timer_pool;

    instance->timer_pool = timer_pool;
    instance->timer = TimerPool_allocate(timer_pool, (Timer_Value_t){
            .period = period,
            .repeats = repeats,
            .callback = callback
        });
}

void util_timer_finalize(void *userData, void *data)
{
    Timer_Class_t *instance = (Timer_Class_t *)data;

    Log_write(LOG_LEVELS_DEBUG, "[TOFU] Finalizing timer #%p", instance->timer);

    TimerPool_release(instance->timer_pool, instance->timer);
}

void util_timer_reset(WrenVM *vm)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.cancel()");
#endif

    Timer_Class_t *instance = (Timer_Class_t *)wrenGetSlotForeign(vm, 0);

    TimerPool_reset(instance->timer_pool, instance->timer);
}

void util_timer_cancel(WrenVM *vm)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.cancel()");
#endif

    Timer_Class_t *instance = (Timer_Class_t *)wrenGetSlotForeign(vm, 0);

    TimerPool_cancel(instance->timer_pool, instance->timer);
}
