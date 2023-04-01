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

#include "music.h"

#include "internal.h"
#include "mix.h"

#include <core/config.h>
#include <libs/dr_libs.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <stdint.h>

// We are going to buffer 1 second of non-converted data. As long as the `SL_music_update()` function is called
// once half a second we are good. Since it's very unlikely we will run at less than 2 FPS... well, we can sleep well. :)
// FIXME: greater value to reduce the I/O? Guess this would be required...
#define STREAMING_BUFFER_SIZE_IN_FRAMES     SL_FRAMES_PER_SECOND

// That's the size of a single chunk read in each `produce()` call. Can't be larger than the buffer size.
#define STREAMING_BUFFER_CHUNK_IN_FRAMES    (STREAMING_BUFFER_SIZE_IN_FRAMES / 4)

#define MIXING_BUFFER_BYTES_PER_SAMPLE      SL_BYTES_PER_SAMPLE
#define MIXING_BUFFER_SAMPLES_PER_CHANNEL   SL_SAMPLES_PER_CHANNEL
#define MIXING_BUFFER_CHANNELS_PER_FRAME    SL_CHANNELS_PER_FRAME
#define MIXING_BUFFER_SIZE_IN_FRAMES        SL_MIXING_BUFFER_SIZE_IN_FRAMES

#define MIXING_BUFFER_BYTES_PER_FRAME       (MIXING_BUFFER_CHANNELS_PER_FRAME * MIXING_BUFFER_SAMPLES_PER_CHANNEL * MIXING_BUFFER_BYTES_PER_SAMPLE)
#define MIXING_BUFFER_SIZE_IN_BYTES         (MIXING_BUFFER_SIZE_IN_FRAMES * MIXING_BUFFER_BYTES_PER_FRAME)

#if MIXING_BUFFER_CHANNELS_PER_FRAME == 1
  #define mix_additive  mix_1on2_additive
#elif MIXING_BUFFER_CHANNELS_PER_FRAME == 2
  #define mix_additive  mix_2on2_additive
#else
  #error "Mixing buffer has wrong number of channels"
#endif

#define LOG_CONTEXT "sl-music"

typedef struct Music_s { // FIXME: rename to `_Music_Source_t`.
    Source_VTable_t vtable;

    SL_Props_t *props;

    SL_Callbacks_t callbacks;

    drflac *decoder;
    size_t length_in_frames;

    ma_pcm_rb buffer;
    size_t frames_completed;
} Music_t;

static bool _music_ctor(SL_Source_t *source, const SL_Context_t *context, SL_Callbacks_t callbacks);
static void _music_dtor(SL_Source_t *source);
static bool _music_reset(SL_Source_t *source);
static bool _music_update(SL_Source_t *source, float delta_time);
static bool _music_generate(SL_Source_t *source, void *output, size_t frames_requested);

static inline bool _rewind(Music_t *music)
{
    LOG_T(LOG_CONTEXT, "rewinding music %p", music);

    drflac_bool32 sought = drflac_seek_to_pcm_frame(music->decoder, 0);
    if (!sought) {
        LOG_E(LOG_CONTEXT, "can't rewind music stream");
        return false;
    }

    music->frames_completed = 0;

    return true;
}

static inline bool _reset(Music_t *music)
{
    LOG_T(LOG_CONTEXT, "rewinding music %p", music);

    ma_pcm_rb *buffer = &music->buffer;
    ma_pcm_rb_reset(buffer);

    return _rewind(music);
}

static inline bool _produce(Music_t *music)
{
    if (music->frames_completed == music->length_in_frames) { // End-of-data, early exit.
        if (!music->props->looped || !_rewind(music)) {
            LOG_D(LOG_CONTEXT, "end-of-data, early exit music %p", music);
            return false;
        }
    }

    ma_pcm_rb *buffer = &music->buffer;
    ma_uint32 frames_to_produce = ma_pcm_rb_available_write(buffer);
    if (frames_to_produce == 0) {
        LOG_W(LOG_CONTEXT, "buffer overrrun for source %p - stalling (waiting for consumer)", music);
        return true;
#if defined(STREAMING_BUFFER_CHUNK_IN_FRAMES)
    } else
    if (frames_to_produce > STREAMING_BUFFER_CHUNK_IN_FRAMES) {
        frames_to_produce = STREAMING_BUFFER_CHUNK_IN_FRAMES;
#endif
    }

    void *write_buffer;
    ma_pcm_rb_acquire_write(buffer, &frames_to_produce, &write_buffer);

#if SL_BYTES_PER_SAMPLE == 2
    size_t frames_produced = drflac_read_pcm_frames_s16(music->decoder, frames_to_produce, write_buffer);
#elif SL_BYTES_PER_SAMPLE == 4
    size_t frames_produced = drflac_read_pcm_frames_f32(music->decoder, frames_to_produce, write_buffer);
#endif

    ma_pcm_rb_commit_write(buffer, frames_produced);

    music->frames_completed += frames_produced;

    if (frames_produced < frames_to_produce && music->frames_completed < music->length_in_frames) { // Check if an error occurred (no more data w/ no EOF)
        LOG_E(LOG_CONTEXT, "can't read %d bytes (%d read)", frames_to_produce, frames_produced);
        return false;
    }

    // If only part of the `frames_to_produce` chunk has been fetched due to end-of-file we are ok; the
    // stream will be looped (if needed) and the rest loaded on the next update call.

    return true;
}

SL_Source_t *SL_music_create(const SL_Context_t *context, SL_Callbacks_t callbacks)
{
    SL_Source_t *music = malloc(sizeof(Music_t));
    if (!music) {
        LOG_E(LOG_CONTEXT, "can't allocate music structure");
        return NULL;
    }

    bool cted = _music_ctor(music, context, callbacks);
    if (!cted) {
        LOG_E(LOG_CONTEXT, "can't initialize music structure");
        free(music);
        return NULL;
    }

    LOG_D(LOG_CONTEXT, "music %p created", music);
    return music;
}

static size_t _music_read(void *user_data, void *buffer, size_t bytes_to_read)
{
    Music_t *music = (Music_t *)user_data;
    const SL_Callbacks_t *callbacks = &music->callbacks;

    return callbacks->read(callbacks->user_data, buffer, bytes_to_read);
}

static drflac_bool32 _music_seek(void *user_data, int offset, drflac_seek_origin origin)
{
    Music_t *music = (Music_t *)user_data;
    const SL_Callbacks_t *callbacks = &music->callbacks;

    bool sought = false;
    if (origin == drflac_seek_origin_start) {
        sought = callbacks->seek(callbacks->user_data, offset, SEEK_SET);
    } else
    if (origin == drflac_seek_origin_current) {
        sought = callbacks->seek(callbacks->user_data, offset, SEEK_CUR);
    }
    return sought ? DRFLAC_TRUE : DRFLAC_FALSE;
}

static bool _music_ctor(SL_Source_t *source, const SL_Context_t *context, SL_Callbacks_t callbacks)
{
    Music_t *music = (Music_t *)source;

    *music = (Music_t){
            .vtable = (Source_VTable_t){
                    .dtor = _music_dtor,
                    .reset = _music_reset,
                    .update = _music_update,
                    .generate = _music_generate
                },
            .callbacks = callbacks,
            .frames_completed = 0
        };

    music->decoder = drflac_open(_music_read, _music_seek, music, NULL);
    if (!music->decoder) {
        LOG_E(LOG_CONTEXT, "can't create music decoder");
        return false;
    }

    music->length_in_frames = music->decoder->totalPCMFrameCount;
    if (music->length_in_frames == 0) {
        LOG_E(LOG_CONTEXT, "can't create music w/ zero length");
        goto error_close_decoder;
    }

    size_t channels = music->decoder->channels;
    size_t sample_rate = music->decoder->sampleRate;
    size_t bits_per_sample = music->decoder->bitsPerSample;
    LOG_D(LOG_CONTEXT, "music decoder %p initialized w/ %d frames, %d channels, %dHz, %d bits", music->decoder, music->length_in_frames, channels, sample_rate, bits_per_sample);

    ma_result result = ma_pcm_rb_init(INTERNAL_FORMAT, channels, STREAMING_BUFFER_SIZE_IN_FRAMES, NULL, NULL, &music->buffer);
    if (result != MA_SUCCESS) {
        LOG_E(LOG_CONTEXT, "can't initialize music ring-buffer (%d frames)", STREAMING_BUFFER_SIZE_IN_FRAMES);
        goto error_close_decoder;
    }

#if defined(__SL_MUSIC_PRELOAD__)
    bool produced = _produce(music);
    if (!produced) {
        LOG_E(LOG_CONTEXT, "can't pre-load music data");
        ma_pcm_rb_uninit(&music->buffer);
        drflac_close(music->decoder);
        return false;
    }
#endif

    music->props = SL_props_create(context, INTERNAL_FORMAT, sample_rate, channels, MIXING_BUFFER_CHANNELS_PER_FRAME);
    if (!music->props) {
        LOG_E(LOG_CONTEXT, "can't initialize music properties");
        goto error_deinitialize_ringbuffer;
    }

    return true;

error_deinitialize_ringbuffer:
    ma_pcm_rb_uninit(&music->buffer);
error_close_decoder:
    drflac_close(music->decoder);
    return false;
}

static void _music_dtor(SL_Source_t *source)
{
    Music_t *music = (Music_t *)source;

    SL_props_destroy(music->props);
    LOG_D(LOG_CONTEXT, "music properties destroyed");

    ma_pcm_rb_uninit(&music->buffer);
    LOG_D(LOG_CONTEXT, "music ring-buffer uninitialized");

    drflac_close(music->decoder);
    LOG_D(LOG_CONTEXT, "music decoder closed");
}

static bool _music_reset(SL_Source_t *source)
{
    Music_t *music = (Music_t *)source;

    bool reset = _reset(music);
    if (!reset) {
        LOG_E(LOG_CONTEXT, "can't reset music %p stream", source);
        return false;
    }

#if defined(__SL_MUSIC_PRELOAD__)
    bool produced = _produce(music);
    if (!produced) {
        LOG_E(LOG_CONTEXT, "can't pre-load music data");
        return false;
    }
#else
    return true;
#endif
}

static bool _music_update(SL_Source_t *source, float delta_time)
{
    Music_t *music = (Music_t *)source;

    return _produce(music);
}

static bool _music_generate(SL_Source_t *source, void *output, size_t frames_requested)
{
    Music_t *music = (Music_t *)source;

    ma_data_converter *converter = &music->props->converter;
    ma_pcm_rb *buffer = &music->buffer;

    uint8_t converted_buffer[MIXING_BUFFER_SIZE_IN_BYTES];

    const SL_Mix_t mix = music->props->precomputed_mix;

    uint8_t *cursor = (uint8_t *)output;

    size_t frames_remaining = frames_requested;
    while (frames_remaining > 0) {
        ma_uint32 frames_available = ma_pcm_rb_available_read(buffer);
        if (frames_available == 0) {
            if (music->frames_completed < music->length_in_frames) {
                LOG_W(LOG_CONTEXT, "buffer underrun for source %p - stalling (waiting for data)", source);
                return true;
            } else {
                LOG_D(LOG_CONTEXT, "end-of-data reached for source %p", source);
                return false;
            }
        }

        size_t frames_to_generate = frames_remaining > MIXING_BUFFER_SIZE_IN_FRAMES ? MIXING_BUFFER_SIZE_IN_FRAMES : frames_remaining;

        ma_uint64 frames_to_consume;
        ma_data_converter_get_required_input_frame_count(converter, frames_to_generate, &frames_to_consume);

        ma_uint32 frames_to_acquire = (ma_uint32)frames_to_consume;
        void *consumed_buffer;
        ma_pcm_rb_acquire_read(buffer, &frames_to_acquire, &consumed_buffer);

        ma_uint64 frames_consumed = frames_to_acquire;
        ma_uint64 frames_generated = frames_to_generate;
        ma_data_converter_process_pcm_frames(converter, consumed_buffer, &frames_consumed, converted_buffer, &frames_generated);

        ma_pcm_rb_commit_read(buffer, frames_consumed);

        mix_additive(cursor, converted_buffer, frames_generated, mix);
        cursor += frames_generated * SL_BYTES_PER_FRAME;
        frames_remaining -= frames_generated;
    }

    return true;
}
