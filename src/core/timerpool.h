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

#ifndef __CORE_TIMERPOOL_H__
#define __CORE_TIMERPOOL_H__

#include <stdbool.h>

#include <wren/wren.h>

typedef enum _Timer_State_t {
    TIMER_STATE_DEAD,
    TIMER_STATE_ALIVE,
    TIMER_STATE_ZOMBIE
} Timer_State_t;

typedef struct _Timer_t {
    float period;
    int repeats;
    WrenHandle *callback; // Need to be released explicitly.
    float age;
    Timer_State_t state;
} Timer_t;

typedef struct _Timer_Pool_t {
    Timer_t *timers;
    size_t count;
    size_t capacity;
} Timer_Pool_t;

typedef void (*TimerPool_Callback_t)(Timer_t *timer, void *parameters);

extern void TimerPool_initialize(Timer_Pool_t *pool, size_t initial_capacity);
extern void TimerPool_terminate(Timer_Pool_t *pool, TimerPool_Callback_t callback, void *parameters);
extern size_t TimerPool_allocate(Timer_Pool_t *pool, float period, int repeats, WrenHandle *callback);
extern void TimerPool_update(Timer_Pool_t *pool, double delta_time, TimerPool_Callback_t callback, void *parameters);
extern void TimerPool_gc(Timer_Pool_t *pool, TimerPool_Callback_t callback, void *parameters);
extern void TimerPool_release(Timer_Pool_t *pool, int slot);

#endif  /* __CORE_TIMERPOOL_H__ */