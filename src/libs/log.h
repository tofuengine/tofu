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
#include <stdio.h>

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

// TODO: rename `_LW_x` to `LOGx` and `_LA_x` to `ASSERTx`

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_TRACE
  #define _LW_T(context, ...)               Log_write(LOG_LEVEL_TRACE, (context), __VA_ARGS__)
  #define _LA_T(condition, context, ...)    Log_assert((condition), LOG_LEVEL_TRACE, (context), __VA_ARGS__)
#else
  #define _LW_T(context, ...)
  #define _LA_T(condition, context, ...)
#endif

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_DEBUG
  #define _LW_D(context, ...)               Log_write(LOG_LEVEL_DEBUG, (context), __VA_ARGS__)
  #define _LA_D(condition, context, ...)    Log_assert((condition), LOG_LEVEL_DEBUG, (context), __VA_ARGS__)
#else
  #define _LW_D(context, ...)
  #define _LA_D(condition, context, ...)
#endif

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_INFO
  #define _LW_I(context, ...)               Log_write(LOG_LEVEL_INFO, (context), __VA_ARGS__)
  #define _LA_I(condition, context, ...)    Log_assert((condition), LOG_LEVEL_INFO, (context), __VA_ARGS__)
#else
  #define _LW_I(context, ...)
  #define _LA_I(condition, context, ...)
#endif

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_WARNING
  #define _LW_W(context, ...)               Log_write(LOG_LEVEL_WARNING, (context), __VA_ARGS__)
  #define _LA_W(condition, context, ...)    Log_assert((condition), LOG_LEVEL_WARNING, (context), __VA_ARGS__)
#else
  #define _LW_W(context, ...)
  #define _LA_W(condition, context, ...)
#endif

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_ERROR
  #define _LW_E(context, ...)               Log_write(LOG_LEVEL_ERROR, (context), __VA_ARGS__)
  #define _LA_E(condition, context, ...)    Log_assert((condition), LOG_LEVEL_ERROR, (context), __VA_ARGS__)
#else
  #define _LW_E(context, ...)
  #define _LA_E(condition, context, ...)
#endif

#if LOG_LEVEL_CURRENT >= LOG_LEVEL_FATAL
  #define _LW_F(context, ...)               Log_write(LOG_LEVEL_FATAL, (context), __VA_ARGS__)
  #define _LA_F(condition, context, ...)    Log_assert((condition), LOG_LEVEL_FATAL, (context), __VA_ARGS__)
#else
  #define _LW_F(context, ...)
  #define _LA_F(condition, context, ...)
#endif

extern void Log_write(int level, const char *context, const char *text, ...);
extern void Log_assert(bool condition, int level, const char *context, const char *text, ...);

#endif  /* __LIBS_LOG_H__ */
