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

#ifndef __TOFU_CONFIG_H__
#define __TOFU_CONFIG_H__


// Constant MACROs have no prefix.

#define STATISTICS_LENGTH           120
#define FPS_AVERAGE_SAMPLES         256
#define FPS_STATISTICS_RESOLUTION   10

#define MAX_PALETTE_COLORS          64

// Behavioural MACROs are uses the "__" prefix/suffix.

#define __FAST_FULLSCREEN__
#undef  __EXPLICIT_SIGNUM__
#undef  __FIND_NEAREST_COLOR_EUCLIDIAN__
#define __GRID_REPEAT_CONTENT__
#undef  __DEBUG_API_CALLS__


#endif  /* __TOFU_CONFIG_H__ */