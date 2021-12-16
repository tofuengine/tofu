/*
 * MIT License
 *
 * Copyright (c) 2019-2021 Marco Lizza
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

#ifndef __VERSION_H__
#define __VERSION_H__

#define TOFU_VERSION_MAJOR          0
#define TOFU_VERSION_MINOR          11
#define TOFU_VERSION_REVISION       0

#define TOFU_VERSION_NUMBER         ((TOFU_VERSION_MAJOR << 16) | (TOFU_VERSION_MINOR << 8) | (TOFU_VERSION_REVISION))

#ifdef DEBUG
  #define _TOFU_CONCAT_VERSION(m, n, r)   #m "." #n "." #r "-dev-debug"
#else
  #define _TOFU_CONCAT_VERSION(m, n, r)   #m "." #n "." #r "-dev"
#endif
#define _TOFU_MAKE_VERSION(m, n, r)     _TOFU_CONCAT_VERSION(m, n, r)
#define TOFU_VERSION_STRING             _TOFU_MAKE_VERSION(TOFU_VERSION_MAJOR, TOFU_VERSION_MINOR, TOFU_VERSION_REVISION)

#endif  /* __VERSION_H__ */
