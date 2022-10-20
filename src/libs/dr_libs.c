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

#if defined(DEBUG) && !defined(SANITIZE)
  #include <stb/stb_leakcheck.h>
#endif

#if defined(DEBUG) && !defined(SANITIZE)
  #define DRFLAC_MALLOC(sz)                   stb_leakcheck_realloc(NULL, (sz), __FILE__, __LINE__)
  #define DRFLAC_REALLOC(p, sz)               stb_leakcheck_realloc((p), (sz), __FILE__, __LINE__)
  #define DRFLAC_FREE(p)                      stb_leakcheck_free((p))
#endif
#define DR_FLAC_IMPLEMENTATION
#include <dr_libs/dr_flac.h>
