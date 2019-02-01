/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

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
