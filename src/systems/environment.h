/*
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#ifndef TOFU_SYSTEMS_ENVIRONMENT_H
#define TOFU_SYSTEMS_ENVIRONMENT_H

#include "display.h"
#include "input.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct Environment_Stats_s {
    size_t fps;
#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
    float times[4];
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
#if defined(TOFU_ENGINE_HEAP_STATISTICS)
    size_t memory_usage;
#endif  /* TOFU_ENGINE_HEAP_STATISTICS */
} Environment_Stats_t;

typedef struct Environment_State_s {
#if defined(TOFU_EVENTS_FOCUS_SUPPORT)
    struct {
        bool is;
        bool was;
    } active;
#endif  /* TOFU_EVENTS_FOCUS_SUPPORT */
#if defined(TOFU_EVENTS_CONTROLLER_SUPPORT)
    struct {
        int previous;
        int current;
    } controllers;
#endif  /* TOFU_EVENTS_CONTROLLER_SUPPORT */
    Environment_Stats_t stats;
    double time;
} Environment_State_t;

typedef struct Environment_s {
    const Display_t *display;
    const Input_t *input;
    Environment_State_t state;
} Environment_t;

extern Environment_t *Environment_create(const Display_t *display, const Input_t *input);
extern void Environment_destroy(Environment_t *environment);

extern const Environment_State_t *Environment_get_state(const Environment_t *environment);

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
extern void Environment_process(Environment_t *environment, float frame_time, const float deltas[4]);
#else
extern void Environment_process(Environment_t *environment, float frame_time);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

extern bool Environment_update(Environment_t *environment, float delta_time);

#endif  /* TOFU_SYSTEMS_ENVIRONMENT_H */
