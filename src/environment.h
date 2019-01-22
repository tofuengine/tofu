#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#include "file.h"

#include <stdbool.h>

#include <raylib/raylib.h>

typedef struct _Bank_t {
    // char file[PATH_FILE_MAX];
    Texture2D atlas;
    int cell_width, cell_height;
} Bank_t;

typedef struct _Graphics_t {
    Bank_t banks[4];
} Graphics_t;

typedef struct _Environment_t {
    char base_path[PATH_FILE_MAX];
    bool should_close;

    Graphics_t graphics;
} Environment_t;

#endif  /* __ENVIRONMENT_H__ */