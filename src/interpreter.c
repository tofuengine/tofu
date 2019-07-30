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

static const char *get_filename_extension(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot || dot == name) {
        return "";
    }
    return dot + 1;
}

static char *load_module_function(WrenVM *vm, const char *name)
{
    // User-defined modules are specified as "relative" paths (where "./" indicates the current directory)
    if (strncmp(name, "./", 2) != 0) {
        Log_write(LOG_LEVELS_INFO, "<VM> loading built-in module '%s'", name);

        for (int i = 0; _modules[i].module != NULL; ++i) {
            const Module_Entry_t *entry = &_modules[i];
            if (strcmp(name, entry->module) == 0) {
                size_t size = (strlen(entry->script) + 1) * sizeof(char);
                void *ptr = malloc(size);
                if (ptr) {
                    memcpy(ptr, entry->script, size);
                }
                return ptr;
            }
        }
        return NULL;
    }

    const Environment_t *environment = (const Environment_t *)wrenGetUserData(vm);

    char pathfile[PATH_FILE_MAX]; // Build the absolute path.
    strcpy(pathfile, environment->base_path);
    if (strlen(get_filename_extension(name)) > 0) { // Has a valid extension, use it.
        strcat(pathfile, name);
    } else {
        strcat(pathfile, name + 2);
        strcat(pathfile, SCRIPT_EXTENSION);
    }

    Log_write(LOG_LEVELS_INFO, "<VM> loading module '%s'", pathfile);
    return file_load_as_string(pathfile, "rt");
}

static void write_function(WrenVM *vm, const char *text)
{
    Log_write(LOG_LEVELS_TRACE, text);
}

static void error_function(WrenVM* vm, WrenErrorType type, const char *module, int line, const char *message)
{
    if (type == WREN_ERROR_COMPILE) {
        Log_write(LOG_LEVELS_ERROR, "<VM> compile error: [%s@%d] %s", module, line, message);
    } else if (type == WREN_ERROR_RUNTIME) {
        Log_write(LOG_LEVELS_ERROR, "<VM> runtime error: %s", message);
    } else if (type == WREN_ERROR_STACK_TRACE) {
        Log_write(LOG_LEVELS_ERROR, "  [%s@%d] in %s", module, line, message);
    }
}

static WrenForeignClassMethods bind_foreign_class_function(WrenVM* vm, const char *module, const char *className)
{
//    Log_write(LOG_LEVELS_TRACE, "%s %s %d %s", module, className);
    for (int i = 0; _classes[i].module != NULL; ++i) {
        const Class_Entry_t *entry = &_classes[i];
        if ((strcmp(module, entry->module) == 0) &&
            (strcmp(className, entry->className) == 0)) {
                return (WrenForeignClassMethods){
                        .allocate = entry->allocate,
                        .finalize = entry->finalize
                    };
            }
    }
    return (WrenForeignClassMethods){};
}

static WrenForeignMethodFn bind_foreign_method_function(WrenVM *vm, const char *module, const char* className, bool isStatic, const char *signature)
{
//    Log_write(LOG_LEVELS_TRACE, "%s %s %d %s", module, className, isStatic, signature);
    for (int i = 0; _methods[i].module != NULL; ++i) {
        const Method_Entry_t *entry = &_methods[i];
        if ((strcmp(module, entry->module) == 0) &&
            (strcmp(className, entry->className) == 0) &&
            (isStatic == entry->isStatic) &&
            (strcmp(signature, entry->signature) == 0)) {
                return entry->method;
            }
    }
    return NULL;
}

static void timerpool_call_callback(Timer_t *timer, void *parameters)
{
    Interpreter_t *interpreter = (Interpreter_t *)parameters;

    lua_rawgeti(interpreter->state, LUA_REGISTRYINDEX, timer->value.callback);
    int result = lua_pcall(interpreter->state, 0, 0, 0);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error calling timer callback: %s", lua_tostring(interpreter->state, -1));
    }
}

static void timerpool_release_callback(Timer_t *timer, void *parameters)
{
    Interpreter_t *interpreter = (Interpreter_t *)parameters;

    luaL_unref(interpreter->state, LUA_REGISTRYINDEX, timer->value.callback);
}

bool Interpreter_initialize(Interpreter_t *interpreter, const Environment_t *environment)
{
    interpreter->environment = environment;

    interpreter->state = luaL_newstate();
    if (!interpreter->state) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't initialize interpreter");
        return false;
    }
	luaL_openlibs(interpreter->state);
//    luaA_open(interpreter->state);

    interpreter->gc_age = 0.0;

    TimerPool_initialize(&interpreter->timer_pool); // Need to initialized before boot-script interpretation.

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
        Log_write(LOG_LEVELS_ERROR, "<VM> could not find root instance");
        return;
    }
    lua_getfield(interpreter->state, -1, "input");
	if (lua_isnil(interpreter->state, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> could not find input method");
        return;
    }
    int result = lua_pcall(interpreter->state, 0, 0, 0);
    if (result != 0) {
        Log_write(LOG_LEVELS_ERROR, "<VM> error calling input function: %s", lua_tostring(interpreter->state, -1));
    }
}

void Interpreter_update(Interpreter_t *interpreter, const double delta_time)
{
    TimerPool_update(&interpreter->timer_pool, delta_time, timerpool_call_callback, interpreter);

    interpreter->gc_age += delta_time;
    while (interpreter->gc_age >= GARBAGE_COLLECTION_PERIOD) { // Periodically collect GC.
        interpreter->gc_age -= GARBAGE_COLLECTION_PERIOD;

#ifdef __DEBUG_GARBAGE_COLLECTOR__
        Log_write(LOG_LEVELS_DEBUG, "<VM> performing periodical garbage collection");
        double start_time = (double)clock() / CLOCKS_PER_SEC;
#endif
        lua_gc(interpreter->state, LUA_GCCOLLECT, 0);
        TimerPool_gc(&interpreter->timer_pool, timerpool_release_callback, interpreter);
#ifdef __DEBUG_GARBAGE_COLLECTOR__
        double elapsed = ((double)clock() / CLOCKS_PER_SEC) - start_time;
        Log_write(LOG_LEVELS_DEBUG, "<VM> garbage collection took %.3fs", elapsed);
#endif
    }

    lua_getglobal(interpreter->state, ROOT_INSTANCE); // TODO: define a helper "call" function.
	if (lua_isnil(interpreter->state, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> could not find root instance");
        return;
    }
    lua_getfield(interpreter->state, -1, "update");
	if (lua_isnil(interpreter->state, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> could not find update method");
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
        Log_write(LOG_LEVELS_ERROR, "<VM> could not find root instance");
        return;
    }
    lua_getfield(interpreter->state, -1, "render");
	if (lua_isnil(interpreter->state, -1)) {
        Log_write(LOG_LEVELS_ERROR, "<VM> could not find render method");
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

    TimerPool_terminate(&interpreter->timer_pool, timerpool_release_callback, interpreter);

    lua_close(interpreter->state);
}
