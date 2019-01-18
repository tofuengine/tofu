#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <stdbool.h>
#include <limits.h>

#include "configuration.h"
#include "display.h"
#include "environment.h"
#include "interpreter.h"

typedef struct _Engine_t {

    Environment_t environment;

    Configuration_t configuration;

    Display_t display;
    Interpreter_t interpreter;

} Engine_t;

extern bool Engine_initialize(Engine_t *engine, const char *base_path);
extern void Engine_terminate(Engine_t *engine);
extern void Engine_run(Engine_t *engine);

#endif  /* __ENGINE_H__ */