#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include <limits.h>
#include <stdbool.h>

#include <wren/wren.h>

typedef struct _UserData_t {
    char base_path[PATH_MAX];
} UserData_t;

typedef enum _Handles_t {
    RECEIVER,
    INITIALIZE,
    STEP,
    TERMINATE,
    Handles_t_CountOf
} Handles_t;

typedef struct _Interpreter_t {

    WrenVM *vm;
    WrenHandle *handles[Handles_t_CountOf];

    UserData_t user_data;

} Interpreter_t;

extern bool Interpreter_initialize(Interpreter_t *interpreter, const char *base_path);
extern void Interpreter_step(Interpreter_t *interpreter);
extern void Interpreter_terminate(Interpreter_t *interpreter);

#endif  /* __INTERPRETER_H__ */