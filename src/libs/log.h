/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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
 */

#ifndef TOFU_LIBS_LOG_H
#define TOFU_LIBS_LOG_H

#include <stdbool.h>
#include <stdio.h>

typedef enum Log_Levels_e {
    LOG_LEVELS_ALL,
    LOG_LEVELS_TRACE,
    LOG_LEVELS_DEBUG,
    LOG_LEVELS_INFO,
    LOG_LEVELS_WARNING,
    LOG_LEVELS_ERROR,
    LOG_LEVELS_FATAL,
    LOG_LEVELS_NONE,
    Log_Levels_t_CountOf
} Log_Levels_t;

#define LOG_T(context, ...) Log_write(LOG_LEVELS_TRACE, (context), __VA_ARGS__)
#define LOG_D(context, ...) Log_write(LOG_LEVELS_DEBUG, (context), __VA_ARGS__)
#define LOG_I(context, ...) Log_write(LOG_LEVELS_INFO, (context), __VA_ARGS__)
#define LOG_W(context, ...) Log_write(LOG_LEVELS_WARNING, (context), __VA_ARGS__)
#define LOG_E(context, ...) Log_write(LOG_LEVELS_ERROR, (context), __VA_ARGS__)
#define LOG_F(context, ...) Log_write(LOG_LEVELS_FATAL, (context), __VA_ARGS__)

#define LOG_IF_T(condition, context, ...)   Log_write_if((condition), LOG_LEVELS_TRACE, (context), __VA_ARGS__)
#define LOG_IF_D(condition, context, ...)   Log_write_if((condition), LOG_LEVELS_DEBUG, (context), __VA_ARGS__)
#define LOG_IF_I(condition, context, ...)   Log_write_if((condition), LOG_LEVELS_INFO, (context), __VA_ARGS__)
#define LOG_IF_W(condition, context, ...)   Log_write_if((condition), LOG_LEVELS_WARNING, (context), __VA_ARGS__)
#define LOG_IF_E(condition, context, ...)   Log_write_if((condition), LOG_LEVELS_ERROR, (context), __VA_ARGS__)
#define LOG_IF_F(condition, context, ...)   Log_write_if((condition), LOG_LEVELS_FATAL, (context), __VA_ARGS__)

// TODO: add log-to-file.
extern void Log_initialize(void);
extern void Log_configure(bool enabled, FILE *stream);
extern void Log_write(Log_Levels_t level, const char *context, const char *text, ...);
extern void Log_write_if(bool condition, Log_Levels_t level, const char *context, const char *text, ...);

#endif  /* TOFU_LIBS_LOG_H */
