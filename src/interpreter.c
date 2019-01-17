#include "interpreter.h"

#include "file.h"
#include "log.h"

#include <raylib/raylib.h>

#include <limits.h>
#include <string.h>

#define SCRIPT_EXTENSION        ".wren"

#define MAIN_MODULE_NAME        "@root@"
#define MAIN_MODULE_FILE        "tofu" SCRIPT_EXTENSION

/***/

static void draw_point(WrenVM* vm)
{
    int x = (int)wrenGetSlotDouble(vm, 1); 
    int y = (int)wrenGetSlotDouble(vm, 2); 
    int color = (int)wrenGetSlotDouble(vm, 3); 

    DrawPixel(x, y, (Color){ color, color, color, 255 });
}

/***/

static void* reallocate_function(void *ptr, size_t size)
{
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, size);
}

static const char *resolve_module_function(WrenVM *vm, const char *importer, const char *name)
{
    Log_write(LOG_LEVELS_DEBUG, "resolving module '%s' for importer '%s'", name, importer);

    if (strcmp(name, "meta") == 0 || strcmp(name, "random") == 0) { // Don't handle internal modules.
        return name;
    }

    char *resolved = reallocate_function(NULL, PATH_FILE_MAX);
    if (strcmp(importer, MAIN_MODULE_NAME) == 0) { // For the main module, use the base-path.
        Interpreter_Config_t *configuration = (Interpreter_Config_t *)wrenGetUserData(vm);
        strcpy(resolved, configuration->base_path);
    } else { // Otherwise, desume the path as relative to the importer.
        char *separator = strrchr(importer, '/');
        size_t length = separator - importer + 1; // Include the path-separator itself!
        strncpy(resolved, importer, length);
    }
    strcat(resolved, name);
    strcat(resolved, SCRIPT_EXTENSION);

    return resolved;
}

static char *load_module_function(WrenVM *vm, const char *name)
{
    if (name[0] != '/') { // Not a path, return NULL to access default modules.
        Log_write(LOG_LEVELS_DEBUG, "don't load module '%s', might be built-in", name);
        return NULL;
    }

    Log_write(LOG_LEVELS_DEBUG, "loading module '%s'", name);
    return file_load_as_string(name, "rt");
}

static void write_function(WrenVM *vm, const char *text)
{
    Log_write(LOG_LEVELS_OTHER, text);
}

static void error_function(WrenVM* vm, WrenErrorType type, const char *module, int line, const char *message)
{
    if (type == WREN_ERROR_COMPILE) {
        Log_write(LOG_LEVELS_ERROR, "Compile error: [%s@%d] %s", module, line, message);
    } else if (type == WREN_ERROR_RUNTIME) {
        Log_write(LOG_LEVELS_ERROR, "Runtime error: %s", message);
    } else if (type == WREN_ERROR_STACK_TRACE) {
        Log_write(LOG_LEVELS_ERROR, "  [%s@%d] %s", module, line, message);
    }
}

static WrenForeignMethodFn bind_foreign_method_function(WrenVM* vm, const char *module, const char *className,
    bool isStatic, const char *signature)
{
//    Log_write(LOG_LEVELS_OTHER, "%s %s %d %s", module, className, isStatic, signature);
    if (strcmp(className, "Draw") == 0) {
        if (isStatic && strcmp(signature, "point(_,_,_)") == 0) {
            return draw_point;
        }
    }
    return NULL;
}

bool Interpreter_initialize(Interpreter_t *interpreter, const Interpreter_Config_t *configuration)
{
    interpreter->configuration = *configuration;

    char module_filename[PATH_FILE_MAX];
    strcpy(module_filename, configuration->base_path);
    strcat(module_filename, MAIN_MODULE_FILE);

    WrenConfiguration vm_configuration; 
    wrenInitConfiguration(&vm_configuration);
    vm_configuration.reallocateFn = reallocate_function;
    vm_configuration.resolveModuleFn = resolve_module_function;
    vm_configuration.loadModuleFn = load_module_function;
    vm_configuration.bindForeignMethodFn = bind_foreign_method_function;
    vm_configuration.writeFn = write_function;
    vm_configuration.errorFn = error_function;

    interpreter->vm = wrenNewVM(&vm_configuration);
    if (!interpreter->vm) {
        Log_write(LOG_LEVELS_ERROR, "Can't initialize Wren's VM!");
        return false;
    }

    wrenSetUserData(interpreter->vm, &interpreter->configuration);

    char *source = file_load_as_string(module_filename, "rt");
    if (!source) {
        Log_write(LOG_LEVELS_ERROR, "Can't read main module '%s'!", module_filename);
        wrenFreeVM(interpreter->vm);
        return false;
    }

    WrenInterpretResult result = wrenInterpret(interpreter->vm, MAIN_MODULE_NAME, source);
    if (result != WREN_RESULT_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, "Can't interpret main module!");
        wrenFreeVM(interpreter->vm);
        return false;
    }

    free(source); // Dispose the script data.

    result = wrenInterpret(interpreter->vm, MAIN_MODULE_NAME, "var tofu = Tofu.new()");
    if (result != WREN_RESULT_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, "Can't create main class!");
        wrenFreeVM(interpreter->vm);
        return false;
    }

    wrenEnsureSlots(interpreter->vm, 1); 
    wrenGetVariable(interpreter->vm, MAIN_MODULE_NAME, "tofu", 0); 
    interpreter->handles[RECEIVER] = wrenGetSlotHandle(interpreter->vm, 0);

    interpreter->handles[HANDLE] = wrenMakeCallHandle(interpreter->vm, "handle(_)");
    interpreter->handles[UPDATE] = wrenMakeCallHandle(interpreter->vm, "update(_)");
    interpreter->handles[RENDER] = wrenMakeCallHandle(interpreter->vm, "render(_)");

    return true;
}

void Interpreter_handle(Interpreter_t *interpreter)
{
    wrenEnsureSlots(interpreter->vm, 1); 
    wrenSetSlotHandle(interpreter->vm, 0, interpreter->handles[RECEIVER]);
    wrenCall(interpreter->vm, interpreter->handles[HANDLE]);
}

void Interpreter_update(Interpreter_t *interpreter, const double delta_time)
{
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
    for (int i = 0; i < Handles_t_CountOf; ++i) {
        WrenHandle *handle = interpreter->handles[i];
        wrenReleaseHandle(interpreter->vm, handle);
    }

    wrenFreeVM(interpreter->vm);
}
