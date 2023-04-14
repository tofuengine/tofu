/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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
#if defined(DEBUG)
  #define __DEFENSIVE_CHECKS__
#else
  #undef  __DEFENSIVE_CHECKS__
#endif

// Current namespaces:
//   - AUDIO
//   - API
//   - DISPLAY
//   - ENGINE
//   - GRAPHICS
//   - INTERPRETER
//   - SCRIPT
//   - STORAGE
//   - SYSTEM

#define TOFU_AUDIO_AUTOSTART
#define TOFU_AUDIO_AUTOSTART_GRACE_PERIOD 30.0
#define TOFU_DISPLAY_FOCUS_SUPPORT
#define TOFU_DISPLAY_OPENGL_STATE_CLEANUP
#define TOFU_ENGINE_BREAKPOINT_DETECTION_THRESHOLD 1.0f
#define TOFU_ENGINE_DATA_NAME "data.pak"
#define TOFU_ENGINE_KERNAL_NAME "kernal.pak"
#define TOFU_ENGINE_HEAP_STATISTICS
#define TOFU_ENGINE_HEAP_STATISTICS_PERIOD  5.0f
#undef  TOFU_ENGINE_HEAP_STATISTICS_DEBUG
#define TOFU_ENGINE_PERFORMANCE_STATISTICS
#define TOFU_ENGINE_PERFORMANCE_STATISTICS_DEBUG
#define TOFU_ENGINE_PERFORMANCE_STATISTICS_PERIOD 10.0f
#define TOFU_FILE_DEBUG_ENABLED
#define TOFU_FILE_SUPPORT_MOUNT_OVERRIDE
#undef  TOFU_GRAPHICS_CLOCKWISE_RASTERIZER_WINDING
#define TOFU_GRAPHICS_FIX_RASTERIZER_WINDING
#undef  TOFU_GRAPHICS_DEBUG_ENABLED
#undef  TOFU_GRAPHICS_DEBUG_TRIANGLES_WINDING
#undef  TOFU_GRAPHICS_DEFAULT_PALETTE_IS_QUANTIZED
#undef  TOFU_GRAPHICS_EUCLIDIAN_NEAREST_COLOR
#define TOFU_GRAPHICS_OPTIMIZED_ROTATIONS
#define TOFU_GRAPHICS_PALETTE_MATCH_MEMOIZATION
#undef  TOFU_GRAPHICS_REPORT_SHADERS_ERRORS
#undef  TOFU_GRAPHICS_XFORM_TRANSPARENCY
#define TOFU_INTERPRETER_CUSTOM_TRACEBACK
#define TOFU_INTERPRETER_GC_REPORTING
#define TOFU_INTERPRETER_GC_TYPE GC_INCREMENTAL
#define TOFU_INTERPRETER_GC_MODE GC_CONTINUOUS
#undef  TOFU_INTERPRETER_GC_PERIODIC
#define TOFU_INTERPRETER_PROTECTED_CALLS
#define TOFU_INTERPRETER_READER_BUFFER_SIZE 1024U
#define TOFU_INPUT_CONTROLLER_DETECTION_PERIOD 10.0
#define TOFU_INPUT_CONTROLLER_IS_EMULATED
#define TOFU_INPUT_CURSOR_IS_EMULATED

// Sound
// #####

// Configure the stereo *balance law* used by the sound sub-system, that is
// the curve that controls the *relative levels* of the left and right channels
// of a sound. The relationship to each other changes, level-wise, but not their
// position in the stereo panorama.
//
// This is used for stereo sound sources.
//
// Can be one of the following values:
//
//   - BALANCE_LAW_LINEAR
//   - BALANCE_LAW_SINCOS
//   - BALANCE_LAW_SQRT
//
#define TOFU_SOUND_BALANCE_LAW BALANCE_LAW_SINCOS

// Configure the stereo *panning law* used by the sound sub-system, that is
// the curve that controls the *position* of a sound in the stereo panorama
// (the levels of the channels aren't changed).
//
// This is used for *mono* sound sources.
//
// Can be one of the following values:
//
//   - PANNING_LAW_CONSTANT_GAIN
//   - PANNING_LAW_CONSTANT_POWER_SINCOS
//   - PANNING_LAW_CONSTANT_POWER_SQRT
//
// Usually a *constant power* law seems to be perceived as more "natural".
//
#define TOFU_SOUND_PANNING_LAW PANNING_LAW_CONSTANT_POWER_SINCOS

// The sound sub-system can support music preloading, that is the playing buffer
// is filled during the opening phase so that it will be available from the
// start and ready to be played. Unless explicitly required the advice is to
// leave this disabled.
#undef  TOFU_SOUND_MUSIC_PRELOAD

// Script
// ######

// This controls whether, in the engine Lua script API, the `Grid` UDT stores
// `integer` or `float`valued "cells". The suggested value is `integer` as it
// is more consistent.
#undef  TOFU_SCRIPT_GRID_INTEGER_VALUES

// Storage
// #######

#define TOFU_STORAGE_CACHE_ENTRIES_LIMIT 32U

#define TOFU_STORAGE_RESOURCE_MAX_AGE 30.0

// In release build, disable VM calls debug and periodic collection for better performance.
#if defined(NDEBUG)
  #undef TOFU_ENGINE_PERFORMANCE_STATISTICS
  #undef TOFU_ENGINE_HEAP_STATISTICS
  #undef TOFU_FILE_DEBUG_ENABLED
  #undef TOFU_GRAPHICS_EUCLIDIAN_NEAREST_COLOR
  #undef TOFU_GRAPHICS_REPORT_SHADERS_ERRORS
  #undef TOFU_INTERPRETER_PROTECTED_CALLS
  #undef TOFU_INTERPRETER_GC_REPORTING
  #undef TOFU_INTERPRETER_GC_PERIODIC
#endif

#endif  /* TOFU_CORE_CONFIG_H */
