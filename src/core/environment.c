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

#include "environment.h"

#include <libs/log.h>
#include <libs/stb.h>

#include <stdlib.h>
#include <string.h>

#define LOG_CONTEXT "environment"

// TODO: http://www.ilikebigbits.com/2017_06_01_float_or_double.html

Environment_t *Environment_create(void)
{
    Environment_t *environment = malloc(sizeof(Environment_t));
    if (!environment) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate environment");
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "environment allocated");

    *environment = (Environment_t){
        .quit = false,
        .fps = 0.0f,
        .time = 0.0
    };

    return environment;
}

void Environment_destroy(Environment_t *environment)
{
    free(environment);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "environment freed");
}
