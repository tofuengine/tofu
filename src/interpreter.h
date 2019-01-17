#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include <limits.h>
#include <stdbool.h>

#include <wren/wren.h>

typedef enum _Handles_t {
    RECEIVER,
    INITIALIZE,
    HANDLE,
    UPDATE,
    RENDER,
    TERMINATE,
    Handles_t_CountOf
} Handles_t;

typedef struct _Interpreter_Config_t {
    const char *base_path;
    // WrenForeignMethodFn is_running_callback;
    // WrenForeignMethodFn input_callback;
} Interpreter_Config_t;

typedef struct _Interpreter_t {
    Interpreter_Config_t configuration;

    WrenVM *vm;
    WrenHandle *handles[Handles_t_CountOf];
} Interpreter_t;

extern bool Interpreter_initialize(Interpreter_t *interpreter, const Interpreter_Config_t *configuration);
extern void Interpreter_handle(Interpreter_t *interpreter);
extern void Interpreter_update(Interpreter_t *interpreter, const double delta_time);
extern void Interpreter_render(Interpreter_t *interpreter, const double ratio);
extern void Interpreter_terminate(Interpreter_t *interpreter);

#endif  /* __INTERPRETER_H__ */