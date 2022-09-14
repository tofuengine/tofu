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

#ifndef __LIBS_MUMALLOC_H__
#define __LIBS_MUMALLOC_H__

#include <stddef.h>

extern void *mu_malloc(size_t size);
extern void *mu_calloc(size_t count, size_t size);
extern void *mu_realloc(void *p, size_t newsize);
//extern void *mi_expand(void *p, size_t newsize);

extern char *mu_strdup(const char *s);
extern char *mu_strndup(const char *s, size_t n);
extern char *mu_realpath(const char *fname, char *resolved_name);

extern void *mu_memdup(const void *ptr, size_t size);

extern void mu_free(void* p);

#endif  /* __LIBS_MUMALLOC_H__ */
