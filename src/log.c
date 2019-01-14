#include "log.h"

#include <raylib/raylib.h>

#include <stdio.h>
#include <stdarg.h>

#define MAX_LOG_MESSAGE_LENGTH      256

void Log_configure(bool enabled)
{
    SetTraceLog(enabled
        ? LOG_DEBUG | LOG_INFO | LOG_WARNING | LOG_ERROR | LOG_OTHER
        : LOG_WARNING | LOG_ERROR);
}

void Log_write(Log_Levels_t level, const char *text, ...)
{
    char message[MAX_LOG_MESSAGE_LENGTH];
    va_list args;
    va_start(args, text);
    vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, text, args);
    va_end(args);

    TraceLog((int)level, message);
}
