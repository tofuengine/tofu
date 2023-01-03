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

#ifndef TOFU_CORE_CONFIG_H
#define TOFU_CORE_CONFIG_H

#define CONFIG_H_INCLUDED

// Constant MACROs have no prefix.
#define FPS_AVERAGE_SAMPLES         128

#define GC_CONTINUOUS_STEP_PERIOD   0.1f
#define GC_COLLECTION_PERIOD        15.0f

#define GC_INCREMENTAL  0
#define GC_GENERATIONAL 1

#define GC_AUTOMATIC  0
#define GC_CONTINUOUS 1
#define GC_MANUAL     2

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

// TODO: better naming for macros, including namespace.
#define __LUAX_RTTI__
#undef  __LUAX_INCLUDE_SYSTEM_LIBRARIES__
#undef  __DEBUG_TRIANGLES_WINDING__
#undef  __FIND_NEAREST_COLOR_EUCLIDIAN__
#undef  __GRID_INTEGER_CELL__
#define __DEBUG_ENGINE_PERFORMANCES__
#undef  __DEBUG_FS_CALLS__
#define __DEBUG_VM_CALLS__
#undef  __DEBUG_GRAPHICS__
#undef  __DEBUG_SHADER_CALLS__
#define __DEBUG_GARBAGE_COLLECTOR__
#define __OPENGL_STATE_CLEANUP__
#define __VM_READER_BUFFER_SIZE__ 1024U
#define __VM_USE_CUSTOM_TRACEBACK__
#define __VM_GARBAGE_COLLECTOR_TYPE__ GC_INCREMENTAL
#define __VM_GARBAGE_COLLECTOR_MODE__ GC_CONTINUOUS
#undef  __VM_GARBAGE_COLLECTOR_PERIODIC_COLLECT__
#define __AUDIO_START_AND_STOP__
#define __AUDIO_START_AND_STOP_GRACE_PERIOD__ 30.0
#define __INPUT_CURSOR_EMULATION__
#define __INPUT_CONTROLLER_EMULATION__
#define __INPUT_CONTROLLER_DETECTION_PERIOD__ 10.0
#define __STORAGE_CHECK_ABSOLUTE_PATHS__
#define __STORAGE_CACHE_ENTRIES_LIMIT__ 32U
#define __FS_ENFORCE_ARCHIVE_EXTENSION__
#define __FS_SUPPORT_MOUNT_OVERRIDE__
#define __SL_BALANCE_LAW__  BALANCE_LAW_SINCOS
#define __SL_PANNING_LAW__  PANNING_LAW_CONSTANT_POWER_SINCOS
#undef  __SL_MUSIC_PRELOAD__
#undef  __GL_CLOCKWISE_WINDING__
#define __GL_FIX_WINDING__
#undef  __GL_XFORM_TRANSPARENCY__
#define __GL_OPTIMIZED_ROTATIONS__
#define __DISPLAY_FOCUS_SUPPORT__
#undef  __PROGRAM_DEFAULT_QUANTIZED_PALETTE__
#define __PALETTE_COLOR_MEMOIZATION__
#define __ENGINE_PERFORMANCE_STATISTICS__
#define __ENGINE_PERFORMANCES_PERIOD__ 10.0f
#define __ENGINE_KERNAL_NAME__ "kernal.pak"
#define __SYSTEM_HEAP_STATISTICS__
#define __SYSTEM_HEAP_PERIOD__  5.0f

// In release build, disable VM calls debug and periodic collection for better performance.
#ifdef NDEBUG
  #undef __LUAX_RTTI__
  #undef __DEBUG_ENGINE_PERFORMANCES__
  #undef __SYSTEM_HEAP_STATISTICS__
  #undef __ENGINE_PERFORMANCE_STATISTICS__
  #undef __DEBUG_VM_CALLS__
  #undef __DEBUG_GARBAGE_COLLECTOR__
  #undef  __VM_GARBAGE_COLLECTOR_PERIODIC_COLLECT__
#endif

#endif  /* TOFU_CORE_CONFIG_H */
