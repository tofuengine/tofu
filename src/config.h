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

#ifndef __TOFU_CONFIG_H__
#define __TOFU_CONFIG_H__

// Constant MACROs have no prefix.
#define FPS_AVERAGE_SAMPLES         100

#define GARBAGE_COLLECTION_PERIOD   60.0

#define BALANCE_LAW_LINEAR    0
#define BALANCE_LAW_SINCOS    1
#define BALANCE_LAW_SQRT      2

#define PANNING_LAW_CONSTANT_GAIN           0
#define PANNING_LAW_CONSTANT_POWER_SINCOS   1
#define PANNING_LAW_CONSTANT_POWER_SQRT     2

// Behavioural MACROs use the `__` prefix/suffix.
#define __GL_VERSION__                      0x0201
#define __GSLS_VERSION__                    0x0114

// Includes checks inside some crucial functions. Could be useful in DEBUG mode.
#ifdef DEBUG
  #define __DEFENSIVE_CHECKS__
#else
  #undef  __DEFENSIVE_CHECKS__
#endif

#define __NO_LINEFEEDS__

// TODO: better naming for macros, including namespace.
#undef  __IGNORE_ALPHA_ON_COLORS__
#undef  __DEBUG_TRIANGLES_WINDING__
#undef  __FIND_NEAREST_COLOR_EUCLIDIAN__
#undef  __GRID_INTEGER_CELL__
#undef  __DEBUG_ENGINE_FPS__
#define __DEBUG_VM_CALLS__
#undef  __DEBUG_GRAPHICS__
#undef  __DEBUG_SHADER_CALLS__
#define __DEBUG_GARBAGE_COLLECTOR__
#define __VM_USE_CUSTOM_TRACEBACK__
#define __FS_SUPPORT_MOUNT_OVERRIDE__
#define __SL_BALANCE_LAW__ BALANCE_LAW_SINCOS
#define __SL_PANNING_LAW__ PANNING_LAW_CONSTANT_POWER_SINCOS
#define __SL_MUSIC_PRELOAD_ON_CREATION__
#undef  __GL_MASK_SUPPORT__

// In release build, disable VM calls debug for faster execution.
#ifdef RELEASE
  #undef __DEBUG_VM_CALLS__
#endif

#endif  /* __TOFU_CONFIG_H__ */