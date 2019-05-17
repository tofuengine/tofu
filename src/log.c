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

#include <stdio.h>
#include <stdarg.h>

#define COLOR_NONE      ""
#define COLOR_RED       "\033[0;31m"
#define COLOR_GREEN     "\033[0;32m"
#define COLOR_YELLOW    "\033[0;33m"
#define COLOR_BLUE      "\033[0;34m"
#define COLOR_MAGENTA   "\033[0;35m"
#define COLOR_CYAN      "\033[0;36m"
#define COLOR_OFF       "\033[0m"

static Log_Levels_t _level = LOG_LEVELS_ALL;

static void log_output(int msg_type, const char *text, va_list args)
{
    static const char *color[] = { COLOR_NONE, COLOR_BLUE, COLOR_BLUE, COLOR_GREEN, COLOR_YELLOW, COLOR_RED, COLOR_RED, COLOR_NONE };
    static const char prefix[] = { '<', 'T', 'D', 'I', 'W', 'E', 'F', '>' };
    printf("%s[%c] ", color[msg_type], prefix[msg_type]);
    vprintf(text, args);
    if (color[msg_type][0] != '\0') {
        printf(COLOR_OFF);
    }
    printf("\n");
}

void Log_initialize()
{
    _level = LOG_LEVELS_ALL;
}

void Log_configure(bool enabled)
{
    _level = enabled ? LOG_LEVELS_ALL : LOG_LEVELS_NONE;
}

void Log_write(Log_Levels_t level, const char *text, ...)
{
    if (level < _level) {
        return;
    }

    va_list args;
    va_start(args, text);
    log_output((int)level, text, args);
    va_end(args);
}
