#include "log.h"

#include <raylib/raylib.h>

#include <stdio.h>
#include <stdarg.h>

#define MAX_LOG_MESSAGE_LENGTH      256

static void custom_log_callback(int msg_type, const char *text, va_list args)
{
    static const char prefix[] = { 'T', 'D', 'I', 'W', 'E', 'F' };
    printf("[%c] ", prefix[msg_type]);
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
