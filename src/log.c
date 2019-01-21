#include "log.h"

#include <raylib/raylib.h>

#include <stdio.h>
#include <stdarg.h>

#define MAX_LOG_MESSAGE_LENGTH      256

static void custom_log_callback(int msg_type, const char *text, va_list args)
{
    const char *prefix = "";
    switch (msg_type) {
        case LOG_TRACE: prefix = "[TRACE] "; break;
        case LOG_DEBUG: prefix = "[DEBUG] "; break;
        case LOG_INFO: prefix = "[INFO ] "; break;
        case LOG_WARNING: prefix = "[WARN ] "; break;
        case LOG_ERROR: prefix = "[ERROR] "; break;
        case LOG_FATAL: prefix = "[FATAL] "; break;
        default: break;
    }

    printf("%s", prefix);
    vprintf(text, args);
    printf("\n");
}

void Log_initialize()
{
    SetTraceLogCallback(custom_log_callback);
    SetTraceLogLevel(LOG_ALL);
}

void Log_configure(bool enabled)
{
    SetTraceLogExit(LOG_NONE);
    SetTraceLogLevel(enabled ? LOG_ALL : LOG_NONE);
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
