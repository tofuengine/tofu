/*
 * MIT License
 * 
 * Copyright (c) 2019-2021 Marco Lizza
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

#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#include <core/io/display.h>

#include <stdbool.h>
#include <stddef.h>

typedef struct _Environment_Stats_t {
    float fps;
#ifdef __ENGINE_PERFORMANCE_STATISTICS__
    float times[4];
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */
#ifdef __SYSTEM_HEAP_STATISTICS__
    size_t memory_usage;
#endif  /* __SYSTEM_HEAP_STATISTICS__ */
} Environment_Stats_t;

typedef struct _Environment_t {
    const char **args;
    const Display_t *display;
#ifdef __DISPLAY_FOCUS_SUPPORT__
    bool is_active;
#endif
    bool quit;
    double time;
    Environment_Stats_t stats;
} Environment_t;

extern Environment_t *Environment_create(int argc, const char *argv[], const Display_t *display);
extern void Environment_destroy(Environment_t *environment);

extern void Environment_quit(Environment_t *environment);

extern bool Environment_should_quit(const Environment_t *environment);

extern double Environment_get_time(const Environment_t *environment);
extern const Environment_Stats_t *Environment_get_stats(const Environment_t *environment);
#ifdef __DISPLAY_FOCUS_SUPPORT__
extern bool Environment_is_active(const Environment_t *environment);
#endif  /* __DISPLAY_FOCUS_SUPPORT__ */

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
extern void Environment_process(Environment_t *environment, float frame_time, const float deltas[4]);
#else
extern void Environment_process(Environment_t *environment, float frame_time);
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */

extern void Environment_update(Environment_t *environment, float delta_time);

#endif  /* __ENVIRONMENT_H__ */
