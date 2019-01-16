#include "engine.h"
#include "configuration.h"
#include "log.h"

#include <raylib/raylib.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

static const char *engineModuleSource =
    "class Engine {\n"
    "\n"
    "    foreign static running\n"
    "\n"
    "    foreign static inputs\n"
    "\n"
    "}\n";

void is_running_callback(WrenVM* vm) 
{ 
    bool shouldClose = Display_shouldClose(NULL); // FIXME: pass the real display context.
    wrenSetSlotBool(vm, 0, shouldClose);
}
void inputs_callback(WrenVM* vm) 
{ 
    wrenSetSlotNewList(vm, 0); // TODO: fill with the actual list of inputs.
}

bool Engine_initialize(Engine_t *engine, const char *base_path)
{
    strcpy(engine->base_path, base_path);

    char filename[PATH_MAX];
    strcpy(filename, base_path);
    strcat(filename, "configuration.json");

    Log_initialize();

    Configuration_initialize(&engine->configuration);
    Configuration_load(&engine->configuration, filename);

    Log_configure(engine->configuration.debug);

    Interpreter_Config_t configuration;
    configuration.base_path = engine->base_path;
    configuration.is_running_callback = is_running_callback;
    configuration.inputs_callback = inputs_callback;
    Interpreter_initialize(&engine->interpreter, base_path);

    Display_initialize(&engine->display, engine->configuration.width, engine->configuration.height, engine->configuration.title);

    return true;
}

void Engine_terminate(Engine_t *engine)
{
    Display_terminate(&engine->display);
    Interpreter_terminate(&engine->interpreter);
}

void Engine_run(Engine_t *engine)
{
    Log_write(LOG_LEVELS_DEBUG, "Engine is now running");

    Interpreter_run(&engine->interpreter);

    while (!Display_shouldClose(&engine->display)) {
        Display_render(&engine->display, NULL);
    }
}
