#include "log.h"

#include <raylib/raylib.h>

#include <stdio.h>
#include <stdarg.h>

#define MAX_LOG_MESSAGE_LENGTH      256

static void custom_log_callback(int msg_type, const char *text, va_list args)
{
    const char *prefix = "";
    switch (msg_type) {
        case LOG_INFO: prefix = "[INFO ] "; break;
        case LOG_ERROR: prefix = "[ERROR] "; break;
        case LOG_WARNING: prefix = "[WARN ] "; break;
        case LOG_DEBUG: prefix = "[DEBUG] "; break;
        case LOG_OTHER: prefix = "[OTHER] "; break;
        default: break;
    }

    printf("%s", prefix);
    vprintf(text, args);
    printf("\n");
}

void Log_initialize()
{
    SetTraceLogCallback(custom_log_callback);
    SetTraceLog(LOG_DEBUG | LOG_INFO | LOG_WARNING | LOG_ERROR | LOG_OTHER);
}

void Log_configure(bool enabled)
{
    SetTraceLog(enabled
        ? LOG_DEBUG | LOG_INFO | LOG_WARNING | LOG_ERROR | LOG_OTHER
        : LOG_ERROR);
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
