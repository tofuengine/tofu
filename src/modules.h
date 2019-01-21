#ifndef __TOFU_MODULES_H__
#define __TOFU_MODULES_H__

#include "wren/wren.h"

typedef struct _Module_Entry_t {
    const char *module;
    const char *script;
} Module_Entry_t;

typedef struct _Class_Entry_t {
    const char *module;
    const char *className;
    WrenForeignMethodFn allocate;
    WrenFinalizerFn finalize;
} Class_Entry_t;

typedef struct _Method_Entry_t {
    const char *module;
    const char *className;
    bool isStatic;
    const char *signature;
    WrenForeignMethodFn method;
} Method_Entry_t;

extern const Module_Entry_t _modules[];
extern const Class_Entry_t _classes[];
extern const Method_Entry_t _methods[];

#endif  /* __TOFU_MODULES_H__ */
