/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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

#include "mumalloc.h"

#include <config.h>
#include <platform.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if PLATFORM_ID == PLATFORM_WINDOWS
  #define realpath(N,R) _fullpath((R),(N),PLATFORM_PATH_MAX)
#endif

void *mu_malloc(size_t size)
{
    return malloc(size);
}

void *mu_calloc(size_t count, size_t size)
{
    return calloc(count, size);
}

void *mu_realloc(void *p, size_t newsize)
{
    return realloc(p, newsize);
}

char *mu_strdup(const char *s)
{
    size_t length = strlen(s);
    return mu_memdup(s, length + 1);
}

char *mu_strndup(const char *s, size_t n)
{
    size_t length = strlen(s);
    if (length > n) {
        char *copy = mu_memdup(s, n);
        copy[n] = '\0';
        return copy;
    }
    return mu_memdup(s, length + 1);
}

char *mu_realpath(const char *fname, char *resolved_name)
{
    char *buffer = mu_malloc(PLATFORM_PATH_MAX + 1);
    if (!buffer) {
        return NULL;
    }
    char *rname  = realpath(fname, buffer);
    char *result = mu_strndup(rname, PLATFORM_PATH_MAX); // ok if `rname==NULL`
    mu_free(buffer);
    return result;
}

void *mu_memdup(const void *ptr, size_t size)
{
    void *copy = malloc(size);
    if (copy) {
        memcpy(copy, ptr, size);
    }
    return copy;
}

void mu_free(void* p)
{
    free(p);
}
