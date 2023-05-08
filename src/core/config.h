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

#define GC_TYPE_INCREMENTAL  0
#define GC_TYPE_GENERATIONAL 1

#define GC_MODE_AUTOMATIC  0
#define GC_MODE_CONTINUOUS 1
#define GC_MODE_PERIODIC   2
#define GC_MODE_MANUAL     3

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

// #############
// ### Audio ###
// #############

// The audio sub-system runs on a separate thread that processes any currently
// active source, mixing them up, and generates the sound data for the audio
// device. However, having the audio device always active can be undesirable,
// and one could prefer that the audio device is open only while there are some
// sound data to process. This macro controls this features.
#define TOFU_AUDIO_AUTOSTART

// This is the amount of time, in seconds, that the game engine will wait when
// no sound data is being processed prior switching the audio device to its
// `stopped` state.
//
// Note: the audio device will be put back into the `started` state as soon as
//       some sound data is to be played.
#define TOFU_AUDIO_AUTOSTART_GRACE_PERIOD 30.0

// ###############
// ### Display ###
// ###############

// When defined the display sub-system will try and keep OpenGL in a controlled
// "clean" state over successive iterations. That is, for example, the
// framebuffer texture will be bound/unbound. Ideally this is "gentler" with
// OpenGL but with some cost. You can try and `#undef` it and try for
// yourself. :)
#define TOFU_DISPLAY_OPENGL_STATE_CLEANUP

// ##############
// ### Engine ###
// ##############

// When a breakpoint is "hit" in the game-loop the current frame time most
// certainly becomes way greater than the usual ones. When the execution resumes
// the `update()` sub-loop will *fast-forward* to recover the missed time.
//
// This threshold is used to discriminate "stuttering but acceptable" frame
// steps from the ones that would compromise the execution.
//
// Note: the feature is enabled only in the `DEBUG` build.
#define TOFU_ENGINE_BREAKPOINT_DETECTION_THRESHOLD 1.0f

// These two macros defines the name of the *data* and *kernal* archives that
// the game-engine expects to find.
#define TOFU_ENGINE_DATA_NAME "data.pak"
#define TOFU_ENGINE_KERNAL_NAME "kernal.pak"

#define TOFU_ENGINE_HEAP_STATISTICS
#define TOFU_ENGINE_HEAP_STATISTICS_PERIOD  5.0f
#undef  TOFU_ENGINE_HEAP_STATISTICS_DEBUG

#define TOFU_ENGINE_PERFORMANCE_STATISTICS
#define TOFU_ENGINE_PERFORMANCE_STATISTICS_DEBUG
#define TOFU_ENGINE_PERFORMANCE_STATISTICS_PERIOD 10.0f

// ##############
// ### Events ###
// ##############

// Controls whether the game-engine should process and fire "focus changed"
// events during the execution. This could be used to automatically pause the
// game when the focus is lost.
#define TOFU_EVENTS_FOCUS_SUPPORT

// Controls the events related to the game-controller state changes. When
// enabled, events will be fired whenever a controller is attached/detached.
// Also, when the last controller is disconnected (or reconnected), suitable
// events will be issued.
#define TOFU_EVENTS_CONTROLLER_SUPPORT

// ############
// ### File ###
// ############

// Enables additional debug information for the `File` sub-system. This should
// normally be disabled in the `RELEASE` build and, on `DEBUG` build, only
// occasionally for explicit (and specific)... ehm, debug. Otherwise the I/O
// could be hindered.
#undef  TOFU_FILE_DEBUG_ENABLED

// The `File` sub-system supports multiple mount-points. This macro controls
// the behaviour when scanning for a file and, if defined, the file instance
// present in "last to be mounted" (with the `FS_attach_folder_or_archive()`
// function) archive/folder will be accessed. We call this "mount-override" as
// it enables a file to be present in more than an archive/folder, with only
// one instance to be used.
//
// In the context of the game-engine, it means that a file in the `data`
// archive/folder can have the same name of a `kernal` counterpart *and*
// override/redefine its implementation.
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

// ###################
// ### Interpreter ###
// ###################

// When using protected-call we also want that a custom trace-back callback is
// used. This will report a simpler output.
#define TOFU_INTERPRETER_CUSTOM_TRACEBACK

#define TOFU_INTERPRETER_GC_REPORTING

// Selects the garbage-collector type. Could be either `GC_TYPE_INCREMENTAL` or
// `GC_TYPE_GENERATIONAL`.
//
// An *incremental* garbage-collector reduces the length of the pause from
// the script execution but doesn't reduce the overhead of the GC phase.
//
// A *generational* garbage-collector works better when this hypothesis holds:
// most objects die young. In case of a game-engine this is rarely true, to
// the point that we use pools of actors/entities that are reused.
//
// For this reason the *incremental* type is suggested.
#define TOFU_INTERPRETER_GC_TYPE GC_TYPE_INCREMENTAL

// Selects the mode under which the garbage-collection is performed during the
// game-engine lifetime. It ca ben one of the following values:
//
// - GC_MODE_AUTOMATIC
//   Garbage-collection is carried out by the Lua virtual-machine according to
//   it's internal logic (e.g. executed incrementally and a clean-cycle is
//   forced when the amount of memory to be freed is significant).
//
// - GC_MODE_CONTINUOUS
//   A single GC step is performed periodically at a fixed time-step (i.e.
//   every 100 milliseconds, please see the `GC_CONTINUOUS_STEP_PERIOD` macro)
//   so that the overhead is distributed over time.
//
// - GC_MODE_PERIODIC
//   Every `GC_COLLECTION_PERIOD` seconds a full garbage-collection cycle is
//   forced. This could have a non trivial overhead.
//
// - GC_MODE_MANUAL
//   no autonomous garbage-collection is performed by the game-engine. It is
//   duty of the programmer to call the `collectgarbage()` function when desired
//   (e.g. during the level loading process).
//
// For small-sized projects, probably `GC_MODE_AUTOMATIC` is advisable. For
// mid-sized project either `GC_MODE_PERIODIC` or `GC_MODE_CONTINUOUS` are
// suggested (with the latter giving the most consistent behaviour). On large
// projects, or where performance really matters, `GC_MODE_MANUAL` is to be used
// as it gives the programmer full control on when the GC is to be used.
#define TOFU_INTERPRETER_GC_MODE GC_MODE_CONTINUOUS

// Enforces 'lua_pcall()' over (faster) 'lua_call()' when calling the scripting
// sub-system callbacks (e.g. `update()`). This will ensure that any potential
// error will be handled and reported with a detailed trace-back.
//
// Note: in the `RELEASE` build this macro is advised to be disable, as
//       protected-calls are *slower* than raw-calls.
#define TOFU_INTERPRETER_PROTECTED_CALLS

// As the game-engine uses a Lua custom reader we have to freedom to set the
// the I/O buffer with any size we like. If not defined, a default value of
// 1024 bytes is used.
//
// Note: the buffer is used to read Lua code, which typically is not that
//       large. Using huge buffers is pointless.
#define TOFU_INTERPRETER_READER_BUFFER_SIZE 1024U

// #############
// ### Input ###
// #############

// The game-engine periodically checks for a new controller attached to the
// system. This is the period (in seconds) for successive checks.
//
// Note: on the contrary, the *disconnection* of a controller is detected and
//       handled in real-time as the APIs used will fail in case a controller
//       is suddenly missing.
#define TOFU_INPUT_CONTROLLER_DETECTION_PERIOD 10.0

// Determines if controllers `#0` and `#1` are to be emulated with the keyboard
// mappings.
#define TOFU_INPUT_CONTROLLER_IS_EMULATED

// Similarly to the `TOFU_INPUT_CONTROLLER_IS_EMULATED` macro, this one
// determines if the cursor (i.e. the mouse controller input on a PC) is to be
// emulated with the *right stick* of the first available controller.
#define TOFU_INPUT_CURSOR_IS_EMULATED

// #############
// ### Sound ###
// #############

// Configures the stereo *balance law* used by the sound sub-system, that is
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
#define TOFU_SOUND_BALANCE_LAW BALANCE_LAW_SINCOS

// Configures the stereo *panning law* used by the sound sub-system, that is
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
#define TOFU_SOUND_PANNING_LAW PANNING_LAW_CONSTANT_POWER_SINCOS

// The sound sub-system can support music preloading, that is the playing buffer
// is filled during the opening phase so that it will be available from the
// start and ready to be played. Unless explicitly required the advice is to
// leave this disabled.
#undef  TOFU_SOUND_MUSIC_PRELOAD

// ##############
// ### Script ###
// ##############

// This controls whether, in the engine Lua script API, the `Grid` UDT stores
// `integer` or `float`valued "cells". The suggested value is `integer` as it
// is more consistent.
#undef  TOFU_SCRIPT_GRID_INTEGER_VALUES

// ###############
// ### Storage ###
// ###############

// When the storage sub-system loads a resource into memory, it is put into a
// cache in order to speed-up further accesses. This value controls the total
// number of entries that can coexist in the cache; when a new resource is
// loaded *but* the threshold is exceeded the older entry is discarded.
//
// There's generally no need for a large cache as the resources are typically
// accessed only once and processed according to their respective format.
#define TOFU_STORAGE_CACHE_ENTRIES_LIMIT 32U

// When a resource is loaded and stored in the cache, unless the cache reaches
// its limit and the resource freed to make room for another one, it will
// be parked for a "while". The length of this period is specified (in seconds)
// by this macro.
//
// When an already cached resource is re-loaded it's age is reset.
//
// This enable a sort of automatic garbage-collection of the resources, that
// are released after some time.
//
// Similarly to the cache limit, there's generally no need for a resource to
// persist in the cache for long periods.
//
// Note: this value is used only if the macro `TOFU_STORAGE_AUTO_COLLECT` is
//       defined.
#define TOFU_STORAGE_RESOURCE_MAX_AGE 30.0

// If defined the macro will enable the automatic garbage-collection of the
// storage resources (that is, they will be automatically freed after a grace
// period of time).
//
// However, this come with the cost of "pumping" the sub-system at each
// frame-step.
//
// For that reason, to save performances, the macro can be disabled and
// resources are to be freed with the `Storage.flush()` API.
#define TOFU_STORAGE_AUTO_COLLECT

// In release build, disable VM calls debug and periodic collection for better
// performance.
#if defined(NDEBUG)
  #undef TOFU_ENGINE_PERFORMANCE_STATISTICS
  #undef TOFU_ENGINE_HEAP_STATISTICS
  #undef TOFU_FILE_DEBUG_ENABLED
  #undef TOFU_GRAPHICS_EUCLIDIAN_NEAREST_COLOR
  #undef TOFU_GRAPHICS_REPORT_SHADERS_ERRORS
  #undef TOFU_INTERPRETER_PROTECTED_CALLS
  #undef TOFU_INTERPRETER_GC_MODE GC_MODE_MANUAL
  #undef TOFU_INTERPRETER_GC_REPORTING
#endif

#endif  /* TOFU_CORE_CONFIG_H */
