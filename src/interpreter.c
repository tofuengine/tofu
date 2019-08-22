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

#if 0
https://www.lua.org/manual/5.2/manual.html
https://www.lua.org/pil/27.3.2.html
https://www.lua.org/pil/25.2.html

https://nachtimwald.com/2014/07/26/calling-lua-from-c/
#endif

#include "interpreter.h"

#include "config.h"
#include "file.h"
#include "log.h"
#include "modules.h"

#include <limits.h>
#include <string.h>
#ifdef __DEBUG_GARBAGE_COLLECTOR__
#include <time.h>
#endif

#define ROOT_INSTANCE           "main"

#define BOOT_SCRIPT \
    "local Main = require(\"main\")\n" \
    "main = Main.new()\n"

#define SHUTDOWN_SCRIPT \
    "main = nil\n"

#define OBJECT_STACK_INDEX      1
#define METHOD_STACK_INDEX(m)   1 + 1 + (m)
#define TRACEBACK_STACK_INDEX   1 + Methods_t_CountOf + 1

typedef enum _Methods_t {
    METHOD_SETUP,
    METHOD_INIT,
    METHOD_INPUT,
    METHOD_UPDATE,
    METHOD_RENDER,
    Methods_t_CountOf
} Methods_t;

static const char *_methods[] = {
    "setup",
    "init",
    "input",
    "update",
    "render",
    NULL
};

static int panic(lua_State *L)
{
    Log_write(LOG_LEVELS_FATAL, "<VM> error in call: %s", lua_tostring(L, -1));
    return 0; // return to Lua to abort
}

#ifdef __DEBUG_VM_USE_CUSTOM_TRACEBACK__
static int error_handler(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);
    if (msg == NULL) {  /* is error object not a string? */
        if (luaL_callmeta(L, 1, "__tostring")  /* does it have a metamethod */
            && lua_type(L, -1) == LUA_TSTRING) { /* that produces a string? */
            return 1;  /* that is the message */
        } else {
            msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
        }
    }
    luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
    return 1;  /* return the traceback */
}
#endif

//
// Detect the presence of the root instance with passed methods. If successful,
// the stack will contain the object instance followed by the fields (which can
// be NIL if not found). When debug mode is enabled, a traceback function is also
// pushed onto the stack.
//
//     O F1 ... Fn T
//
static bool detect(lua_State *L, const char *root, const char *methods[])
{
    lua_getglobal(L, root); // Get the global variable on top of the stack (will always stay on top).
    if (lua_isnil(L, -1)) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't find root instance: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }

    for (int i = 0; methods[i]; ++i) { // Push the methods on stack
        lua_getfield(L, -(i + 1), methods[i]); // The table become farer and farer along the loop.
        if (!lua_isnil(L, -1)) {
            Log_write(LOG_LEVELS_INFO, "<VM> method '%s' found", methods[i]);
        } else {
            Log_write(LOG_LEVELS_WARNING, "<VM> method '%s' is missing", methods[i]);
        }
    }

#ifdef __DEBUG_VM_CALLS__
#ifndef __DEBUG_VM_USE_CUSTOM_TRACEBACK__
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_remove(L, -2);
#else
    lua_pushcfunction(L, error_handler);
#endif
#endif

    return true;
}

static void call(lua_State *L, Methods_t method, int nargs, int nresults)
{
    int index = METHOD_STACK_INDEX(method); // O F1 ... Fn
    if (lua_isnil(L, index)) {
        lua_pop(L, nargs); // Discard the unused arguments pushed by the caller.
        for (int i = 0; i < nresults; ++i) { // Push fake NIL results for the caller.
            lua_pushnil(L);
        }
        return;
    }
    lua_pushvalue(L, index);                // O F1 .. Fn A1 .. An     -> O F1 .. Fn A1 .. An F
    lua_pushvalue(L, OBJECT_STACK_INDEX);   // O F1 .. Fn A1 .. An F   -> O F1 .. Fn A1 .. An F O
    lua_rotate(L, -(nargs + 2), 2);         // O F1 .. Fn A1 .. An F O -> O F1 ... Fn F O A1 .. An

#ifdef __DEBUG_VM_CALLS__
    int result = lua_pcall(L, nargs + 1, nresults, TRACEBACK_STACK_INDEX);
    if (result != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error in call: %s", lua_tostring(L, -1));
    }
#else
    lua_call(L, nargs + 1, nresults);
#endif
}

static void timerpool_callback(Timer_t *timer, void *parameters)
{
    Interpreter_t *interpreter = (Interpreter_t *)parameters;

    // TODO: explicit access the VM state and expose `L` variable.

    lua_rawgeti(interpreter->state, LUA_REGISTRYINDEX, timer->value.callback);
#if 0
    if (lua_isnil(interpreter->state, -1)) {
        lua_pop(interpreter->state, -1);
        Log_write(LOG_LEVELS_ERROR, "<VM> can't find timer callback");
        return;
    }
#endif
#ifdef __DEBUG_VM_CALLS__
    int result = lua_pcall(interpreter->state, 0, 0, TRACEBACK_STACK_INDEX);
    if (result != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error in call: %s", lua_tostring(interpreter->state, -1));
    }
#else
    lua_call(interpreter->state, 0, 0);
#endif
}

bool Interpreter_initialize(Interpreter_t *interpreter, Configuration_t *configuration, const Environment_t *environment)
{
    interpreter->environment = environment;
    interpreter->gc_age = 0.0;
    interpreter->state = luaL_newstate();
    if (!interpreter->state) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't initialize interpreter");
        return false;
    }
    lua_atpanic(interpreter->state, panic);

    luaL_openlibs(interpreter->state);

    lua_pushlightuserdata(interpreter->state, (void *)environment); // Discard `const` qualifier.
    modules_initialize(interpreter->state, 1);

    // TODO: register a custom searcher for the "packed" archive feature.
    luaX_appendpath(interpreter->state, environment->base_path);

    TimerPool_initialize(&interpreter->timer_pool, timerpool_callback, interpreter); // Need to initialized before boot-script interpretation.

    int result = luaL_dostring(interpreter->state, BOOT_SCRIPT);
    if (result != 0) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't interpret boot script: %s", lua_tostring(interpreter->state, -1));
        lua_pop(interpreter->state, -1);
        lua_close(interpreter->state);
        return false;
    }

    if (!detect(interpreter->state, ROOT_INSTANCE, _methods)) {
        lua_close(interpreter->state);
        return false;
    }

    call(interpreter->state, METHOD_SETUP, 0, 1);
    Configuration_parse(interpreter->state, configuration);
    lua_pop(interpreter->state, 1); // Remove the configuration table from the stack.

    return true;
}

void Interpreter_init(Interpreter_t *interpreter)
{
    call(interpreter->state, METHOD_INIT, 0, 0);
}

void Interpreter_input(Interpreter_t *interpreter)
{
    call(interpreter->state, METHOD_INPUT, 0, 0);
}

void Interpreter_update(Interpreter_t *interpreter, const double delta_time)
{
    TimerPool_update(&interpreter->timer_pool, delta_time);

    interpreter->gc_age += delta_time;
    while (interpreter->gc_age >= GARBAGE_COLLECTION_PERIOD) { // Periodically collect GC.
        interpreter->gc_age -= GARBAGE_COLLECTION_PERIOD;

#ifdef __DEBUG_GARBAGE_COLLECTOR__
        Log_write(LOG_LEVELS_DEBUG, "<VM> performing periodical garbage collection");
        double start_time = (double)clock() / CLOCKS_PER_SEC;
#endif
        lua_gc(interpreter->state, LUA_GCCOLLECT, 0);
        TimerPool_gc(&interpreter->timer_pool);
#ifdef __DEBUG_GARBAGE_COLLECTOR__
        double elapsed = ((double)clock() / CLOCKS_PER_SEC) - start_time;
        Log_write(LOG_LEVELS_DEBUG, "<VM> garbage collection took %.3fs", elapsed);
#endif
    }

    lua_pushnumber(interpreter->state, delta_time);
    call(interpreter->state, METHOD_UPDATE, 1, 0);
}

void Interpreter_render(Interpreter_t *interpreter, const double ratio)
{
    lua_pushnumber(interpreter->state, ratio);
    call(interpreter->state, METHOD_RENDER, 1, 0);
}

void Interpreter_terminate(Interpreter_t *interpreter)
{
//    lua_pushnil(interpreter->state);
//    lua_setglobal(interpreter->state, "tofu");
    lua_settop(interpreter->state, 0);

    int result = luaL_dostring(interpreter->state, SHUTDOWN_SCRIPT);
    if (result != 0) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't interpret shutdown script: %s", lua_tostring(interpreter->state, -1));
        lua_pop(interpreter->state, 1);
    }

    lua_gc(interpreter->state, LUA_GCCOLLECT, 0);

    TimerPool_terminate(&interpreter->timer_pool);

    lua_close(interpreter->state);
}
