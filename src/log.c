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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// http://jafrog.com/2013/11/23/colors-in-terminal.html
#define COLOR_BLACK         "\x1b[30m"
#define COLOR_RED           "\x1b[31m"
#define COLOR_GREEN         "\x1b[32m"
#define COLOR_YELLOW        "\x1b[33m"
#define COLOR_BLUE          "\x1b[34m"
#define COLOR_MAGENTA       "\x1b[35m"
#define COLOR_CYAN          "\x1b[36m"
#define COLOR_WHITE         "\x1b[37m"

#define COLOR_BLACK_HC      "\x1b[90m"
#define COLOR_RED_HC        "\x1b[91m"
#define COLOR_GREEN_HC      "\x1b[92m"
#define COLOR_YELLOW_HC     "\x1b[93m"
#define COLOR_BLUE_HC       "\x1b[94m"
#define COLOR_MAGENTA_HC    "\x1b[95m"
#define COLOR_CYAN_HC       "\x1b[96m"
#define COLOR_WHITE_HC      "\x1b[97m"

#define COLOR_OFF           "\x1b[0m"

static Log_Levels_t _level = LOG_LEVELS_ALL;

static void log_output(int msg_type, const char *text, va_list args)
{
    static const char *color[] = { COLOR_WHITE, COLOR_BLUE_HC, COLOR_CYAN, COLOR_GREEN, COLOR_YELLOW, COLOR_RED, COLOR_MAGENTA, COLOR_WHITE };
    printf("%s", color[msg_type]);
    vprintf(text, args);
    printf(COLOR_OFF);
    if (text[strlen(text) - 1] != '\n') {
        printf("\n");
    }
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
