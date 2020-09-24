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

// We are going to buffer 1 second of non-converted data. As long as the `SL_music_update()` function is called
// once half a second we are good. Since it's very unlikely we will run at less than 2 FPS... well, we can sleep well. :)
#define STREAMING_BUFFER_SIZE_IN_FRAMES     SL_FRAMES_PER_SECOND

// That's the size of a single chunk read in each `produce()` call. Can't be larger than the buffer size.
#define STREAMING_BUFFER_CHUNK_IN_FRAMES    (STREAMING_BUFFER_SIZE_IN_FRAMES / 4)

// An XM module is generated in stereo mode, which means that we need to handle a stereo source.
#define MODULE_OUTPUT_FORMAT                ma_format_s16
#define MODULE_OUTPUT_BYTES_PER_SAMPLE      2
#define MODULE_OUTPUT_SAMPLES_PER_CHANNEL   1
#define MODULE_OUTPUT_CHANNELS_PER_FRAME    2
#define MODULE_OUTPUT_BYTES_PER_FRAME       (MODULE_OUTPUT_CHANNELS_PER_FRAME * MODULE_OUTPUT_SAMPLES_PER_CHANNEL * MODULE_OUTPUT_BYTES_PER_SAMPLE)

#define MIXING_BUFFER_SAMPLES_PER_CHANNEL   SL_SAMPLES_PER_CHANNEL
#define MIXING_BUFFER_CHANNELS_PER_FRAME    SL_CHANNELS_PER_FRAME
#define MIXING_BUFFER_SIZE_IN_FRAMES        128

#define MIXING_BUFFER_BYTES_PER_FRAME       (MIXING_BUFFER_CHANNELS_PER_FRAME * MIXING_BUFFER_SAMPLES_PER_CHANNEL * SL_BYTES_PER_SAMPLE)
#define MIXING_BUFFER_SIZE_IN_BYTES         (MIXING_BUFFER_SIZE_IN_FRAMES * MIXING_BUFFER_BYTES_PER_FRAME)

#define LOG_CONTEXT "sl-module"

typedef struct _Module_t {
    Source_VTable_t vtable;

    SL_Props_t props;

    xmp_context context;
    struct xmp_frame_info frame_info;

    ma_pcm_rb buffer;
    bool completed;
} Module_t;

static bool _module_ctor(SL_Source_t *source, const SL_Context_t *context, SL_Callbacks_t callbacks);
static void _module_dtor(SL_Source_t *source);
static bool _module_reset(SL_Source_t *source);
static bool _module_update(SL_Source_t *source, float delta_time);
static bool _module_mix(SL_Source_t *source, void *output, size_t frames_requested);

static inline bool _rewind(Module_t *module)
{
    xmp_restart_module(module->context);

    module->completed = false;

    return true;
}

static inline bool _reset(Module_t *module)
{
    ma_pcm_rb *buffer = &module->buffer;

    ma_pcm_rb_reset(buffer);

    return _rewind(module);
}

static inline bool _produce(Module_t *module)
{
    if (module->completed) { // End-of-data, early exit.
        return true;
    }

    ma_pcm_rb *buffer = &module->buffer;
    ma_uint32 frames_to_produce = ma_pcm_rb_available_write(buffer);
#ifdef STREAMING_BUFFER_CHUNK_IN_FRAMES
    if (frames_to_produce > STREAMING_BUFFER_CHUNK_IN_FRAMES) {
        frames_to_produce = STREAMING_BUFFER_CHUNK_IN_FRAMES;
    }
#endif

    void *write_buffer;
    ma_pcm_rb_acquire_write(buffer, &frames_to_produce, &write_buffer);

    // The requested buffer size (in bytes) is always filled, with trailing zeroes if needed.
    xmp_context context = module->context;
    const int loops = module->props.looping ? 0 : 1; // Automatically loop (properly filling the internal buffer), or tell EOD when not looping.
    int play_result = xmp_play_buffer(context, write_buffer, frames_to_produce * MODULE_OUTPUT_BYTES_PER_FRAME, loops);

    ma_pcm_rb_commit_write(buffer, frames_to_produce, write_buffer);

    if (play_result == -XMP_END) {
        module->completed = true;
    } else
    if (play_result != 0) { // Mark the end-of-data for both "end" and "error state" cases.
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "module %p in error state %d, forcing end-of-data", module, play_result);
        return false;
    }

    return true;
}

SL_Source_t *SL_module_create(const SL_Context_t *context, SL_Callbacks_t callbacks)
{
    Module_t *module = malloc(sizeof(Module_t));
    if (!module) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate module structure");
        return NULL;
    }

    bool cted = _module_ctor(module, context, callbacks);
    if (!cted) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize module structure");
        free(module);
        return NULL;
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "module %p created", module);
    return module;
}

static size_t _xmp_read(void *buffer, size_t size, size_t amount, void *user_data)
{
    SL_Callbacks_t *callbacks = (SL_Callbacks_t *)user_data;
    return callbacks->read(callbacks->user_data, buffer, size * amount) / size; // Convert from and to `fread()` values.
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

static bool _module_ctor(SL_Source_t *source, const SL_Context_t *context, SL_Callbacks_t callbacks)
{
    Module_t *module = (Module_t *)source;

    *module = (Module_t){
            .vtable = (Source_VTable_t){
                    .dtor = _module_dtor,
                    .reset = _module_reset,
                    .update = _module_update,
                    .mix = _module_mix
                },
            .completed = false
        };

    module->context  = xmp_create_context();

    int loaded = xmp_load_module_from_callbacks(module->context, _xmp_read, _xmp_seek, _xmp_tell, _xmp_eof, &callbacks);
    if (loaded != 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create module context");
        xmp_free_context(module->context);
        return false;
    }

    ma_result result = ma_pcm_rb_init(INTERNAL_FORMAT, MODULE_OUTPUT_CHANNELS_PER_FRAME, STREAMING_BUFFER_SIZE_IN_FRAMES, NULL, NULL, &module->buffer);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize music ring-buffer (%d frames)", STREAMING_BUFFER_SIZE_IN_FRAMES);
        xmp_release_module(module->context);
        xmp_free_context(module->context);
        return false;
    }

    bool initialized = SL_props_init(&module->props, context, MODULE_OUTPUT_FORMAT, SL_FRAMES_PER_SECOND, MODULE_OUTPUT_CHANNELS_PER_FRAME, MIXING_BUFFER_CHANNELS_PER_FRAME);
    if (!initialized) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize module properties");
        xmp_release_module(module->context);
        xmp_free_context(module->context);
        return false;
    }

    xmp_start_player(module->context, SL_FRAMES_PER_SECOND, 0);

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
    Module_t *module = (Module_t *)source;

    return _produce(module);
}

static bool _module_mix(SL_Source_t *source, void *output, size_t frames_requested)
{
    Module_t *module = (Module_t *)source;

    ma_data_converter *converter = &module->props.converter;
    ma_pcm_rb *buffer = &module->buffer;

    uint8_t converted_buffer[MIXING_BUFFER_SIZE_IN_BYTES];

    const SL_Mix_t mix = module->props.precomputed_mix;

    uint8_t *cursor = (uint8_t *)output;

    size_t frames_remaining = frames_requested;
    while (frames_remaining > 0) {
        ma_uint32 frames_available = ma_pcm_rb_available_read(buffer);
        if (frames_available == 0) {
            if (!module->completed) {
                Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "buffer underrun for source %p - stalling (waiting for data)", source);
                return true;
            } else {
                Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "end-of-data reached for source %p", source);
                return false;
            }
        }

        size_t frames_to_generate = frames_remaining > MIXING_BUFFER_SIZE_IN_FRAMES ? MIXING_BUFFER_SIZE_IN_FRAMES : frames_remaining;

        ma_uint32 frames_to_consume = ma_data_converter_get_required_input_frame_count(converter, frames_to_generate);
        if (frames_to_consume > MIXING_BUFFER_SIZE_IN_FRAMES) {
            frames_to_consume = MIXING_BUFFER_SIZE_IN_FRAMES;
        }

        void *consumed_buffer;
        ma_pcm_rb_acquire_read(buffer, &frames_to_consume, &consumed_buffer);

        ma_uint64 frames_consumed = frames_to_consume;
        ma_uint64 frames_generated = frames_to_generate;
        ma_data_converter_process_pcm_frames(converter, consumed_buffer, &frames_consumed, converted_buffer, &frames_generated);

        ma_pcm_rb_commit_read(buffer, frames_consumed, consumed_buffer);

        mix_2on2_additive(cursor, converted_buffer, frames_generated, mix);
        cursor += frames_generated * SL_BYTES_PER_FRAME;
        frames_remaining -= frames_generated;
    }
    return true;
}
