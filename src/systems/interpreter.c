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

#if 0
https://www.lua.org/manual/5.2/manual.html
https://www.lua.org/pil/27.3.2.html
https://www.lua.org/pil/25.2.html

https://nachtimwald.com/2014/07/26/calling-lua-from-c/
#endif

#include "interpreter.h"

#include <config.h>
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>
#include <modules/modules.h>

#include <stdint.h>
#ifdef __DEBUG_GARBAGE_COLLECTOR__
  #include <time.h>
#endif

#define LOG_CONTEXT "interpreter"

#ifdef __DEBUG_VM_CALLS__
  #define TRACEBACK_STACK_INDEX   1
  #define OBJECT_STACK_INDEX      (TRACEBACK_STACK_INDEX + 1)
  #define METHOD_STACK_INDEX(m)   (OBJECT_STACK_INDEX + 1 + (m))
#else
  #define OBJECT_STACK_INDEX      1
  #define METHOD_STACK_INDEX(m)   OBJECT_STACK_INDEX + 1 + (m)
#endif

#ifdef __VM_READER_BUFFER_SIZE__
  #define READER_CONTEXT_BUFFER_SIZE  1024
#else
  #define READER_CONTEXT_BUFFER_SIZE  __VM_READER_BUFFER_SIZE__
#endif

#ifdef DEBUG
  #define BOOT_SCRIPT "boot-debug"
#else
  #define BOOT_SCRIPT "boot-release"
#endif
static const char *_kickstart_lua = "return require(\"" BOOT_SCRIPT "\")";

typedef enum Entry_Point_Methods_e {
    METHOD_PROCESS,
    METHOD_UPDATE,
    METHOD_RENDER,
    Methods_t_CountOf
} Entry_Point_Methods_t;

static const char *_methods[] = { // We don't use a compound-literal on purpose here, since we are referring to the above enum.
    "process",
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
    lua_Warning_States_t *warning_state = (lua_Warning_States_t *)ud;
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
    if (!msg) {  /* is error object not a string? */
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

// [...] Every time lua_load needs another piece of the chunk, it calls the reader, passing along its data parameter.
// The reader must return a pointer to a block of memory with a new piece of the chunk and set size to the block size.
// The block must exist until the reader function is called again. To signal the end of the chunk, the reader must
// return NULL or set size to zero. The reader function may return pieces of any size greater than zero. [...]
typedef struct lua_Reader_Context_s {
    FS_Handle_t *handle;
    uint8_t buffer[READER_CONTEXT_BUFFER_SIZE];
} lua_Reader_Context_t;

static const char *_reader(lua_State *L, void *ud, size_t *size)
{
    lua_Reader_Context_t *context = (lua_Reader_Context_t *)ud;

    const size_t bytes_read = FS_read(context->handle, context->buffer, READER_CONTEXT_BUFFER_SIZE);
    if (bytes_read == 0) {
        *size = 0;
        return NULL;
    }

    *size = bytes_read;
    return (const char *)context->buffer;
}

static int _searcher(lua_State *L)
{
    Storage_t *storage = (Storage_t *)lua_touserdata(L, lua_upvalueindex(1));

    const char *name = lua_tostring(L, 1);

    char path[PLATFORM_PATH_MAX] = { 0 };
    path_lua_to_fs(path, name);

    FS_Handle_t *handle = Storage_open(storage, path + 1); // Don't waste storage cache! The module will be cached by Lua!
    if (!handle) {
        lua_pushfstring(L, "file `%s` can't be found into the storage", path + 1);
        return 1;
    }

    int result = lua_load(L, _reader, &(lua_Reader_Context_t){ .handle = handle }, path, NULL); // Set `mode` to `NULL`. Autodetect format to support both `text` and `binary` sources.

    FS_close(handle);

    if (result != LUA_OK) {
        lua_pushfstring(L, "failed w/ error #%d while loading file `%s`", result, path + 1); // Skip the `@` character.
        return 1;
    }

    lua_pushstring(L, path); // Return the path of the loaded file as second return value.

    return 2;
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

static inline int _raw_call(lua_State *L, int nargs, int nresults)
{
#ifdef __DEBUG_VM_CALLS__
    int result = lua_pcall(L, nargs, nresults, TRACEBACK_STACK_INDEX);
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

static inline int _method_call(lua_State *L, Entry_Point_Methods_t method, int nargs, int nresults)
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

    return _raw_call(L, nargs + 1, nresults); // Add the object instance to the arguments count.
}

Interpreter_t *Interpreter_create(const Storage_t *storage)
{
    Interpreter_t *interpreter = malloc(sizeof(Interpreter_t));
    if (!interpreter) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate interpreter");
        return NULL;
    }

    *interpreter = (Interpreter_t){ 0 };

    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "Lua: %s.%s.%s", LUA_VERSION_MAJOR, LUA_VERSION_MINOR, LUA_VERSION_RELEASE);

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

bool Interpreter_boot(Interpreter_t *interpreter, const void *userdatas[])
{
    int nup = 0;
    for (int i = 0; userdatas[i]; ++i) {
        lua_pushlightuserdata(interpreter->state, (void *)userdatas[i]); // Discard `const` qualifier.
        nup += 1;
    }
    modules_initialize(interpreter->state, nup);

    luaL_loadstring(interpreter->state, _kickstart_lua);
    int result = _raw_call(interpreter->state, 0, 1);
    if (result != LUA_OK) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't load boot script");
        return false;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "boot script loaded");

    if (!_detect(interpreter->state, -1, _methods)) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't detect entry-points");
        return false;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "entry-points detected");

    return true;
}

bool Interpreter_process(const Interpreter_t *interpreter, const char *events[])
{
//    if (events && events[0]) {
    if (events[0]) { // Create an event table, or `nil` when non presents.
        lua_newtable(interpreter->state);
        for (size_t i = 0; events[i]; ++i) {
            lua_pushstring(interpreter->state, events[i]);
            lua_rawseti(interpreter->state, -2, i + 1);
        }
    } else {
        lua_pushnil(interpreter->state);
    }
    return _method_call(interpreter->state, METHOD_PROCESS, 1, 0) == LUA_OK;
}

bool Interpreter_update(Interpreter_t *interpreter, float delta_time)
{
    lua_pushnumber(interpreter->state, (lua_Number)delta_time);
    if (_method_call(interpreter->state, METHOD_UPDATE, 1, 0) != LUA_OK) {
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
    // TODO: pass the default `Canvas` instance?
    lua_pushnumber(interpreter->state, (lua_Number)ratio); // TODO: is the `ratio` parameter really useful?
    return _method_call(interpreter->state, METHOD_RENDER, 1, 0) == LUA_OK;
}

bool Interpreter_call(const Interpreter_t *interpreter, int nargs, int nresults)
{
    return _raw_call(interpreter->state, nargs, nresults) == LUA_OK;
}
