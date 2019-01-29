#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#include "file.h"

#include <stdbool.h>

#include <raylib/raylib.h>

#define MAX_GRAPHIC_BANKS       4

typedef struct _Bank_t {
    // char file[PATH_FILE_MAX];
    Texture2D atlas;
    int cell_width, cell_height;
} Bank_t;

typedef struct _Graphics_t {
    int width, height;
    Bank_t banks[MAX_GRAPHIC_BANKS];
} Graphics_t;

typedef struct _Environment_t {
    char base_path[PATH_FILE_MAX];
    bool should_close;

    Graphics_t graphics;
} Environment_t;

extern void Environment_initialize(Environment_t *environment, const char *base_path, int width, int height);
extern void Environment_terminate(Environment_t *environment);

#endif  /* __ENVIRONMENT_H__ */