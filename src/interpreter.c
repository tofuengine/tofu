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

#include "interpreter.h"

#include "config.h"
#include "file.h"
#include "log.h"
#include "memory.h"
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
    "import \"./tofu\" for Tofu\n" \
    "var tofu = Tofu.new()\n"

#define SHUTDOWN_SCRIPT \
    "tofu = null\n"

static const char *get_filename_extension(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot || dot == name) {
        return "";
    }
    return dot + 1;
}

static void *reallocate_function(void *ptr, size_t size)
{
    return Memory_realloc(ptr, size);
}

static char *load_module_function(WrenVM *vm, const char *name)
{
    // User-defined modules are specified as "relative" paths (where "./" indicates the current directory)
    if (strncmp(name, "./", 2) != 0) {
        Log_write(LOG_LEVELS_INFO, "<VM> loading built-in module '%s'", name);

        for (int i = 0; _modules[i].module != NULL; ++i) {
            const Module_Entry_t *entry = &_modules[i];
            if (strcmp(name, entry->module) == 0) {
                return Memory_clone(entry->script, strlen(entry->script) + 1);
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

    wrenEnsureSlots(interpreter->vm, 1);
    wrenSetSlotHandle(interpreter->vm, 0, timer->value.callback);
    wrenCall(interpreter->vm, interpreter->handles[CALL]);
}

static void timerpool_release_callback(Timer_t *timer, void *parameters)
{
    Interpreter_t *interpreter = (Interpreter_t *)parameters;

    wrenReleaseHandle(interpreter->vm, timer->value.callback);
}

bool Interpreter_initialize(Interpreter_t *interpreter, const Environment_t *environment)
{
    interpreter->environment = environment;

    WrenConfiguration vm_configuration;
    wrenInitConfiguration(&vm_configuration);
    vm_configuration.reallocateFn = reallocate_function;
    vm_configuration.loadModuleFn = load_module_function;
    vm_configuration.bindForeignClassFn = bind_foreign_class_function;
    vm_configuration.bindForeignMethodFn = bind_foreign_method_function;
    vm_configuration.writeFn = write_function;
    vm_configuration.errorFn = error_function;

    interpreter->vm = wrenNewVM(&vm_configuration);
    if (!interpreter->vm) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't initialize interpreter");
        return false;
    }

    wrenSetUserData(interpreter->vm, (void *)environment); // HACK: we discard the const qualifier :(

    interpreter->gc_age = 0.0;

    TimerPool_initialize(&interpreter->timer_pool, 32); // Need to initialized before boot-script interpretation.

    WrenInterpretResult result = wrenInterpret(interpreter->vm, ROOT_MODULE, BOOT_SCRIPT);
    if (result != WREN_RESULT_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't interpret boot script");
        wrenFreeVM(interpreter->vm);
        return false;
    }

    wrenEnsureSlots(interpreter->vm, 1);
    wrenGetVariable(interpreter->vm, ROOT_MODULE, ROOT_INSTANCE, 0);
    interpreter->handles[RECEIVER] = wrenGetSlotHandle(interpreter->vm, 0);

    interpreter->handles[INPUT] = wrenMakeCallHandle(interpreter->vm, "input()");
    interpreter->handles[UPDATE] = wrenMakeCallHandle(interpreter->vm, "update(_)");
    interpreter->handles[RENDER] = wrenMakeCallHandle(interpreter->vm, "render(_)");
    interpreter->handles[CALL] = wrenMakeCallHandle(interpreter->vm, "call()"); // Generic, for timers.

    return true;
}

void Interpreter_input(Interpreter_t *interpreter)
{
    wrenEnsureSlots(interpreter->vm, 1);
    wrenSetSlotHandle(interpreter->vm, 0, interpreter->handles[RECEIVER]);
    wrenCall(interpreter->vm, interpreter->handles[INPUT]);
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
        wrenCollectGarbage(interpreter->vm);
        TimerPool_gc(&interpreter->timer_pool, timerpool_release_callback, interpreter);
#ifdef __DEBUG_GARBAGE_COLLECTOR__
        double elapsed = ((double)clock() / CLOCKS_PER_SEC) - start_time;
        Log_write(LOG_LEVELS_DEBUG, "<VM> garbage collection took %.3fs", elapsed);
#endif
    }

    wrenEnsureSlots(interpreter->vm, 2);
    wrenSetSlotHandle(interpreter->vm, 0, interpreter->handles[RECEIVER]);
    wrenSetSlotDouble(interpreter->vm, 1, delta_time);
    wrenCall(interpreter->vm, interpreter->handles[UPDATE]);
}

void Interpreter_render(Interpreter_t *interpreter, const double ratio)
{
    wrenEnsureSlots(interpreter->vm, 1);
    wrenSetSlotHandle(interpreter->vm, 0, interpreter->handles[RECEIVER]);
    wrenCall(interpreter->vm, interpreter->handles[RENDER]);
}

void Interpreter_terminate(Interpreter_t *interpreter)
{
    WrenInterpretResult result = wrenInterpret(interpreter->vm, ROOT_MODULE, SHUTDOWN_SCRIPT);
    if (result != WREN_RESULT_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't interpret shutdown script");
    }

    wrenCollectGarbage(interpreter->vm);

    TimerPool_terminate(&interpreter->timer_pool, timerpool_release_callback, interpreter);

    for (int i = 0; i < Handles_t_CountOf; ++i) {
        WrenHandle *handle = interpreter->handles[i];
        wrenReleaseHandle(interpreter->vm, handle);
    }

    wrenFreeVM(interpreter->vm);
}
