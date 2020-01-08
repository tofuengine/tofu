/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <config.h>
#include <core/configuration.h>
#include <core/environment.h>
#include <core/io/audio.h>
#include <core/io/display.h>
#include <core/io/input.h>
#include <core/vm/interpreter.h>
#include <libs/fs/fs.h>

#include <stdbool.h>
#include <limits.h>

#define TOFU_VERSION_MAJOR          0
#define TOFU_VERSION_MINOR          6
#define TOFU_VERSION_REVISION       0

typedef struct _Engine_t {
    File_System_t file_system;

    Configuration_t configuration;

    Interpreter_t interpreter;
    Audio_t audio;
    Display_t display;
    Input_t input;

    Environment_t environment;
} Engine_t;

extern bool Engine_initialize(Engine_t *engine, const char *base_path);
extern void Engine_terminate(Engine_t *engine);
extern void Engine_run(Engine_t *engine);

#endif  /* __ENGINE_H__ */