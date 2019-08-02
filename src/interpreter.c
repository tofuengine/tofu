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

#define SCRIPT_EXTENSION        ".wren"

#define ROOT_MODULE             "@root@"

#define ROOT_INSTANCE           "tofu"

#define BOOT_SCRIPT \
    "Tofu = require(\"tofu\")\n" \
    "local tofu = Tofu.new()\n"

#define SHUTDOWN_SCRIPT \
    "tofu = nil\n"

static void timerpool_update_callback(Timer_t *timer, void *parameters)
{
    Interpreter_t *interpreter = (Interpreter_t *)parameters;

    lua_rawgeti(interpreter->state, LUA_REGISTRYINDEX, timer->value.callback);
	if (lua_isnil(interpreter->state, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> can't find timer callback: %s", lua_tostring(interpreter->state, -1));
        return;
    }
    int result = lua_pcall(interpreter->state, 0, 0, 0);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error calling timer callback: %s", lua_tostring(interpreter->state, -1));
    }
}

bool Interpreter_initialize(Interpreter_t *interpreter, const Environment_t *environment)
{
    interpreter->environment = environment;
    interpreter->gc_age = 0.0;
    interpreter->state = luaL_newstate();
    if (!interpreter->state) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't initialize interpreter");
        return false;
    }
	luaL_openlibs(interpreter->state);
//    luaA_open(interpreter->state);
	util_initialize(interpreter->state);

    TimerPool_initialize(&interpreter->timer_pool, timerpool_update_callback, interpreter); // Need to initialized before boot-script interpretation.

    int result = luaL_dostring(interpreter->state, BOOT_SCRIPT);
    if (result != 0) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't interpret boot script: %d", result);
        lua_close(interpreter->state);
        return false;
    }

    return true;
}

void Interpreter_input(Interpreter_t *interpreter)
{
    lua_getglobal(interpreter->state, ROOT_INSTANCE); // TODO: define a helper "call" function.
	if (lua_isnil(interpreter->state, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> can't find root instance");
        return;
    }
    lua_getfield(interpreter->state, -1, "input");
	if (lua_isnil(interpreter->state, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> can't find input method");
        return;
    }
    int result = lua_pcall(interpreter->state, 0, 0, 0);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error calling input function: %s", lua_tostring(interpreter->state, -1));
    }
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

    lua_getglobal(interpreter->state, ROOT_INSTANCE); // TODO: define a helper "call" function.
	if (lua_isnil(interpreter->state, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> can't find root instance");
        return;
    }
    lua_getfield(interpreter->state, -1, "update");
	if (lua_isnil(interpreter->state, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> can't find update method");
        return;
    }
    lua_pushnumber(interpreter->state, delta_time);
    int result = lua_pcall(interpreter->state, 1, 0, 0);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error calling update function: %s", lua_tostring(interpreter->state, -1));
    }
}

void Interpreter_render(Interpreter_t *interpreter, const double ratio)
{
    lua_getglobal(interpreter->state, ROOT_INSTANCE); // TODO: define a helper "call" function.
	if (lua_isnil(interpreter->state, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> can't find root instance");
        return;
    }
    lua_getfield(interpreter->state, -1, "render");
	if (lua_isnil(interpreter->state, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> can't find render method");
        return;
    }
    lua_pushnumber(interpreter->state, ratio);
    int result = lua_pcall(interpreter->state, 1, 0, 0);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error calling render function: %s", lua_tostring(interpreter->state, -1));
    }
}

void Interpreter_terminate(Interpreter_t *interpreter)
{
    int result = luaL_dostring(interpreter->state, SHUTDOWN_SCRIPT);
    if (result != 0) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't interpret shutdown script");
    }

    lua_gc(interpreter->state, LUA_GCCOLLECT, 0);

    TimerPool_terminate(&interpreter->timer_pool);

    lua_close(interpreter->state);
}
