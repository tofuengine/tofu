#ifndef __LOG_H__
#define __LOG_H__

#include <stdbool.h>

typedef enum _Log_Levels_t {
    LOG_LEVELS_INFO = 1,
    LOG_LEVELS_WARNING = 2,
    LOG_LEVELS_ERROR = 4,
    LOG_LEVELS_DEBUG = 8,
    LOG_LEVELS_OTHER = 16
} Log_Levels_t;

extern void Log_initialize();
extern void Log_configure(bool enabled);
extern void Log_write(Log_Levels_t level, const char *text, ...);

#endif  /* __LOG_H__ */
