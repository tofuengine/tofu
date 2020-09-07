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

#include "music.h"

#include "common.h"
#include "internals.h"
#include "mix.h"

#include <config.h>
#include <libs/log.h>
#include <libs/stb.h>
#include <xmp-lite/xmp.h>

#include <stdint.h>

// An XM module is generated in stereo mode, which means that the need to handle a stereo source.
#define MODULE_OUTPUT_FORMAT                ma_format_s16
#define MODULE_OUTPUT_BYTES_PER_SAMPLE      2
#define MODULE_OUTPUT_SAMPLES_PER_CHANNEL   1
#define MODULE_OUTPUT_CHANNELS_PER_FRAME    2
#define MODULE_OUTPUT_BYTES_PER_FRAME       (MODULE_OUTPUT_CHANNELS_PER_FRAME * MODULE_OUTPUT_SAMPLES_PER_CHANNEL * MODULE_OUTPUT_BYTES_PER_SAMPLE)

#define MIXING_BUFFER_SAMPLES_PER_CHANNEL   1
#define MIXING_BUFFER_CHANNELS_PER_FRAME    2
#define MIXING_BUFFER_SIZE_IN_FRAMES        128

#define MIXING_BUFFER_BYTES_PER_FRAME       (MIXING_BUFFER_CHANNELS_PER_FRAME * MIXING_BUFFER_SAMPLES_PER_CHANNEL * SL_BYTES_PER_SAMPLE)
#define MIXING_BUFFER_SIZE_IN_BYTES         (MIXING_BUFFER_SIZE_IN_FRAMES * MIXING_BUFFER_BYTES_PER_FRAME)

#define LOG_CONTEXT "sl-module"

typedef struct _Module_t {
    Source_VTable_t vtable;

    SL_Props_t props;

    xmp_context context;
    struct xmp_frame_info frame_info;
} Module_t;

static bool _module_ctor(SL_Source_t *source, SL_Callbacks_t callbacks);
static void _module_dtor(SL_Source_t *source);
static bool _module_reset(SL_Source_t *source);
static bool _module_update(SL_Source_t *source, float delta_time);
static bool _module_mix(SL_Source_t *source, void *output, size_t frames_requested, const SL_Group_t *groups);

static inline bool _rewind(Module_t *module)
{
    xmp_restart_module(module->context);

    return true;
}

static inline bool _reset(Module_t *module)
{
    return _rewind(module);
}

static inline Source_States_t _consume(Module_t *module, size_t frames_requested, void *output, size_t size_in_frames, size_t *frames_processed)
{
    ma_data_converter *converter = &module->props.converter;
    xmp_context context = module->context;

    *frames_processed = 0;

    uint8_t read_buffer[MIXING_BUFFER_SIZE_IN_BYTES];

    uint8_t *cursor = output;

    size_t frames_remaining = (frames_requested > size_in_frames) ? size_in_frames : frames_requested;
    while (frames_remaining > 0) { // Read as much data as possible, filling the buffer and eventually looping!
        ma_uint64 frames_to_convert = ma_data_converter_get_required_input_frame_count(converter, frames_remaining);

        ma_uint32 frames_to_consume = (frames_to_convert > MIXING_BUFFER_SIZE_IN_FRAMES) ? MIXING_BUFFER_SIZE_IN_FRAMES : frames_to_convert;

        int play_result = xmp_play_buffer(context, read_buffer, frames_to_consume * MODULE_OUTPUT_BYTES_PER_FRAME, 1); // Just a single run, but tell EOD.
        size_t frames_read = frames_to_consume; // The requested buffer size (in bytes) is always filled, with trailing zeroes if needed.

        ma_uint64 frames_consumed = frames_read;
        ma_uint64 frames_generated = frames_remaining;
        ma_data_converter_process_pcm_frames(converter, read_buffer, &frames_consumed, cursor, &frames_generated);

        cursor += frames_generated * MIXING_BUFFER_BYTES_PER_FRAME;

        *frames_processed += frames_generated;
        frames_remaining -= frames_generated;

        if (play_result == -XMP_END) { // Handle the result here, after the (possibly partially filled) buffer has been copied.
            if (!module->props.looping) {
                return SOURCE_STATE_EOD;
            } else {
                _rewind(module);
            }
        } else
        if (play_result == -XMP_ERROR_STATE) {
            Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "module %p in error state", module);
            return SOURCE_STATE_EOD;
        }
    }

    return SOURCE_STATE_PLAYING;
}

SL_Source_t *SL_module_create(SL_Callbacks_t callbacks)
{
    Module_t *module = malloc(sizeof(Module_t));
    if (!module) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate module structure");
        return NULL;
    }

    bool cted = _module_ctor(module, callbacks);
    if (!cted) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize module structure");
        free(module);
        return NULL;
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "module %p created", module);
    return module;
}

static size_t _xmp_read(void *user_data, void *buffer, size_t bytes_to_read)
{
    SL_Callbacks_t *callbacks = (SL_Callbacks_t *)user_data;
    return callbacks->read(callbacks->user_data, buffer, bytes_to_read);
}

static int _xmp_seek(void *user_data, long offset, int whence)
{
    SL_Callbacks_t *callbacks = (SL_Callbacks_t *)user_data;
    return callbacks->seek(callbacks->user_data, offset, whence) ? 0 : -1; // Convert to `fseek()` return values.
}

static long _xmp_tell(void *user_data)
{
    SL_Callbacks_t *callbacks = (SL_Callbacks_t *)user_data;
    return callbacks->tell(callbacks->user_data);
}

static int _xmp_eof(void *user_data)
{
    SL_Callbacks_t *callbacks = (SL_Callbacks_t *)user_data;
    return callbacks->eof(callbacks->user_data);
}

static bool _module_ctor(SL_Source_t *source, SL_Callbacks_t callbacks)
{
    Module_t *module = (Module_t *)source;

    *module = (Module_t){
            .vtable = (Source_VTable_t){
                    .dtor = _module_dtor,
                    .reset = _module_reset,
                    .update = _module_update,
                    .mix = _module_mix
                },
            .props = { 0 }
        };

    module->context  = xmp_create_context();

    int loaded = xmp_load_module_from_callbacks(module->context, _xmp_read, _xmp_seek, _xmp_tell, _xmp_eof, &callbacks);
    if (loaded != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create module context");
        xmp_free_context(module->context);
        return false;
    }

    xmp_start_player(module->context, SL_FRAMES_PER_SECOND, 0);

    bool initialized = SL_props_init(&module->props, MODULE_OUTPUT_FORMAT, SL_FRAMES_PER_SECOND, MODULE_OUTPUT_CHANNELS_PER_FRAME, MIXING_BUFFER_CHANNELS_PER_FRAME);
    if (!initialized) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize module properties");
        xmp_release_module(module->context);
        xmp_free_context(module->context);
        return false;
    }

    return true;
}

static void _module_dtor(SL_Source_t *source)
{
    Module_t *module = (Module_t *)source;

    SL_props_deinit(&module->props);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "module properties deinitialized");

    xmp_end_player(module->context);
    xmp_release_module(module->context);
    xmp_free_context(module->context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "module context freed");
}

static bool _module_reset(SL_Source_t *source)
{
    Module_t *module = (Module_t *)source;

    bool reset = _reset(module);
    if (!reset) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't reset module stream");
        return false;
    }

    return true;
}

static bool _module_update(SL_Source_t *source, float delta_time)
{
    return true; // NO-OP
}

static bool _module_mix(SL_Source_t *source, void *output, size_t frames_requested, const SL_Group_t *groups)
{
    Module_t *module = (Module_t *)source;

    uint8_t buffer[MIXING_BUFFER_SIZE_IN_BYTES];

    const SL_Mix_t mix = SL_props_precompute(&module->props, groups);

    uint8_t *cursor = (uint8_t *)output;

    size_t frames_remaining = frames_requested;
    while (frames_remaining > 0) {
        size_t frames_processed; // FIXME: use this as the return value of the function below.
        Source_States_t state = _consume(module, frames_remaining, buffer, MIXING_BUFFER_SIZE_IN_FRAMES, &frames_processed);
        mix_2on2_additive(cursor, buffer, frames_processed, mix);
        if (state == SOURCE_STATE_EOD) {
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "end-of-data reached for source %p", source);
            return false;
        }
        cursor += frames_processed * SL_BYTES_PER_FRAME;
        frames_remaining -= frames_processed;
    }
    return true;
}
