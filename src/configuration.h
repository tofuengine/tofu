#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <stdbool.h>

#define MAX_CONFIGURATION_TITLE_LENGTH      128

typedef struct _Configuration {
    char title[MAX_CONFIGURATION_TITLE_LENGTH];
    int width, height, depth;
    bool fullscreen;
    bool autofit;
    int fps;
    bool debug;
} Configuration_t;

extern void Configuration_initialize(Configuration_t *configuration);
extern void Configuration_load(Configuration_t *configuration, const char *filename);

#endif  /* __CONFIGURATION_H__ */