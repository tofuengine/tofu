#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include <limits.h>
#include <stdbool.h>

#include <wren/wren.h>

#include "environment.h"

typedef enum _Handles_t {
    RECEIVER,
    HANDLE,
    UPDATE,
    RENDER,
    Handles_t_CountOf
} Handles_t;

typedef struct _Interpreter_t {
    const Environment_t *environment;

    WrenVM *vm;
    WrenHandle *handles[Handles_t_CountOf];
} Interpreter_t;

extern bool Interpreter_initialize(Interpreter_t *interpreter, const Environment_t *environment);
extern void Interpreter_handle(Interpreter_t *interpreter);
extern void Interpreter_update(Interpreter_t *interpreter, const double delta_time);
extern void Interpreter_render(Interpreter_t *interpreter, const double ratio);
extern void Interpreter_terminate(Interpreter_t *interpreter);

#endif  /* __INTERPRETER_H__ */