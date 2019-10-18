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

#include "environment.h"

#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif

// TODO: http://www.ilikebigbits.com/2017_06_01_float_or_double.html

void Environment_initialize(Environment_t *environment, const char *base_path)
{
    // *environment = (Environment_t){};
    memset(environment, 0x00, sizeof(Environment_t)); // TODO: use memset or assignment?

    environment->quit = false;
    environment->fps = 0.0f;
    environment->time = 0.0f;

    FS_initialize(&environment->fs, base_path);
}

void Environment_terminate(Environment_t *environment)
{
    FS_terminate(&environment->fs);
}
