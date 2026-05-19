/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2026 Marco Lizza
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
#define BACKEND_GLFW 0
#define BACKEND_RGFW 1
#define BACKEND_SDL3 2

#define COLOR_MATCH_EUCLIDIAN  0
#define COLOR_MATCH_WEIGHTED   1
#define COLOR_MATCH_PERCEPTUAL 2
#define COLOR_MATCH_OCTREE     3

#define GC_REPORTING_PERIOD 5.0f

#define GC_TYPE_INCREMENTAL  0
#define GC_TYPE_GENERATIONAL 1

#define GC_MODE_AUTOMATIC  0
#define GC_MODE_CONTINUOUS 1
#define GC_MODE_MANUAL     2

#define BALANCE_LAW_LINEAR    0
#define BALANCE_LAW_SINCOS    1
#define BALANCE_LAW_SQRT      2

#define PANNING_LAW_CONSTANT_GAIN           0
#define PANNING_LAW_CONSTANT_POWER_SINCOS   1
#define PANNING_LAW_CONSTANT_POWER_SQRT     2

#define PIXEL_FORMAT_RGBA8888 0
#define PIXEL_FORMAT_RGB565   1

// #############
// ### Audio ###
// #############

// When defined, the audio mixing is performed using floating point format. This
// allows for a simpler and more consistent mixing, but it can be more CPU
// intensive on some platforms. When not defined, the mixing is performed using
// signed 16-bit integer format.
//
// The suggested setting, for more accurate result, is to enable this macro and
// disable `TOFU_AUDIO_CLAMP_DURING_MIXING`, to allow for some better overall
// sample "blending" (where one sample could "correct" the other one, instead
// of just clipping).
#undef  TOFU_AUDIO_FLOATING_POINT_MIXING

// We can choose to clamp the mixed sample during the mixing process, that is
// for every sample that is accumulated/mixed.
//
// When not using floating point format, disabling this option will just let
// the sample value to overflow and wrap around, which is what happens in the
// real hardware.
//
// Otherwise, when using floating point format, disabling this option will just
// let the audio backend to clip the samples during just before sending them to
// the audio device, which is more efficient than clamping every sample during
// the mixing process.
//
// The suggested setting is to disable this option, and if more accurate mixing
// is desired, to enable `TOFU_AUDIO_FLOATING_POINT_MIXING` as well.
#undef  TOFU_AUDIO_CLAMP_DURING_MIXING

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

// ############
// ### Core ###
// ############

// Configures the multi-plaform backend to be used. Valid choices are:
//
//   - BACKEND_GLFW
//   - BACKEND_RGFW (to be done)
//   - BACKEND_SDL3 (to be done)
#define TOFU_CORE_BACKEND BACKEND_GLFW

// Controls the use of OpenGL/ES for rendering.
//
// Note: for the moment being the setting should be used only for "mobile"
//       builds, but in the future it will be used as common backend.
#undef  TOFU_CORE_OPENGL_ES

// Forces the usage of the (deprecated) POSIX API `usleep()`, instead of the
// more efficient and supported `nanosleep()`.
//
// Note: this is not valid for the Windows build.
#undef  TOFU_CORE_USE_USLEEP

// Uses the preferred `clock_nanosleep()` function over `nanosleep()` for
// a more consistent (micro) wait, as a monotonic times is used.
//
// Note: this is not valid for the Windows build.
#undef  TOFU_CORE_USE_CLOCK_NANOSLEEP

// Define if you want the I/O and processing profiling to be enabled
// regardless of the build mode. Otherwise, it will be automatically enabled
// only for the `DEBUG` build.
#define TOFU_CORE_PROFILING_ENABLED

// Includes additional but very performance limiting debug information. This
// should be left disabled, unless you are tracking some bug.
#undef  TOFU_CORE_VERBOSE_DEBUG

// Includes checks inside some crucial functions. Could be useful in DEBUG mode.
#define TOFU_CORE_DEFENSIVE_CHECKS

// When define the following macro enables "faster" integer and float math
// operations. See the `imath.h` and `fmath.h` sources.
#undef  TOFU_CORE_FAST_MATH

// ##############
// ### Engine ###
// ##############

// Takes into account the actual wait time, during the frame capping, and
// compensate (in the next frame) if the wait is longer than the requested
// amount. This can occasionally happen in some slower systems where there are
// occasional system "hiccups".
#undef  TOFU_ENGINE_WAIT_SKID_COMPENSATION

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
//
// Note: the default engine behaviour is to use archives, not folders.
#define TOFU_ENGINE_DATA_NAME "data.pak"
#define TOFU_ENGINE_KERNAL_NAME "kernal.pak"

// Enables the *heap* statistics tracking. Currently both the game-engine and
// the VM total heap usage are fetched. The data can be accessed with the
// `System.heap()` method.
#define TOFU_ENGINE_HEAP_STATISTICS

// When defined, heap statistics area periodically logged.
#undef  TOFU_ENGINE_HEAP_STATISTICS_DEBUG

// Controls the period (in seconds) of the aforementioned debug heap statistics.
#define TOFU_ENGINE_HEAP_STATISTICS_PERIOD 5.0f

// Enables the *performance* statistics feature of the engine. This means that
// detailed "deltas" times are calculated on every game-loop iteration (process,
// update, render, and total delta-times). The data can be accessed with the
// `System.stats()` method.
//
// Note: this feature is meant to be used only in the `DEBUG` build of the
//       game engine due to its overhead.
#define TOFU_ENGINE_PERFORMANCE_STATISTICS

// Enables a more costly (but precise) performance average calculation, based
// upon a moving average.
#undef  TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE

// When the moving average is enabled this value tells the amount of samples
// the average is calculated upon.
#define TOFU_ENGINE_PERFORMANCE_MOVING_AVERAGE_SAMPLES 128

// When defined, performance and heap statistics is periodically be outputted
// as logging information. This is useful to track detailed information for a
// specific game-loop phase.
#define TOFU_ENGINE_STATISTICS_DEBUG

// Controls the period (in seconds) of the aforementioned debug statistics.
#define TOFU_ENGINE_STATISTICS_PERIOD 10.0f

// When defined, include the Lua profiling support in the engine. This is
// normally disabled in the `RELEASE` build.
#define TOFU_ENGINE_SCRIPT_LEVEL_PROFILING

// ############
// ### File ###
// ############

// Enables additional debug information for the `File` sub-system. This should
// normally be disabled in the `RELEASE` build and, on `DEBUG` build, only
// occasionally for explicit (and specific)... ehm, debug. Otherwise the I/O
// could be hindered.
#undef  TOFU_FILE_DEBUG_ENABLED

// ################
// ### Graphics ###
// ################

// This macro defines the pixel format used for the internal VRAM texture.
// Valid choices are:
//
// - `PIXEL_FORMAT_RGBA8888`
// - `PIXEL_FORMAT_RGB565`
//
// The default is `PIXEL_FORMAT_RGB565`, as it offers a good compromise
// between quality and performance. Since this is and pixel-arts oriented engine
// the loss of color depth is generally not an issue.
//
// `PIXEL_FORMAT_RGBA8888` is the original format used by the engine, but it
// is generally overkill for the kind of graphics the engine is supposed to
// handle. However, it requires not conversion at all from the GPU when
// transferring the VRAM texture to the screen, so it could be useful in some
// specific cases.
#define TOFU_GRAPHICS_PIXEL_FORMAT PIXEL_FORMAT_RGB565

// The rendering system features texture buffering, to decouple the UPLOAD
// (RAM to texture) and DRAW (texture to framebuffer) phases. This is convenient
// to avoid the chance the DRAW stalls because the upload is still in progress
// for the texture.
//
// Usually a DOUBLE buffering system is more than enough, especially considered
// this is 2D low-res game engine.
#define TOFU_GRAPHICS_RENDER_BUFFERS_COUNT 2

// Even by adopting a texture buffering technique the UPLOAD to the texture
// (CPU-to-GPU) can become a potential synchronization that stalls the rendering
// for a while as the CPU will wait for the transfer to be completed before
// resuming execution. We can "detach" the whole process by mean of *pixel
// buffer objects*.
//
// These involved macros are the following:
//
//   - `TOFU_GRAPHICS_ASYNC_UPLOAD`, to activate the UPLOAD async
//     transfer,
#define TOFU_GRAPHICS_ASYNC_UPLOAD
//   - `TOFU_GRAPHICS_ASYNC_UPLOAD_BUFFERS_COUNT`, to set the amount of slots
//     in the upload ring-buffer (default is `3` if not set),
#define TOFU_GRAPHICS_ASYNC_UPLOAD_BUFFERS_COUNT 3
//   - `TOFU_GRAPHICS_ASYNC_UPLOAD_ORPHAN_RECOVERING`, controls the behaviour
//     in the unfortunate chance the upload process can't complete. If set
//     the old buffer is "orphaned" and a new one is requested. This is very
//     unlikely to happen, but nonetheless is nice to have it (since it's
//     trivial to implement).
#define TOFU_GRAPHICS_ASYNC_UPLOAD_ORPHAN_RECOVERING

// Enables additional debug information for the `Graphics` sub-system. This
// should normally be disabled in *both* the `DEBUG` and the `RELEASE` builds
// as it is to be used only occasionally for explicit (and specific)... ehm,
// debug.
#undef  TOFU_GRAPHICS_DEBUG_ENABLED

// Optionally logs any access to an undefined shader uniform variable. It is
// advisable to define this macro only occasionally to clean/spot any unused
// variable.
#undef  TOFU_GRAPHICS_REPORT_SHADERS_ERRORS

// Also, we can check for runtime errors coming from the OpenGL system. The
// underlying usage of the `glGetError()` API can introduce stalls in the GPU
// operations, so we don't call it on each frame. Also, this will be disabled
// in the `RELEASE` build.
#define TOFU_GRAPHICS_REPORT_OPENGL_ERRORS

// OpenGL model-view-projection matrix is used only in the projection shader,
// so we are not required to store a copy of it. Anyway, in some cases it might
// prove useful, so we could prefer to save it for later uses.
#undef  TOFU_GRAPHICS_SAVE_MVP_MATRIX

// Controls if the filled-triangle rasterizer algorithm requires the vertices
// to be passed in clockwise (if defined) or counter-clockwise (if not defined)
// order.
#undef  TOFU_GRAPHICS_CLOCKWISE_RASTERIZER_WINDING

// Enables an automatic check and on-the-fly fix of the vertices order in case
// they are not passed in the order the engine requires.
#define TOFU_GRAPHICS_FIX_RASTERIZER_WINDING

// Enables OpenGL's back-face culling in order to detect (and debug) when a
// polygon is wrongly represented.
#undef  TOFU_GRAPHICS_DEBUG_TRIANGLES_WINDING

// When defined the initial (default) palette will be automatically generated
// as color-quantized. This will result in a gradient of colors that will span
// the whole spectrum (using the size of the palette as step).
//
// If not defined, the default palette will be a greyscale gradient.
#undef  TOFU_GRAPHICS_DEFAULT_PALETTE_IS_QUANTIZED

// Enables the use of branchless calculations in the blitting process. The code
// is more efficient but it can be less accurate in some cases, especially when
// the scale is close to zero.
#undef  TOFU_GRAPHICS_BRANCHLESS_CALCULATIONS

// We can optimize the rotated AABB calculation by avoiding rotating all the
// corners of the rectangle, and just calculating the rotated half-extent. This
// will save half of the rotation calculations.
#define TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS

// Enabling the "fast path" for blitting operations means that when operating
// with no rotation and/ or no scaling the most efficient `GL_context_blit*()`
// function will be called according to the actual parameters. This optimization
// applies only to the sheet-based blitting functions.
//
// Note: some small differences can be observed when the enabled: the
//       zero-rotation case can differ slightly from the general case due
//       to fractional-scale and pivot-to-top-left rounding.
#define TOFU_GRAPHICS_BLIT_FAST_PATH

// Controls the algorithm used to match similar colors during the image indexing
// process (i.e. finding the best matching palette color). The following modes
// are available:
//
// - COLOR_MATCH_EUCLIDIAN (simpler)
//   Each color is treated as a three-components vector and the (squared)
//   Euclidian distance is used to find the nearest color.
//
// - COLOR_MATCH_WEIGHTED (best compromise)
//   Similar to the Euclidian distance but it takes into account also the
//   "red-mean" and it's more consistent across the color spectrum, relatively
//   to the human eye sensitivity to the RGB components.
//
// - COLOR_MATCH_PERCEPTUAL (slower)
//   Uses the CIELab color-space representation for the color, which adopts a
//   relatively perceptually uniform space, to find the best match (CIE76).
//   This algorithm is noticeably slower than the others and occasionally can
//   result in odds matching.
//
// - COLOR_MATCH_OCTREE (to be implemented)
//   Adopts an octree representations for the colors to detect the most similar
//   colors.
//
#define TOFU_GRAPHICS_COLOR_MATCHING_ALGORITHM COLOR_MATCH_WEIGHTED

// Determines whether the Mode7-like transformations will consider the surface
// as opaque or any (currently) transparent color will be discared.
//
// There's no silver-bullet here, as there are occasions in which transparency
// is required. If not defined, however, a minor boost in performance is
// granted.
#undef  TOFU_GRAPHICS_XFORM_TRANSPARENCY

// When defined, the surface-to-VRAM processor will be limited to a single
// command per pixel. Generally, this is not wanted as multiple commands per
// pixel are useful to create more sophisticated effects.
#undef  TOFU_GRAPHICS_PROCESSOR_ONE_COMMAND_PER_PIXEL

// Enables additional checks inside the processor functions to validate
// the correctness of the processor program. This is useful to track bugs in
// the program creation phase (for example, badly formed `WAIT` commands that
// could cause infinite wait-loops).
#define TOFU_GRAPHICS_PROCESSOR_DEFENSIVE_CHECKS

// ###################
// ### Interpreter ###
// ###################

// When using protected-call we also want that a custom trace-back callback is
// used. This will report a simpler output.
#define TOFU_INTERPRETER_CUSTOM_TRACEBACK

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
// game-engine lifetime. It can be one of the following values:
//
// - GC_MODE_AUTOMATIC
//   Garbage-collection is carried out by the Lua virtual-machine according to
//   it's internal logic (e.g. executed incrementally and a clean-cycle is
//   forced when the amount of memory to be freed is significant).
//
// - GC_MODE_CONTINUOUS
//   A single GC step is performed periodically at a fixed time-step (i.e.
//   at every VM update step) so that the overhead is distributed over time.
//
// - GC_MODE_MANUAL
//   no autonomous garbage-collection is performed by the game-engine. It is
//   duty of the programmer to call the `collectgarbage()` function when desired
//   (e.g. during the level loading process).
//
// For small-sized projects, probably `GC_MODE_AUTOMATIC` is advisable. For
// mid-sized project either `GC_MODE_CONTINUOUS` are suggested. On large
// projects, or where performance really matters, `GC_MODE_MANUAL` is to be used
#define TOFU_INTERPRETER_GC_MODE GC_MODE_CONTINUOUS

// When the `GC_MODE_CONTINUOUS` mode is enabled, Interpreter_collect()` is
// called periodically to perform a garbage-collection step. With this macro we
// can control if a single (indivisible) or a full (i.e. while it is actually
// completed) step is performed. Usually a *single* step is preferable as it is
// more consistent and less expensive, but it might be not sufficient to reclaim
// all the memory.
#define TOFU_INTERPRETER_GC_FULL_CYCLE

// Enforces 'lua_pcall()' over (faster) 'lua_call()' when calling the scripting
// sub-system callbacks (e.g. `update()`). This will ensure that any potential
// error will be handled and reported with a detailed trace-back.
//
// Note: in the `RELEASE` build this macro is advised to be disable, as
//       protected-calls are *slower* than raw-calls.
#define TOFU_INTERPRETER_PROTECTED_CALLS

// When this macro is defined, the main object (see the `boot-XXX.lua` files)
// can be partially implemented (i.e. some of its method can be missing). This
// is not the usual case, as the game API is usually fully implemented.
// Please note that supporting "partial objects" could slow down the execution
// as additional checks are introduced as a side-effect.
#undef  TOFU_INTERPRETER_PARTIAL_OBJECT

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

#if defined(NDEBUG)
  #undef TOFU_CORE_PROFILING_ENABLED
  #undef TOFU_CORE_DEFENSIVE_CHECKS
  #undef TOFU_ENGINE_STATISTICS_DEBUG
  #undef TOFU_ENGINE_PERFORMANCE_STATISTICS
  #undef TOFU_ENGINE_SCRIPT_LEVEL_PROFILING
  #undef TOFU_FILE_DEBUG_ENABLED
  #undef TOFU_GRAPHICS_REPORT_SHADERS_ERRORS
  #undef TOFU_GRAPHICS_REPORT_OPENGL_ERRORS
  #undef TOFU_GRAPHICS_PROCESSOR_DEFENSIVE_CHECKS
  #undef TOFU_INTERPRETER_PROTECTED_CALLS
#endif

#endif  /* TOFU_CORE_CONFIG_H */
