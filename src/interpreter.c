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
#include "fs.h"
#include "log.h"
#include "modules.h"

#include <limits.h>
#include <string.h>
#ifdef __DEBUG_GARBAGE_COLLECTOR__
  #include <time.h>
#endif
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif

#define ROOT_INSTANCE           "main"

#define BOOT_SCRIPT \
    "local Main = require(\"main\")\n" \
    "main = Main.new()\n"

#define SHUTDOWN_SCRIPT \
    "main = nil\n"

#ifdef __DEBUG_VM_CALLS__
  #define TRACEBACK_STACK_INDEX   1
  #define OBJECT_STACK_INDEX      TRACEBACK_STACK_INDEX + 1
  #define METHOD_STACK_INDEX(m)   OBJECT_STACK_INDEX + 1 + (m)
#else
  #define OBJECT_STACK_INDEX      1
  #define METHOD_STACK_INDEX(m)   OBJECT_STACK_INDEX + 1 + (m)
#endif

typedef enum _Methods_t {
    METHOD_SETUP,
    METHOD_INIT,
    METHOD_INPUT, // TODO: is the `input()` method useless? Probably...
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
    lua_pop(L, 1);
    return 0; // return to Lua to abort
}

#ifdef __DEBUG_VM_CALLS__
#ifdef __VM_USE_CUSTOM_TRACEBACK__
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
#endif

static int custom_searcher(lua_State *L)
{
    const File_System_t *fs = (const File_System_t *)lua_touserdata(L, lua_upvalueindex(1));

    const char *file = lua_tostring(L, 1);

    char path_file[PATH_FILE_MAX] = {};
    strcpy(path_file, file);
    for (int i = 0; path_file[i] != '\0'; ++i) { // Replace `.' with '/` to map file system entry.
        if (path_file[i] == '.') {
            path_file[i] = FILE_PATH_SEPARATOR;
        }
    }
    strcat(path_file, ".lua");

    char *buffer = FS_load_as_string(fs, path_file);

    if (buffer != NULL) {
        luaL_loadstring(L, buffer);
        free(buffer);
    } else {
        lua_pushstring(L, "Error: file not found");
    }

    return 1;
}

//
// Detect the presence of the root instance with passed methods. If successful,
// the stack will contain the object instance followed by the fields (which can
// be NIL if not found). The traceback function is already on the stack.
//
//     T O F1 ... Fn
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

    return true;
}

static int execute(lua_State *L, const char *script)
{
    int loaded = luaL_loadstring(L, script);
    if (loaded != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error in execute: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return loaded;
    }
#ifdef __DEBUG_VM_CALLS__
    int called = lua_pcall(L, 0, 0, TRACEBACK_STACK_INDEX);
    if (called != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error in execute: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return called;
#else
    lua_call(L, 0, 0);
    return LUA_OK;
#endif
}

static int call(lua_State *L, Methods_t method, int nargs, int nresults)
{
    int index = METHOD_STACK_INDEX(method); // T O F1 .. Fn
    if (lua_isnil(L, index)) {
        lua_pop(L, nargs); // Discard the unused arguments pushed by the caller.
        for (int i = 0; i < nresults; ++i) { // Push fake NIL results for the caller.
            lua_pushnil(L);
        }
        return LUA_OK;
    }
    lua_pushvalue(L, index);                // T O F1 ... Fn A1 ... An     -> T O F1 ... Fn A1 ... An F
    lua_pushvalue(L, OBJECT_STACK_INDEX);   // T O F1 ... Fn A1 ... An F   -> T O F1 ... Fn A1 ... An F O
    lua_rotate(L, -(nargs + 2), 2);         // T O F1 ... Fn A1 ... An F O -> T O F1 ... Fn F O A1 ... An

#ifdef __DEBUG_VM_CALLS__
    int called = lua_pcall(L, nargs + 1, nresults, TRACEBACK_STACK_INDEX);
    if (called != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error in call: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return called;
#else
    lua_call(L, nargs + 1, nresults);
    return LUA_OK;
#endif
}

static bool timerpool_callback(Timer_t *timer, void *parameters)
{
    lua_State *L = (lua_State *)parameters;

    lua_rawgeti(L, LUA_REGISTRYINDEX, BUNDLE_TO_INT(timer->bundle));
#if 0
    if (lua_isnil(L, -1)) {
        lua_pop(L, -1);
        Log_write(LOG_LEVELS_ERROR, "<VM> can't find timer callback");
        return;
    }
#endif
#ifdef __DEBUG_VM_CALLS__
    int result = lua_pcall(L, 0, 0, TRACEBACK_STACK_INDEX);
    if (result != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error in call: %s", lua_tostring(L, -1));
    }
#else
    lua_call(L, 0, 0);
    int result = LUA_OK;
#endif
    return result == LUA_OK;
}

bool Interpreter_initialize(Interpreter_t *interpreter, Configuration_t *configuration, const Environment_t *environment, const Display_t *display)
{
    interpreter->environment = environment;
    interpreter->gc_age = 0.0f;
    interpreter->state = luaL_newstate();
    if (!interpreter->state) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't initialize interpreter");
        return false;
    }
    lua_atpanic(interpreter->state, panic); // TODO: remove the panic handler?

    luaL_openlibs(interpreter->state);

    lua_pushlightuserdata(interpreter->state, (void *)environment); // Discard `const` qualifier.
    lua_pushlightuserdata(interpreter->state, (void *)display);
    lua_pushlightuserdata(interpreter->state, (void *)interpreter);
    modules_initialize(interpreter->state, 3);

#if 0
    luaX_appendpath(interpreter->state, environment->base_path);
#else
    lua_pushlightuserdata(interpreter->state, (void *)&environment->fs);
    luaX_overridesearchers(interpreter->state, custom_searcher, 1);
#endif

#ifdef __DEBUG_VM_CALLS__
#ifndef __VM_USE_CUSTOM_TRACEBACK__
    lua_getglobal(interpreter->state, "debug");
    lua_getfield(interpreter->state, -1, "traceback");
    lua_remove(interpreter->state, -2);
#else
    lua_pushcfunction(interpreter->state, error_handler);
#endif
#endif

    int result = execute(interpreter->state, BOOT_SCRIPT);
    if (result != 0) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't interpret boot script");
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

    TimerPool_initialize(&interpreter->timer_pool, timerpool_callback, interpreter->state);

    return true;
}

void Interpreter_terminate(Interpreter_t *interpreter)
{
#ifdef SHUTDOWN_SCRIPT
    lua_settop(interpreter->state, 1);      // T O F1 ... Fn -> T
    int result = execute(interpreter->state, SHUTDOWN_SCRIPT);
    if (result != 0) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't interpret shutdown script");
    }
    lua_settop(interpreter->state, 0);      // T -> <empty>
#else
    lua_pushnil(interpreter->state);
    lua_setglobal(interpreter->state, ROOT_INSTANCE); // Just set the (global) *root* instance to `nil`.
#endif
    lua_gc(interpreter->state, LUA_GCCOLLECT, 0);
    lua_close(interpreter->state);

    TimerPool_terminate(&interpreter->timer_pool);
}

bool Interpreter_init(Interpreter_t *interpreter)
{
    return call(interpreter->state, METHOD_INIT, 0, 0) == LUA_OK;
}

bool Interpreter_input(Interpreter_t *interpreter)
{
    return call(interpreter->state, METHOD_INPUT, 0, 0) == LUA_OK;
}

bool Interpreter_update(Interpreter_t *interpreter, float delta_time)
{
    if (!TimerPool_update(&interpreter->timer_pool, delta_time)) {
        return false;
    }

    lua_pushnumber(interpreter->state, delta_time);
    if (call(interpreter->state, METHOD_UPDATE, 1, 0) != LUA_OK) {
        return false;
    }

    interpreter->gc_age += delta_time;
    while (interpreter->gc_age >= GARBAGE_COLLECTION_PERIOD) { // Periodically collect GC.
        interpreter->gc_age -= GARBAGE_COLLECTION_PERIOD;

#ifdef __DEBUG_GARBAGE_COLLECTOR__
        Log_write(LOG_LEVELS_DEBUG, "<VM> performing periodical garbage collection");
        float start_time = (float)clock() / CLOCKS_PER_SEC;
#endif
        lua_gc(interpreter->state, LUA_GCCOLLECT, 0);
        TimerPool_gc(&interpreter->timer_pool);
#ifdef __DEBUG_GARBAGE_COLLECTOR__
        float elapsed = ((float)clock() / CLOCKS_PER_SEC) - start_time;
        Log_write(LOG_LEVELS_DEBUG, "<VM> garbage collection took %.3fs", elapsed);
#endif
    }

    return true;
}

bool Interpreter_render(Interpreter_t *interpreter, float ratio)
{
    lua_pushnumber(interpreter->state, ratio);
    return call(interpreter->state, METHOD_RENDER, 1, 0) == LUA_OK;
}

bool Interpreter_call(Interpreter_t *interpreter, int nargs, int nresults)
{
    lua_State *L = interpreter->state;
#ifdef __DEBUG_VM_CALLS__
    int called = lua_pcall(L, nargs, nresults, TRACEBACK_STACK_INDEX);
    if (called != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error in execute: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return called == LUA_OK ? true : false;
#else
    lua_call(L, nargs, nresults);
    return true;
#endif
}
