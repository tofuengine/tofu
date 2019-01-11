#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include <stdbool.h>

#include <wren/wren.h>

typedef struct _Interpreter_t {

} Interpreter_t;

extern bool Interpreter_initialize(Interpreter_t *interpreter);
extern void Interpreter_terminate(Interpreter_t *interpreter);

#endif  /* __INTERPRETER_H__ */