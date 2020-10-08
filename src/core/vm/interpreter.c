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

#if 0
https://www.lua.org/manual/5.2/manual.html
https://www.lua.org/pil/27.3.2.html
https://www.lua.org/pil/25.2.html

https://nachtimwald.com/2014/07/26/calling-lua-from-c/
#endif

#include "interpreter.h"

#include <config.h>
#include <core/vm/modules.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <stdint.h>
#ifdef __DEBUG_GARBAGE_COLLECTOR__
  #include <time.h>
#endif

#define LOG_CONTEXT "interpreter"

#ifdef __DEBUG_VM_CALLS__
  #define TRACEBACK_STACK_INDEX   1
  #define OBJECT_STACK_INDEX      TRACEBACK_STACK_INDEX + 1
  #define METHOD_STACK_INDEX(m)   OBJECT_STACK_INDEX + 1 + (m)
#else
  #define OBJECT_STACK_INDEX      1
  #define METHOD_STACK_INDEX(m)   OBJECT_STACK_INDEX + 1 + (m)
#endif

static const uint8_t _boot_lua[] = {
#ifdef DEBUG
  #include "boot-debug.inc"
#else
  #include "boot-release.inc"
#endif
};

#define READER_BUFFER_SIZE  2048

typedef struct _Reader_Context_t {
    File_System_Handle_t *handle;
    char buffer[READER_BUFFER_SIZE];
} Reader_Context_t;

typedef enum _Methods_t {
    METHOD_INPUT,
    METHOD_UPDATE,
    METHOD_RENDER,
    Methods_t_CountOf
} Methods_t;

static const char *_methods[] = {
    "input",
    "update",
    "render",
    NULL
};

static void *_allocate(void *ud, void *ptr, size_t osize, size_t nsize)
{
    if (nsize == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, nsize);
}

static int _panic(lua_State *L)
{
    const char *message = lua_tostring(L, -1);
    if (!message) {
        message = "error object is not a string";
    }
    Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "%s", message);
    return 0; // return to Lua to abort
}

static void _warning(void *ud, const char *message, int tocont)
{
    Warning_States_t *warning_state = (Warning_States_t *)ud;
    if (*warning_state != WARNING_STATE_APPENDING && !tocont && *message == '@') {
        if (strcmp(message, "@off") == 0) {
            *warning_state = WARNING_STATE_DISABLED;
        } else
        if (strcmp(message, "@on") == 0) {
            *warning_state = WARNING_STATE_READY;
        }
        return;
    } else
    if (*warning_state == WARNING_STATE_DISABLED) {
        return;
    }

    if (*warning_state == WARNING_STATE_READY) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "%s", message);
    } else {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "\t%s", message);
    }

    if (tocont) {
        *warning_state = WARNING_STATE_APPENDING;
    } else {
        *warning_state = WARNING_STATE_READY;
    }
}

#ifdef __DEBUG_VM_CALLS__
#ifdef __VM_USE_CUSTOM_TRACEBACK__
static int _error_handler(lua_State *L)
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

static const char *_reader(lua_State *L, void *ud, size_t *size)
{
    Reader_Context_t *context = (Reader_Context_t *)ud;
    File_System_Handle_t *handle = context->handle;

    if (FS_eof(handle)) {
        return NULL;
    }

    *size = FS_read(handle, context->buffer, READER_BUFFER_SIZE);

    return context->buffer;
}

static int _load(const Storage_t *storage, const char *file, lua_State *L)
{
    File_System_Handle_t *handle = Storage_open(storage, file); // FIXME: move everything in `_reader`.
    if (!handle) {
        return LUA_ERRFILE;
    }

    char name[FILE_PATH_MAX] = "@"; // Prepend a `@`, required by Lua to track files.
    strcat(name, file);

    int result = lua_load(L, _reader, &(Reader_Context_t){ .handle = handle }, name, NULL); // nor `text` nor `binary`, autodetect.

    FS_close(handle);

    return result;
}

static int _searcher(lua_State *L)
{
    const Storage_t *storage = (const Storage_t *)lua_touserdata(L, lua_upvalueindex(1));

    const char *file = lua_tostring(L, 1);

    char path_file[FILE_PATH_MAX];
    strcpy(path_file, file);
    for (int i = 0; path_file[i] != '\0'; ++i) { // Replace `.' with '/` to map file system entry.
        if (path_file[i] == '.') {
            path_file[i] = FILE_SYSTEM_PATH_SEPARATOR;
        }
    }
    strcat(path_file, ".lua");

    int result = _load(storage, path_file, L);
    if (result != LUA_OK) {
        luaL_error(L, "failed w/ error #%d while loading file `%s`", result, path_file);
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
static bool _detect(lua_State *L, int index, const char *methods[])
{
    if (lua_isnil(L, index)) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't find root instance");
        lua_pop(L, 1);
        return false;
    }

    for (int i = 0; methods[i]; ++i) { // Push the methods on stack
        lua_getfield(L, -(i + 1), methods[i]); // The table become farer and farer along the loop.
        if (!lua_isnil(L, -1)) {
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "method `%s` found", methods[i]);
        } else {
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "method `%s` is missing", methods[i]);
        }
    }

    return true;
}

static int _execute(lua_State *L, const char *script, size_t size, const char *name, int nargs, int nresults)
{
    int result = luaL_loadbuffer(L, script, size, name);
    if (result != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "error #%d in load %s", result, lua_tostring(L, -1));
        lua_pop(L, 1);
        return result;
    }
#ifdef __DEBUG_VM_CALLS__
    result = lua_pcall(L, nargs, nresults, TRACEBACK_STACK_INDEX);
    if (result != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "error #%d in call: %s", result, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return result;
#else
    lua_call(L, nargs, nresults);
    return LUA_OK;
#endif
}

static int _call(lua_State *L, Methods_t method, int nargs, int nresults)
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
    int result = lua_pcall(L, nargs + 1, nresults, TRACEBACK_STACK_INDEX);
    if (result != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "error #%d in call: %s", result, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return result;
#else
    lua_call(L, nargs + 1, nresults);
    return LUA_OK;
#endif
}

Interpreter_t *Interpreter_create(const Storage_t *storage, const void *userdatas[])
{
    Interpreter_t *interpreter = malloc(sizeof(Interpreter_t));
    if (!interpreter) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate interpreter");
        return NULL;
    }

    *interpreter = (Interpreter_t){ 0 };

    interpreter->state = lua_newstate(_allocate, NULL); // No user-data is passed.
    if (!interpreter->state) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create interpreter VM");
        free(interpreter);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "interpreter VM %p created", interpreter->state);

    lua_atpanic(interpreter->state, _panic); // Set a custom panic-handler, just like `luaL_newstate()`.
    lua_setwarnf(interpreter->state, _warning, &interpreter->warning_state); // (and a custom warning-handler, too).

#if __VM_GARBAGE_COLLECTOR_TYPE__ == GC_INCREMENTAL
    lua_gc(interpreter->state, LUA_GCINC, 0, 0, 0);
#elif __VM_GARBAGE_COLLECTOR_TYPE__ == GC_GENERATIONAL
    lua_gc(interpreter->state, LUA_GCGEN, 0, 0);
#endif

#if __VM_GARBAGE_COLLECTOR_MODE__ != GC_AUTOMATIC
    lua_gc(interpreter->state, LUA_GCSTOP); // Garbage collector is enabled, as a default.
#endif

    luaX_openlibs(interpreter->state); // Custom loader, only selected libraries.

    int nup = 0;
    for (int i = 0; userdatas[i]; ++i) {
        lua_pushlightuserdata(interpreter->state, (void *)userdatas[i]); // Discard `const` qualifier.
        nup += 1;
    }
    lua_pushlightuserdata(interpreter->state, interpreter); // Push the interpreter itself as first upvalue.
    modules_initialize(interpreter->state, nup + 1); // Take into account the self-pushed interpreter pointer.

    lua_pushlightuserdata(interpreter->state, (void *)storage);
    luaX_overridesearchers(interpreter->state, _searcher, 1);

#ifdef __DEBUG_VM_CALLS__
#ifndef __VM_USE_CUSTOM_TRACEBACK__
    lua_getglobal(interpreter->state, "debug");
    lua_getfield(interpreter->state, -1, "traceback");
    lua_remove(interpreter->state, -2);
#else
    lua_pushcfunction(interpreter->state, _error_handler);
#endif
#endif

    size_t version = (size_t)lua_version(interpreter->state);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "Lua: %d.%d", version / 100, version % 100);

    int result = _execute(interpreter->state, (const char *)_boot_lua, sizeof(_boot_lua) / sizeof(char), "@boot.lua", 0, 1); // Prefix '@' to trace as filename internally in Lua.
    if (result != 0) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't interpret boot script");
        lua_close(interpreter->state);
        free(interpreter);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "boot script executed");

    if (!_detect(interpreter->state, -1, _methods)) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't detect entry-points");
        lua_close(interpreter->state);
        free(interpreter);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry-points detected");

    return interpreter;
}

void Interpreter_destroy(Interpreter_t *interpreter)
{
    lua_settop(interpreter->state, 0); // T O F1 ... Fn -> <empty>
    lua_gc(interpreter->state, LUA_GCCOLLECT); // Full GC cycle to trigger resource release.
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "interpreter VM %p garbage-collected", interpreter->state);

    lua_close(interpreter->state);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "interpreter VM %p destroyed", interpreter->state);

    free(interpreter);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "interpreter freed");
}

bool Interpreter_input(const Interpreter_t *interpreter)
{
    return _call(interpreter->state, METHOD_INPUT, 0, 0) == LUA_OK;
}

bool Interpreter_update(Interpreter_t *interpreter, float delta_time)
{
    lua_pushnumber(interpreter->state, delta_time);
    if (_call(interpreter->state, METHOD_UPDATE, 1, 0) != LUA_OK) {
        return false;
    }

#if __VM_GARBAGE_COLLECTOR_MODE__ == GC_CONTINUOUS
    interpreter->gc_step_age += delta_time;
    while (interpreter->gc_step_age >= GC_CONTINUOUS_STEP_PERIOD) {
        interpreter->gc_step_age -= GC_CONTINUOUS_STEP_PERIOD;

        lua_gc(interpreter->state, LUA_GCSTEP, 0); // Basic step.
    }
#endif


#if defined(__VM_GARBAGE_COLLECTOR_PERIODIC_COLLECT__) || defined(__DEBUG_GARBAGE_COLLECTOR__)
    interpreter->gc_age += delta_time;
    while (interpreter->gc_age >= GC_COLLECTION_PERIOD) { // Periodically collect GC.
        interpreter->gc_age -= GC_COLLECTION_PERIOD;

#ifdef __VM_GARBAGE_COLLECTOR_PERIODIC_COLLECT__
#ifdef __DEBUG_GARBAGE_COLLECTOR__
        float start_time = (float)clock() / CLOCKS_PER_SEC;
        int pre = lua_gc(interpreter->state, LUA_GCCOUNT);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "performing periodical garbage collection (%dKb of memory in use)", pre);
#endif
        lua_gc(interpreter->state, LUA_GCCOLLECT);
#ifdef __DEBUG_GARBAGE_COLLECTOR__
        int post = lua_gc(interpreter->state, LUA_GCCOUNT);
        float elapsed = ((float)clock() / CLOCKS_PER_SEC) - start_time;
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "garbage collection took %.3fs (memory used %dKb, %dKb freed)", elapsed, post, pre - post);
#endif
#else
#ifdef __DEBUG_GARBAGE_COLLECTOR__
        int count = lua_gc(interpreter->state, LUA_GCCOUNT);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "memory usage is %dKb", count);
#endif
#endif
    }
#endif

    return true;
}

bool Interpreter_render(const Interpreter_t *interpreter, float ratio)
{
    lua_pushnumber(interpreter->state, ratio);
    return _call(interpreter->state, METHOD_RENDER, 1, 0) == LUA_OK;
}

bool Interpreter_call(const Interpreter_t *interpreter, int nargs, int nresults)
{
    lua_State *L = interpreter->state;
#ifdef __DEBUG_VM_CALLS__
    int result = lua_pcall(L, nargs, nresults, TRACEBACK_STACK_INDEX);
    if (result != LUA_OK) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "error #%d in execute: %s", result, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return result == LUA_OK ? true : false;
#else
    lua_call(L, nargs, nresults);
    return true;
#endif
}
