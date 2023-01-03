/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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

#ifndef TOFU_SYSTEMS_INTERPRETER_H
#define TOFU_SYSTEMS_INTERPRETER_H

#include "storage.h"

#include <core/config.h>
#include <libs/luax.h>

#include <stdbool.h>

typedef enum lua_Warning_States_e {
    WARNING_STATE_DISABLED,
    WARNING_STATE_READY,
    WARNING_STATE_APPENDING
} lua_Warning_States_t;

typedef struct Interpreter_s {
    lua_State *state;
    lua_Warning_States_t warning_state;
#if __VM_GARBAGE_COLLECTOR_MODE__ == GC_CONTINUOUS
    float gc_step_age;
#endif
#if defined(__VM_GARBAGE_COLLECTOR_PERIODIC_COLLECT__) || defined(__DEBUG_GARBAGE_COLLECTOR__)
    float gc_age;
#endif
} Interpreter_t;

extern Interpreter_t *Interpreter_create(const Storage_t *storage);
extern void Interpreter_destroy(Interpreter_t *interpreter);

extern bool Interpreter_boot(Interpreter_t *interpreter, const void *userdatas[]);
extern bool Interpreter_process(const Interpreter_t *interpreter, const char *events[]);
extern bool Interpreter_update(Interpreter_t *interpreter, float delta_time);
extern bool Interpreter_render(const Interpreter_t *interpreter, float ratio);
extern bool Interpreter_call(const Interpreter_t *interpreter, int nargs, int nresults);

#endif  /* __ TOFU_SYSTEMS_INTERPRETER_H__ */
