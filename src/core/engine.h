/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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

#ifndef TOFU_CORE_ENGINE_H
#define TOFU_CORE_ENGINE_H

#include "configuration.h"

#include <systems/audio.h>
#include <systems/display.h>
#include <systems/environment.h>
#include <systems/input.h>
#include <systems/storage.h>
#include <systems/interpreter.h>

typedef struct Engine_Options_s {
    char kernal_path[PLATFORM_PATH_MAX];
    char data_path[PLATFORM_PATH_MAX];
} Engine_Options_t;

typedef struct Engine_s {
    Storage_t *storage;
    Configuration_t *configuration;
    Display_t *display;
    Input_t *input;
    Audio_t *audio;
    Environment_t *environment;
    Interpreter_t *interpreter;
} Engine_t;

extern Engine_t *Engine_create(const Engine_Options_t *options);
extern void Engine_destroy(Engine_t *engine);

extern void Engine_run(Engine_t *engine);

#endif  /* TOFU_CORE_ENGINE_H */
