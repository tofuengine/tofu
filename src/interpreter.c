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

typedef struct _Entry_Point_Method_t {
    const char *name;
    bool available;
    bool failure;
} Entry_Point_Method_t;

typedef enum _Entry_Point_Methods_t {
    METHOD_SETUP,
    METHOD_INIT,
    METHOD_INPUT,
    METHOD_UPDATE,
    METHOD_RENDER,
    Entry_Point_Methods_t_CountOf
} Entry_Point_Methods_t;

static Entry_Point_Method_t methods[] = {
    { "setup", false, false },
    { "init", false, false },
    { "input", false, false },
    { "update", false, false },
    { "render", false, false },
    { NULL, false, false }
};

static bool detect(lua_State *L, Entry_Point_Method_t *methods)
{
    lua_getglobal(L, ROOT_INSTANCE); // Get the global variable on top of the stack (will always stay on top).
    if (lua_isnil(L, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> can't find root instance: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }

    for (; methods->name; ++methods) {
        lua_getfield(L, -1, methods->name);
        methods->available = !lua_isnil(L, -1);
        if (methods->available) {
            Log_write(LOG_LEVELS_INFO, "<VM> method '%s' found", methods->name);
        } else {
            Log_write(LOG_LEVELS_WARNING, "<VM> method '%s' is missing", methods->name);
        }
        lua_pop(L, 1);
    }

    return true;
}

static int call(lua_State *L, Entry_Point_Method_t *method, int nargs, int nresults)
{
    if (!method->available || method->failure) {
        lua_pop(L, nargs); // Discard the unused arguments pushed by the caller.
        for (int i = 0; i < nresults; ++i) {
            lua_pushnil(L);
        }
        return LUA_OK;
    }
    lua_getfield(L, -(nargs + 1), method->name); // O A1 .. An -> O A1 .. An F
    lua_pushvalue(L, -(nargs + 2)); // O A1 .. An F -> O A1 .. An F O
    lua_rotate(L, -(nargs + 2), 2); // O A1 .. An F O -> O F O A1 .. An
    int result = lua_pcall(L, nargs + 1, nresults, 0); // Acoount for the `self` object.
    if (result != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error calling method '%s': %s", method->name, lua_tostring(L, -1));
        lua_pop(L, 1);

        Log_write(LOG_LEVELS_WARNING, "<VM> setting failure flag for method '%s'", method->name);
        method->failure = true;
    }
    return result;
}

static void timerpool_update_callback(Timer_t *timer, void *parameters)
{
    Interpreter_t *interpreter = (Interpreter_t *)parameters;

    lua_rawgeti(interpreter->state, LUA_REGISTRYINDEX, timer->value.callback);
#if 0
    if (lua_isnil(interpreter->state, -1)) {
        lua_pop(interpreter->state, -1);
        Log_write(LOG_LEVELS_ERROR, "<VM> can't find timer callback");
        return;
    }
#endif
    int result = lua_pcall(interpreter->state, 0, 0, 0);
    if (result != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error calling timer callback: %s", lua_tostring(interpreter->state, -1));
    }
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
    luaL_openlibs(interpreter->state);

    lua_pushlightuserdata(interpreter->state, (void *)environment); // Discard `const` qualifier.
    modules_initialize(interpreter->state, 1);

    // TODO: register a custom searcher for the "packed" archive feature.
    luaX_appendpath(interpreter->state, environment->base_path);

    TimerPool_initialize(&interpreter->timer_pool, timerpool_update_callback, interpreter); // Need to initialized before boot-script interpretation.

    int result = luaL_dostring(interpreter->state, BOOT_SCRIPT);
    if (result != 0) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't interpret boot script: %s", lua_tostring(interpreter->state, -1));
        lua_pop(interpreter->state, -1);
        lua_close(interpreter->state);
        return false;
    }

    if (!detect(interpreter->state, methods)) {
        lua_close(interpreter->state);
        return false;
    }

    result = call(interpreter->state, &methods[METHOD_SETUP], 0, 1);
    if (result == LUA_OK) {
        Configuration_parse(interpreter->state, configuration);
        lua_pop(interpreter->state, 1); // Remove the configuration table from the stack.
    }

    return true;
}

void Interpreter_init(Interpreter_t *interpreter)
{
    call(interpreter->state, &methods[METHOD_INIT], 0, 0);
}

void Interpreter_input(Interpreter_t *interpreter)
{
    call(interpreter->state, &methods[METHOD_INPUT], 0, 0);
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
        //luaX_dump(interpreter->state);
#endif
        lua_gc(interpreter->state, LUA_GCCOLLECT, 0);
        TimerPool_gc(&interpreter->timer_pool);
#ifdef __DEBUG_GARBAGE_COLLECTOR__
        double elapsed = ((double)clock() / CLOCKS_PER_SEC) - start_time;
        Log_write(LOG_LEVELS_DEBUG, "<VM> garbage collection took %.3fs", elapsed);
        //luaX_dump(interpreter->state);
#endif
    }

    lua_pushnumber(interpreter->state, delta_time);
    call(interpreter->state, &methods[METHOD_UPDATE], 1, 0);
}

void Interpreter_render(Interpreter_t *interpreter, const double ratio)
{
    lua_pushnumber(interpreter->state, ratio);
    call(interpreter->state, &methods[METHOD_RENDER], 1, 0);
}

void Interpreter_terminate(Interpreter_t *interpreter)
{
//    lua_pushnil(interpreter->state);
//    lua_setglobal(interpreter->state, "tofu");
    int result = luaL_dostring(interpreter->state, SHUTDOWN_SCRIPT);
    if (result != 0) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't interpret shutdown script: %s", lua_tostring(interpreter->state, -1));
        lua_pop(interpreter->state, 1);
    }

    lua_gc(interpreter->state, LUA_GCCOLLECT, 0);

    TimerPool_terminate(&interpreter->timer_pool);

    lua_close(interpreter->state);
}
