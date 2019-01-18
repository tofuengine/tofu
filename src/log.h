#ifndef __LOG_H__
#define __LOG_H__

#include <stdbool.h>

typedef enum _Log_Levels_t {
    LOG_LEVELS_TRACE,
    LOG_LEVELS_DEBUG,
    LOG_LEVELS_INFO,
    LOG_LEVELS_WARNING,
    LOG_LEVELS_ERROR,
    LOG_LEVELS_FATAL
} Log_Levels_t;

extern void Log_initialize();
extern void Log_configure(bool enabled);
extern void Log_write(Log_Levels_t level, const char *text, ...);

#endif  /* __LOG_H__ */
