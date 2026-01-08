/*
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

#include "time.h"

#include <core/config.h>
#include <core/platform.h>
#define _LOG_TAG "soy:time"
#include <libs/log.h>

#if TOFU_CORE_BACKEND == BACKEND_GLFW
  #include <GLFW/glfw3.h>
#endif

#if PLATFORM_ID == PLATFORM_WINDOWS
    #include <windows.h>
#elif PLATFORM_ID == PLATFORM_LINUX && defined(TOFU_CORE_USE_USLEEP)
    #include <sched.h>
    #include <unistd.h>
#else
    #include <sched.h>
    #include <time.h>
#endif

double soy_get_time(void)
{
#if TOFU_CORE_BACKEND == BACKEND_GLFW
    return glfwGetTime();
#else
    return 0.0;
#endif
}

void soy_set_time(double seconds)
{
#if TOFU_CORE_BACKEND == BACKEND_GLFW
    glfwSetTime(seconds);
#else
    // Do nothing.
#endif
}

// This is the lowest amount of time (in milliseconds) that we are willing to
// suspend the execution (i.e. sleep) in a single loop.
//
// We set it to `1` as the actual sleep call will almost certainly (overall)
// consume a bit more than the requested amount (due to the call overhead). This
// way we are reasonably sure not to oversleep, at the cost of burning with a
// "semi-busy wait" the last millisecond (at most).
//
// See: https://nkga.github.io/post/frame-pacing-analysis-of-the-game-loop/
//      https://github.com/urho3d/urho3d/blob/master/Source/Urho3D/Engine/Engine.cpp#L750
#define _WAIT_SLOT  1

void soy_wait_for(float seconds)
{
    double marker = soy_get_time();

    for (;;) {
        const double now = soy_get_time();
        const double elapsed = now - marker;

        if (elapsed >= seconds) { // The requested time has passed, bail out!
            break;
        }

        long millis = (long)((seconds - elapsed) * 1000.0); // The delta time is expressed in seconds...
        if (millis > _WAIT_SLOT) {
            // If more than a the wait-slot is left to wait then suspend the execution
            // for that amount of time...
            long millis_to_wait = millis - _WAIT_SLOT;
#if PLATFORM_ID == PLATFORM_WINDOWS
            Sleep(millis_to_wait);
#elif defined(TOFU_CORE_USE_USLEEP)
            usleep(millis_to_wait * 1000L); // usleep takes sleep time in us (1 millionth of a second)
#else
            struct timespec ts = (struct timespec){
                    .tv_sec = (time_t)(millis_to_wait / 1000L),
                    .tv_nsec = (time_t)((millis_to_wait % 1000L) * 1000000L)
                };
#if defined(TOFU_CORE_USE_CLOCK_NANOSLEEP)
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
#else
            nanosleep(&ts, NULL);
#endif
#endif
        } else {
            // ... otherwise adopt as semi-busy wait, yielding the processor
            // usage to avoid "clogging" the system and successive "sleep spikes".
#if PLATFORM_ID == PLATFORM_WINDOWS
            YieldProcessor();
#else
            sched_yield();
#endif
        }
    }
}