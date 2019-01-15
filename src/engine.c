#include "engine.h"
#include "configuration.h"
#include "log.h"

#include <raylib/raylib.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

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

    while (!Display_shouldClose(&engine->display)) {
        Interpreter_step(&engine->interpreter);
        Display_render(&engine->display, NULL);
    }
}
