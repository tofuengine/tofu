#include "engine.h"
#include "configuration.h"

#include <raylib/raylib.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

bool Engine_initialize(Engine_t *engine, const char *basepath)
{
    strcpy(engine->basepath, basepath);

    char cfgfile[PATH_MAX];
    strcpy(cfgfile, basepath);
    strcat(cfgfile, "/configuration.json");

    Configuration_load(&engine->configuration, cfgfile);

    SetTraceLog(engine->configuration.debug ? LOG_DEBUG | LOG_INFO | LOG_WARNING : 0);

    Display_initialize(&engine->display, engine->configuration.width, engine->configuration.height, engine->configuration.title);

    return true;
}

void Engine_terminate(Engine_t *engine)
{
    Display_terminate(&engine->display);
}

void Engine_run(Engine_t *engine)
{
    TraceLog(LOG_DEBUG, "Engine is now running");

    while (!Display_shouldClose(&engine->display)) {
        Display_render(&engine->display, NULL);
    }
}
