/*
 * MIT License
 *
 * Copyright (c) 2019-2020 Marco Lizza
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

#ifndef __LIBS_LOG_H__
#define __LIBS_LOG_H__

#include <stdbool.h>

#define LOG_LEVEL_FATAL        0
#define LOG_LEVEL_ERROR        1
#define LOG_LEVEL_WARNING      2
#define LOG_LEVEL_INFO         3
#define LOG_LEVEL_DEBUG        4
#define LOG_LEVEL_TRACE        5

#ifndef LOG_LEVEL_CURRENT
  #ifdef DEBUG
    #define LOG_LEVEL_CURRENT LOG_LEVEL_TRACE
  #else
    #define LOG_LEVEL_CURRENT LOG_LEVEL_ERROR
  #endif
#endif

// TODO: rename `LOG_x` to `TE_LOG_x` and `ASSERT_x` to `TE_ASSERT_x` to convey some kind of "namespace"?

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_TRACE
  #define LOG_TRACE(context, ...)                 Log_write(LOG_LEVEL_TRACE, (context), __VA_ARGS__)
  #define LOG_TRACE_IF(condition, context, ...)   Log_write_if((condition), LOG_LEVEL_TRACE, (context), __VA_ARGS__)
#else
  #define LOG_TRACE(context, ...)
  #define LOG_TRACE_IF(condition, context, ...)
#endif

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_DEBUG
  #define LOG_DEBUG(context, ...)                 Log_write(LOG_LEVEL_DEBUG, (context), __VA_ARGS__)
  #define LOG_DEBUG_IF(condition, context, ...)   Log_write_if((condition), LOG_LEVEL_DEBUG, (context), __VA_ARGS__)
#else
  #define LOG_DEBUG(context, ...)
  #define LOG_DEBUG_IF(condition, context, ...)
#endif

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_INFO
  #define LOG_INFO(context, ...)                  Log_write(LOG_LEVEL_INFO, (context), __VA_ARGS__)
  #define LOG_INFO_IF(condition, context, ...)    Log_write_if((condition), LOG_LEVEL_INFO, (context), __VA_ARGS__)
#else
  #define LOG_INFO(context, ...)
  #define LOG_INFO_IF(condition, context, ...)
#endif

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_WARNING
  #define LOG_WARNING(context, ...)               Log_write(LOG_LEVEL_WARNING, (context), __VA_ARGS__)
  #define LOG_WARNING_IF(condition, context, ...) Log_write_if((condition), LOG_LEVEL_WARNING, (context), __VA_ARGS__)
#else
  #define LOG_WARNING(context, ...)
  #define LOG_WARNING_IF(condition, context, ...)
#endif

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_ERROR
  #define LOG_ERROR(context, ...)                 Log_write(LOG_LEVEL_ERROR, (context), __VA_ARGS__)
  #define LOG_ERROR_IF(condition, context, ...)   Log_write_if((condition), LOG_LEVEL_ERROR, (context), __VA_ARGS__)
#else
  #define LOG_ERROR(context, ...)
  #define LOG_ERROR_IF(condition, context, ...)
#endif

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_FATAL
  #define LOG_FATAL(context, ...)                 Log_write(LOG_LEVEL_FATAL, (context), __VA_ARGS__)
  #define LOG_FATAL_IF(condition, context, ...)   Log_write_if((condition), LOG_LEVEL_FATAL, (context), __VA_ARGS__)
#else
  #define LOG_FATAL(context, ...)
  #define LOG_FATAL_IF(condition, context, ...)
#endif

extern void Log_write(int level, const char *context, const char *text, ...);
extern void Log_write_if(bool condition, int level, const char *context, const char *text, ...);

#endif  /* __LIBS_LOG_H__ */
