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

#include <config.h>
#include <core/io/display.h>
#include <core/io/fs.h>
#include <core/vm/modules.h>
#include <libs/imath.h>
#include <libs/log.h>

#include <limits.h>
#include <string.h>
#ifdef __DEBUG_GARBAGE_COLLECTOR__
  #include <time.h>
#endif
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif

#ifdef __DEBUG_VM_CALLS__
  #define TRACEBACK_STACK_INDEX   1
  #define OBJECT_STACK_INDEX      TRACEBACK_STACK_INDEX + 1
  #define METHOD_STACK_INDEX(m)   OBJECT_STACK_INDEX + 1 + (m)
#else
  #define OBJECT_STACK_INDEX      1
  #define METHOD_STACK_INDEX(m)   OBJECT_STACK_INDEX + 1 + (m)
#endif

static const uint8_t _boot_lua[] = {
#include "boot.inc"
};

typedef enum _Methods_t {
    METHOD_SETUP,
    METHOD_INIT,
    METHOD_DEINIT,
    METHOD_INPUT, // TODO: is the `input()` method useless? Probably...
    METHOD_UPDATE,
    METHOD_RENDER,
    Methods_t_CountOf
} Methods_t;

static const char *_methods[] = {
    "setup",
    "init",
    "deinit",
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

    char path_file[PATH_FILE_MAX];
    strcpy(path_file, "@");
    strcat(path_file, file);
    for (int i = 0; path_file[i] != '\0'; ++i) { // Replace `.' with '/` to map file system entry.
        if (path_file[i] == '.') {
            path_file[i] = FILE_PATH_SEPARATOR;
        }
    }
    strcat(path_file, ".lua");

    char *buffer = FS_load_as_string(fs, path_file + 1); // Skip the '@', we are using it for Lua to trace the file.

    if (buffer != NULL) {
        luaL_loadbuffer(L, buffer, strlen(buffer), path_file);
        free(buffer);
    } else {
        lua_pushfstring(L, "<VM> file '%s' not found", path_file);
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
static bool detect(lua_State *L, int index, const char *methods[])
{
    if (lua_isnil(L, index)) {
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

static int execute(lua_State *L, const char *script, const char *name, int nargs, int nresults)
{
    int loaded = luaL_loadbuffer(L, script, strlen(script), name);
    if (loaded != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error in execute: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return loaded;
    }
#ifdef __DEBUG_VM_CALLS__
    int called = lua_pcall(L, nargs, nresults, TRACEBACK_STACK_INDEX);
    if (called != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error in execute: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return called;
#else
    lua_call(L, nargs, nresults);
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

void parse(lua_State *L, Configuration_t *configuration)
{
    strncpy(configuration->title, DEFAULT_WINDOW_TITLE, MAX_CONFIGURATION_TITLE_LENGTH);
    *configuration = (Configuration_t){
            .width = DEFAULT_SCREEN_WIDTH,
            .height = DEFAULT_SCREEN_HEIGHT,
            .scale = DEFAULT_SCREEN_SCALE,
            .fullscreen = false,
            .vertical_sync = false,
            .fps = DEFAULT_FRAMES_PER_SECOND,
            .skippable_frames = DEFAULT_FRAMES_PER_SECOND / 5, // About 20% of the FPS amount.
            .fps_cap = -1, // No capping as a default. TODO: make it run-time configurable?
            .hide_cursor = true,
            .exit_key_enabled = true,
            .debug = true
        };

    if (!lua_istable(L, -1)) {
        Log_write(LOG_LEVELS_WARNING, "<VM> setup method returned no value");
        return;
    }

    lua_pushnil(L); // first key
    while (lua_next(L, -2)) { // Table is at the top, prior pushing NIL!
        const char *key = lua_tostring(L, -2); // uses 'key' (at index -2) and 'value' (at index -1)

        if (strcmp(key, "title") == 0) {
            strncpy(configuration->title, lua_tostring(L, -1), MAX_CONFIGURATION_TITLE_LENGTH);
        } else
        if (strcmp(key, "width") == 0) {
            configuration->width = lua_tointeger(L, -1);
        } else
        if (strcmp(key, "height") == 0) {
            configuration->height = lua_tointeger(L, -1);
        } else
        if (strcmp(key, "scale") == 0) {
            configuration->scale = lua_tointeger(L, -1);
        } else
        if (strcmp(key, "fullscreen") == 0) {
            configuration->fullscreen = lua_toboolean(L, -1);
        } else
        if (strcmp(key, "vertical-sync") == 0) {
            configuration->vertical_sync = lua_toboolean(L, -1);
        } else
        if (strcmp(key, "fps") == 0) {
            configuration->fps = lua_tointeger(L, -1);
            configuration->skippable_frames = configuration->fps / 5; // Keep synched. About 20% of the FPS amount.
        } else
        if (strcmp(key, "skippable-frames") == 0) {
            int suggested = configuration->fps / 5;
            configuration->skippable_frames = imin(lua_tointeger(L, -1), suggested); // TODO: not sure if `imin` or `imax`. :P
        } else
        if (strcmp(key, "fps-cap") == 0) {
            configuration->fps_cap = lua_tointeger(L, -1);
        } else
        if (strcmp(key, "hide-cursor") == 0) {
            configuration->hide_cursor = lua_toboolean(L, -1);
        } else
        if (strcmp(key, "exit-key-enabled") == 0) {
            configuration->exit_key_enabled = lua_toboolean(L, -1);
        } else
        if (strcmp(key, "debug") == 0) {
            configuration->debug = lua_toboolean(L, -1);
        }

        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
}

bool Interpreter_initialize(Interpreter_t *interpreter, const char *base_path, Configuration_t *configuration, const void *userdatas[])
{
    FS_initialize(&interpreter->file_system, base_path);
    TimerPool_initialize(&interpreter->timer_pool, timerpool_callback, interpreter->state);

    interpreter->gc_age = 0.0f;
    interpreter->state = luaL_newstate();
    if (!interpreter->state) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't initialize interpreter");
        return false;
    }
    lua_atpanic(interpreter->state, panic); // TODO: remove the panic handler?

    luaL_openlibs(interpreter->state);

    int nup = 0;
    for (int i = 0; userdatas[i]; ++i) {
        lua_pushlightuserdata(interpreter->state, (void *)userdatas[i]); // Discard `const` qualifier.
        nup += 1;
    }
    modules_initialize(interpreter->state, nup);

    lua_pushlightuserdata(interpreter->state, (void *)&interpreter->file_system);
    luaX_overridesearchers(interpreter->state, custom_searcher, 1);

#ifdef __DEBUG_VM_CALLS__
#ifndef __VM_USE_CUSTOM_TRACEBACK__
    lua_getglobal(interpreter->state, "debug");
    lua_getfield(interpreter->state, -1, "traceback");
    lua_remove(interpreter->state, -2);
#else
    lua_pushcfunction(interpreter->state, error_handler);
#endif
#endif

    int result = execute(interpreter->state, (const char *)_boot_lua, "@boot.lua", 0, 1); // Prefix '@' to trace as filename internally in Lua.
    if (result != 0) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't interpret boot script");
        lua_close(interpreter->state);
        return false;
    }

    if (!detect(interpreter->state, -1, _methods)) {
        lua_close(interpreter->state);
        return false;
    }

    call(interpreter->state, METHOD_SETUP, 0, 1);
    parse(interpreter->state, configuration);
    lua_pop(interpreter->state, 1); // Remove the configuration table from the stack.

    return true;
}

void Interpreter_terminate(Interpreter_t *interpreter)
{
    lua_settop(interpreter->state, 0);      // T O F1 ... Fn -> <empty>

    lua_gc(interpreter->state, LUA_GCCOLLECT, 0);
    lua_close(interpreter->state);

    TimerPool_terminate(&interpreter->timer_pool);
    FS_terminate(&interpreter->file_system);
}

bool Interpreter_init(Interpreter_t *interpreter) // TODO: we can move this into the "boot.lua" script.
{
    return call(interpreter->state, METHOD_INIT, 0, 0) == LUA_OK;
}

bool Interpreter_deinit(Interpreter_t *interpreter) // TODO: not sure it's really needed.
{
    return call(interpreter->state, METHOD_DEINIT, 0, 0) == LUA_OK;
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
