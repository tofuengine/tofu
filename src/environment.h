#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#include "file.h"

#include <stdbool.h>

typedef struct _Environment_t {
    char base_path[PATH_FILE_MAX];
    bool should_close;
} Environment_t;

#endif  /* __ENVIRONMENT_H__ */