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

#include "sample.h"

#include "internals.h"
#include "mix.h"

#include <libs/log.h>
#include <libs/stb.h>

#define SAMPLE_MAX_LENGTH_IN_SECONDS    10.0f

#define MIXING_BUFFER_CHANNELS          1
#define MIXING_BUFFER_SIZE_IN_FRAMES    128

#define LOG_CONTEXT "sl-sample"

typedef struct _Sample_t {
    Source_VTable_t vtable;

    SL_Props_t props;

    SL_Callbacks_t callbacks;
    void *user_data;

    size_t length_in_frames;

    ma_audio_buffer buffer; // FIXME: is the buffer type the only difference?
    size_t frames_completed;
} Sample_t;

static bool _sample_ctor(SL_Source_t *source, SL_Callbacks_t callbacks, void *user_data, size_t length_in_frames, ma_format format, ma_uint32 sample_rate, ma_uint32 channels);
static void _sample_dtor(SL_Source_t *source);
static bool _sample_reset(SL_Source_t *source);
static bool _sample_update(SL_Source_t *source, float delta_time);
static bool _sample_mix(SL_Source_t *source, void *output, size_t frames_requested, const SL_Mix_t *mixes);

static inline bool _rewind(Sample_t *sample)
{
    ma_audio_buffer *buffer = &sample->buffer;

    ma_audio_buffer_seek_to_pcm_frame(buffer, 0); // Can't fail, we are rewind into memory (seeking frame is safe).

    sample->frames_completed = 0;

    return true;
}

static inline bool _reset(Sample_t *sample)
{
    return _rewind(sample);
}

static inline bool _produce(Sample_t *sample)
{
    const SL_Callbacks_t *callbacks = &sample->callbacks;
    ma_audio_buffer *buffer = &sample->buffer;

    void *write_buffer;
    ma_uint64 frames_available = sample->length_in_frames;
    ma_audio_buffer_map(buffer, &write_buffer, &frames_available); // No need to check the result, can't fail.

    size_t frames_produced = callbacks->read(sample->user_data, write_buffer, frames_available);

    ma_audio_buffer_unmap(buffer, frames_produced); // Ditto.

    ma_audio_buffer_seek_to_pcm_frame(buffer, 0); // Ditto.

    return frames_produced == sample->length_in_frames;
}

static inline Source_States_t _consume(Sample_t *sample, size_t frames_requested, void *output, size_t size_in_frames, size_t *frames_processed)
{
    ma_data_converter *converter = &sample->props.converter;
    ma_audio_buffer *buffer = &sample->buffer;

    *frames_processed = 0;

    uint8_t *cursor = output;

    size_t frames_remaining = (frames_requested > size_in_frames) ? size_in_frames : frames_requested;
    while (frames_remaining > 0) { // Read as much data as possible, filling the buffer and eventually looping!
        if (sample->frames_completed == sample->length_in_frames) {
            if (!sample->props.looping || !_rewind(sample)) {
                return SOURCE_STATE_EOD;
            }
        }

        ma_uint64 frames_to_convert = ma_data_converter_get_required_input_frame_count(converter, frames_remaining);

        void *read_buffer;
        ma_uint64 frames_available = frames_to_convert;
        ma_audio_buffer_map(buffer, &read_buffer, &frames_available); // No need to check the result, can't fail.

        ma_uint64 frames_consumed = frames_available;
        ma_uint64 frames_generated = frames_remaining;
        ma_data_converter_process_pcm_frames(converter, read_buffer, &frames_consumed, cursor, &frames_generated);

        ma_audio_buffer_unmap(buffer, frames_consumed); // Ditto.

        sample->frames_completed += frames_generated;

        cursor += frames_generated * MIXING_BUFFER_CHANNELS * SL_BYTES_PER_FRAME;

        *frames_processed += frames_generated;
        frames_remaining -= frames_generated;
    }

    return SOURCE_STATE_PLAYING;
}

SL_Source_t *SL_sample_create(SL_Callbacks_t callbacks, void *user_data, size_t length_in_frames, ma_format format, ma_uint32 sample_rate, ma_uint32 channels)
{
    if (channels != 1) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "samples need to be 1 channel");
        return NULL;
    }
    float duration = (float)length_in_frames / (float)sample_rate;
    if (duration > SAMPLE_MAX_LENGTH_IN_SECONDS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "sample is too long (%.2f seconds)", duration);
        return NULL;
    }

    Sample_t *sample = malloc(sizeof(Sample_t));
    if (!sample) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate sample structure");
        return NULL;
    }

    bool cted = _sample_ctor(sample, callbacks, user_data, length_in_frames, format, sample_rate, channels);
    if (!cted) {
        free(sample);
        return NULL;
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sample %p created", sample);
    return sample;
}

static bool _sample_ctor(SL_Source_t *source, SL_Callbacks_t callbacks, void *user_data, size_t length_in_frames, ma_format format, ma_uint32 sample_rate, ma_uint32 channels)
{
    Sample_t *sample = (Sample_t *)source;

    *sample = (Sample_t){
            .vtable = (Source_VTable_t){
                    .dtor = _sample_dtor,
                    .reset = _sample_reset,
                    .update = _sample_update,
                    .mix = _sample_mix
                },
            .callbacks = callbacks,
            .user_data = user_data,
            .length_in_frames = length_in_frames
        };

    ma_audio_buffer_config config = ma_audio_buffer_config_init(format, channels, length_in_frames, NULL, NULL);
    ma_result result = ma_audio_buffer_init_copy(&config, &sample->buffer); // NOTE: It will allocate but won't copy.
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate buffer for %d frames", length_in_frames);
        return false;
    }

    bool produced = _produce(sample);
    if (!produced) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read %d frames for sample", length_in_frames);
        ma_audio_buffer_uninit(&sample->buffer);
        return false;
    }

    bool initialized = SL_props_init(&sample->props, format, sample_rate, channels, MIXING_BUFFER_CHANNELS);
    if (!initialized) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize sample properties");
        ma_audio_buffer_uninit(&sample->buffer);
        return false;
    }

    return true;
}

static void _sample_dtor(SL_Source_t *source)
{
    Sample_t *sample = (Sample_t *)source;

    SL_props_deinit(&sample->props);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sample properties deinitialized");

    ma_audio_buffer_uninit(&sample->buffer);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sample buffer deinitialized");
}

static bool _sample_reset(SL_Source_t *source)
{
    Sample_t *sample = (Sample_t *)source;

    bool reset = _reset(sample);
    if (!reset) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't reset sample data");
        return false;
    }

    return true;
}

static bool _sample_update(SL_Source_t *source, float delta_time)
{
    return true; // NO-OP
}

static bool _sample_mix(SL_Source_t *source, void *output, size_t frames_requested, const SL_Mix_t *mixes)
{
    Sample_t *sample = (Sample_t *)source;

    uint8_t buffer[MIXING_BUFFER_SIZE_IN_FRAMES * MIXING_BUFFER_CHANNELS * SL_BYTES_PER_FRAME];

    const SL_Mix_t mix = SL_props_precompute(&sample->props, mixes);

    uint8_t *cursor = (uint8_t *)output;

    size_t frames_remaining = frames_requested;
    while (frames_remaining > 0) {
        size_t frames_processed;
        Source_States_t state = _consume(sample, frames_remaining, buffer, MIXING_BUFFER_SIZE_IN_FRAMES, &frames_processed);
        mix_1on2_additive(cursor, buffer, frames_processed, mix);
        if (state == SOURCE_STATE_EOD) {
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "end-of-data reached for source %p", source);
            return false;
        }
        cursor += frames_processed * SL_CHANNELS_PER_FRAME * MIXING_BUFFER_CHANNELS * SL_BYTES_PER_FRAME;
        frames_remaining -= frames_processed;
    }
    return true;
}
