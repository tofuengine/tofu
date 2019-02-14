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

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <stdbool.h>
#include <limits.h>

#include "configuration.h"
#include "display.h"
#include "environment.h"
#include "interpreter.h"

#define STATISTICS_LENGTH       120

typedef struct _Engine_t {
    Environment_t environment;

    Configuration_t configuration;

    Display_t display;
    Interpreter_t interpreter;
} Engine_t;

typedef struct _Engine_Statistics_t {
    double delta_time;
    double min_fps, max_fps;
    double current_fps;
    double history[STATISTICS_LENGTH];
    int index;
} Engine_Statistics_t;

extern bool Engine_initialize(Engine_t *engine, const char *base_path);
extern void Engine_terminate(Engine_t *engine);
extern void Engine_run(Engine_t *engine);

#endif  /* __ENGINE_H__ */