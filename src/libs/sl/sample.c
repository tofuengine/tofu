/*
 * MIT License
 *
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "internal.h"
#include "mix.h"

#include <core/config.h>
#include <libs/dr_libs.h>
#define _LOG_TAG "sl-sample"
#include <libs/log.h>
#include <libs/stb.h>

#include <stdint.h>

// When defined this macro will pose a limit in maximum the length of a sample, as it is unpractical to have
// too-long samples. However, we could also don't limit it and just leave it to the user.
#define _SAMPLE_MAX_LENGTH_IN_SECONDS      10.0f

// Samples differs from musics and modules since they are *mono* in nature. We have just *one* channel per frame.
// This means that we will be using the `mix_1on2_additive()` mixing function to duplicate the mono channel to stereo.
#define _MIXING_BUFFER_BYTES_PER_SAMPLE    SL_BYTES_PER_SAMPLE
#define _MIXING_BUFFER_SAMPLES_PER_CHANNEL SL_SAMPLES_PER_CHANNEL
#define _MIXING_BUFFER_CHANNELS_PER_FRAME  1
#define _MIXING_BUFFER_SIZE_IN_FRAMES      SL_MIXING_BUFFER_SIZE_IN_FRAMES

#define _MIXING_BUFFER_BYTES_PER_FRAME     (_MIXING_BUFFER_CHANNELS_PER_FRAME * _MIXING_BUFFER_SAMPLES_PER_CHANNEL * _MIXING_BUFFER_BYTES_PER_SAMPLE)
#define _MIXING_BUFFER_SIZE_IN_BYTES       (_MIXING_BUFFER_SIZE_IN_FRAMES * _MIXING_BUFFER_BYTES_PER_FRAME)

typedef struct Sample_s {
    Source_VTable_t vtable;

    SL_Props_t *props;

    SL_Callbacks_t callbacks;

    drflac *decoder;
    size_t length_in_frames;

    uint8_t mixing_buffer[_MIXING_BUFFER_SIZE_IN_BYTES];

    ma_audio_buffer buffer;
    size_t frames_completed;
} Sample_t;

static bool _sample_ctor(SL_Source_t *source, const SL_Context_t *context, SL_Callbacks_t callbacks);
static void _sample_dtor(SL_Source_t *source);
static bool _sample_reset(SL_Source_t *source);
static bool _sample_update(SL_Source_t *source, float delta_time);
static bool _sample_generate(SL_Source_t *source, void *output, size_t frames_requested);

static inline bool _rewind(Sample_t *sample)
{
    LOG_T("rewinding sample %p", sample);

    ma_audio_buffer *buffer = &sample->buffer;
    ma_audio_buffer_seek_to_pcm_frame(buffer, 0); // Can't fail, we are rewinding into memory (frame-seeking is safe).

    sample->frames_completed = 0;

    return true;
}

static inline bool _reset(Sample_t *sample)
{
    LOG_T("rewinding sample %p", sample);

    return _rewind(sample);
}

static inline bool _produce(Sample_t *sample)
{
    ma_audio_buffer *buffer = &sample->buffer;

    void *write_buffer;
    ma_uint64 frames_available = sample->length_in_frames;
    ma_audio_buffer_map(buffer, &write_buffer, &frames_available); // No need to check the result, can't fail.

#if SL_BYTES_PER_SAMPLE == 2
    size_t frames_produced = drflac_read_pcm_frames_s16(sample->decoder, frames_available, write_buffer);
#elif SL_BYTES_PER_SAMPLE == 4
    size_t frames_produced = drflac_read_pcm_frames_f32(sample->decoder, frames_available, write_buffer);
#endif

    ma_audio_buffer_unmap(buffer, frames_produced); // Ditto.

    ma_audio_buffer_seek_to_pcm_frame(buffer, 0); // Ditto.

    return frames_produced == sample->length_in_frames;
}

SL_Source_t *SL_sample_create(const SL_Context_t *context, SL_Callbacks_t callbacks)
{
    SL_Source_t *sample = malloc(sizeof(Sample_t));
    if (!sample) {
        LOG_E("can't allocate sample structure");
        goto error_exit;
    }

    bool cted = _sample_ctor(sample, context, callbacks);
    if (!cted) {
        goto error_free_sample;
    }

    LOG_D("sample %p created", sample);
    return sample;

error_free_sample:
    free(sample);
error_exit:
    return NULL;
}

static size_t _sample_read(void *user_data, void *buffer, size_t bytes_to_read)
{
    const Sample_t *sample = (const Sample_t *)user_data;
    const SL_Callbacks_t *callbacks = &sample->callbacks;

    return callbacks->read(callbacks->user_data, buffer, bytes_to_read);
}

static drflac_bool32 _sample_seek(void *user_data, int offset, drflac_seek_origin origin)
{
    const Sample_t *sample = (const Sample_t *)user_data;
    const SL_Callbacks_t *callbacks = &sample->callbacks;

    bool sought = false;
    if (origin == drflac_seek_origin_start) {
        sought = callbacks->seek(callbacks->user_data, offset, SEEK_SET);
    } else
    if (origin == drflac_seek_origin_current) {
        sought = callbacks->seek(callbacks->user_data, offset, SEEK_CUR);
    }
    return sought ? DRFLAC_TRUE : DRFLAC_FALSE;
}

static void *_malloc(size_t sz, void *pUserData) // FIXME: move to custom library.
{
    return malloc(sz);
}

static void *_realloc(void *ptr, size_t sz, void *pUserData)
{
    return realloc(ptr, sz);
}

static void  _free(void *ptr, void *pUserData)
{
    free(ptr);
}

static bool _sample_ctor(SL_Source_t *source, const SL_Context_t *context, SL_Callbacks_t callbacks)
{
    Sample_t *sample = (Sample_t *)source;

    *sample = (Sample_t){
            .vtable = (Source_VTable_t){
                .dtor = _sample_dtor,
                .reset = _sample_reset,
                .update = _sample_update,
                .generate = _sample_generate
            },
            .callbacks = callbacks,
            .frames_completed = 0
        };

    sample->decoder = drflac_open(_sample_read, _sample_seek, sample, &(drflac_allocation_callbacks){
            .pUserData = NULL,
            .onMalloc  = _malloc,
            .onRealloc = _realloc,
            .onFree    = _free
        });
    if (!sample->decoder) {
        LOG_E("can't create sample decoder");
        goto error_exit;
    }

    sample->length_in_frames = sample->decoder->totalPCMFrameCount;
    if (sample->length_in_frames == 0) {
        LOG_E("can't create sample w/ zero length");
        goto error_close_decoder;
    }

    size_t channels = sample->decoder->channels;
    size_t sample_rate = sample->decoder->sampleRate;
    size_t bits_per_sample = sample->decoder->bitsPerSample;
    LOG_D("sample decoder %p initialized w/ %d frames, %d channels, %dHz, %d bits", sample->decoder, sample->length_in_frames, channels, sample_rate, bits_per_sample);

    if (channels != 1) {
        LOG_E("samples need to be monaural (i.e. w/ 1 channel)");
        goto error_close_decoder;
    }

#if defined(_SAMPLE_MAX_LENGTH_IN_SECONDS)
    float duration = (float)sample->length_in_frames / (float)sample_rate;
    if (duration > _SAMPLE_MAX_LENGTH_IN_SECONDS) {
        LOG_E("sample is too long (%.2f seconds)", duration);
        goto error_close_decoder;
    }
#endif

    ma_audio_buffer_config config = ma_audio_buffer_config_init(INTERNAL_FORMAT, channels, sample->length_in_frames, NULL, NULL);
    ma_result result = ma_audio_buffer_init_copy(&config, &sample->buffer); // NOTE: It will allocate but won't copy.
    if (result != MA_SUCCESS) {
        LOG_E("can't allocate buffer for %d frames", sample->length_in_frames);
        goto error_close_decoder;
    }

    bool produced = _produce(sample);
    if (!produced) {
        LOG_E("can't read %d frames for sample", sample->length_in_frames);
        goto error_deinitialize_audio_buffer;
    }

    sample->props = SL_props_create(context, INTERNAL_FORMAT, sample_rate, channels, _MIXING_BUFFER_CHANNELS_PER_FRAME);
    if (!sample->props) {
        LOG_E("can't initialize sample properties");
        goto error_deinitialize_audio_buffer;
    }

    return true;

error_deinitialize_audio_buffer:
    ma_audio_buffer_uninit(&sample->buffer);
error_close_decoder:
    drflac_close(sample->decoder);
error_exit:
    return false;
}

static void _sample_dtor(SL_Source_t *source)
{
    Sample_t *sample = (Sample_t *)source;

    SL_props_destroy(sample->props);
    LOG_D("sample properties destroyed");

    ma_audio_buffer_uninit(&sample->buffer);
    LOG_D("sample buffer uninitialized");

    drflac_close(sample->decoder);
    LOG_D("sample decoder closed");
}

static bool _sample_reset(SL_Source_t *source)
{
    Sample_t *sample = (Sample_t *)source;

    bool reset = _reset(sample);
    if (!reset) {
        LOG_E("can't reset sample %p data", source);
        return false;
    }

    return true;
}

static bool _sample_update(SL_Source_t *source, float delta_time)
{
    return true; // NO-OP
}

static bool _sample_generate(SL_Source_t *source, void *output, size_t frames_requested)
{
    Sample_t *sample = (Sample_t *)source;

    ma_data_converter *converter = &sample->props->converter;
    ma_audio_buffer *buffer = &sample->buffer;
    const bool looped = sample->props->looped;

    uint8_t *converted_buffer = sample->mixing_buffer;

    const SL_Mix_t mix = sample->props->precomputed_mix;

    uint8_t *cursor = (uint8_t *)output;

    size_t frames_remaining = frames_requested;
    while (frames_remaining > 0) {
        if (sample->frames_completed == sample->length_in_frames) {
            if (!looped || !_rewind(sample)) {
                LOG_D("end-of-data reached for source %p", source);
                return false;
            }
        }

        size_t frames_to_generate = frames_remaining > _MIXING_BUFFER_SIZE_IN_FRAMES ? _MIXING_BUFFER_SIZE_IN_FRAMES : frames_remaining;

        ma_uint64 frames_to_consume;
        ma_data_converter_get_required_input_frame_count(converter, frames_to_generate, &frames_to_consume);

        void *consumed_buffer;
        ma_audio_buffer_map(buffer, &consumed_buffer, &frames_to_consume); // No need to check the result, can't fail.

        ma_uint64 frames_consumed = frames_to_consume;
        ma_uint64 frames_generated = frames_to_generate;
        ma_data_converter_process_pcm_frames(converter, consumed_buffer, &frames_consumed, converted_buffer, &frames_generated);

        ma_audio_buffer_unmap(buffer, frames_consumed); // Ditto.

        sample->frames_completed += frames_consumed;

#if _MIXING_BUFFER_CHANNELS_PER_FRAME == 1
        mix_1on2_additive(cursor, converted_buffer, frames_generated, mix);
#elif _MIXING_BUFFER_CHANNELS_PER_FRAME == 2
        mix_2on2_additive(cursor, converted_buffer, frames_generated, mix);
#else
    #error "Mixing buffer has wrong number of channels"
#endif
        cursor += frames_generated * SL_BYTES_PER_FRAME;
        frames_remaining -= frames_generated;
    }

    return true;
}
