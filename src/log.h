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

#ifndef __LOG_H__
#define __LOG_H__

#include <stdbool.h>

typedef enum _Log_Levels_t {
    LOG_LEVELS_ALL,
    LOG_LEVELS_TRACE,
    LOG_LEVELS_DEBUG,
    LOG_LEVELS_INFO,
    LOG_LEVELS_WARNING,
    LOG_LEVELS_ERROR,
    LOG_LEVELS_FATAL,
    LOG_LEVELS_NONE
} Log_Levels_t;

extern void Log_initialize();
extern void Log_configure(bool enabled);
extern void Log_write(Log_Levels_t level, const char *text, ...);

#endif  /* __LOG_H__ */
