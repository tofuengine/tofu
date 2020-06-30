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

// We are going to buffer 1 second of non-converted data. As long as the `SL_music_update()` function is called
// once half a second we are good. Since it's very unlikely we will run at less than 2 FPS... well, we can sleep well. :)
#define STREAMING_BUFFER_SIZE_IN_FRAMES     SL_FRAMES_PER_SECOND

// That's the size of a single chunk read in each `produce()` call. Can't be larger than the buffer size.
#define STREAMING_BUFFER_CHUNK_IN_FRAMES    (STREAMING_BUFFER_SIZE_IN_FRAMES / 4)

#define MIXING_BUFFER_CHANNELS          SL_CHANNELS_PER_FRAME
#define MIXING_BUFFER_SIZE_IN_FRAMES    128

#define LOG_CONTEXT "sl-music"

typedef struct _Music_t { // FIXME: rename to `_Music_Source_t`.
    Source_VTable_t vtable;

    SL_Props_t props;

    SL_Callbacks_t callbacks;
    void *user_data;

    size_t length_in_frames;

    ma_pcm_rb buffer;
    size_t frames_completed;
} Music_t;

static bool _music_ctor(SL_Source_t *source, SL_Callbacks_t callbacks, void *user_data, size_t length_in_frames, ma_format format, ma_uint32 sample_rate, ma_uint32 channels);
static void _music_dtor(SL_Source_t *source);
static bool _music_reset(SL_Source_t *source);
static bool _music_update(SL_Source_t *source, float delta_time);
static bool _music_mix(SL_Source_t *source, void *output, size_t frames_requested, const SL_Mix_t *groups);

static inline bool _rewind(Music_t *music)
{
    const SL_Callbacks_t *callbacks = &music->callbacks;

    bool seeked = callbacks->seek(music->user_data, 0);
    if (!seeked) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't rewind music stream");
        return false;
    }

    music->frames_completed = 0;

    return true;
}

static inline bool _reset(Music_t *music)
{
    ma_pcm_rb *buffer = &music->buffer;

    ma_pcm_rb_reset(buffer);

    return _rewind(music);
}

static inline bool _produce(Music_t *music)
{
    const SL_Callbacks_t *callbacks = &music->callbacks;
    ma_pcm_rb *buffer = &music->buffer;

    if (music->frames_completed == music->length_in_frames) { // End-of-data, early exit.
        if (!music->props.looping) { // FIXME: duplicated.
            return true;
        }
        bool rewound = _rewind(music);
        if (!rewound) {
            return false;
        }
    }

    ma_uint32 frames_to_produce = ma_pcm_rb_available_write(buffer);
#ifdef STREAMING_BUFFER_CHUNK_IN_FRAMES
    if (frames_to_produce > STREAMING_BUFFER_CHUNK_IN_FRAMES) {
        frames_to_produce = STREAMING_BUFFER_CHUNK_IN_FRAMES;
    }
#endif

    void *write_buffer;
    ma_pcm_rb_acquire_write(buffer, &frames_to_produce, &write_buffer);

    size_t frames_produced = callbacks->read(music->user_data, write_buffer, frames_to_produce);

    ma_pcm_rb_commit_write(buffer, frames_produced, write_buffer);

    music->frames_completed += frames_produced;

    if (frames_produced < frames_to_produce && music->frames_completed < music->length_in_frames) { // Check if an error occurred (no more data w/ no EOF)
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read %d bytes (%d read)", frames_to_produce, frames_produced);
        return false;
    }

    // If only part of the `frames_to_produce` chunk has been fetched due to end-of-file we are ok; the
    // stream will be looped (if needed) and the rest loaded on the next update call.

    return true;
}

// https://english.stackexchange.com/questions/457305/the-difference-between-state-and-status
typedef enum _States_t {
    STATE_STREAMING,
    STATE_STALLING,
    STATE_EOD,
} States_t;

static inline size_t _consume(Music_t *music, size_t frames_requested, void *output, size_t size_in_frames, States_t *state)
{
    ma_data_converter *converter = &music->props.converter;
    ma_pcm_rb *buffer = &music->buffer;

    *state = STATE_STREAMING;

    size_t frames_processed = 0;

    uint8_t *cursor = output;

    size_t frames_remaining = (frames_requested > size_in_frames) ? size_in_frames : frames_requested;
    while (frames_remaining > 0) { // Read as much data as possible, filling the buffer and eventually looping!
        ma_uint32 frames_available = ma_pcm_rb_available_read(buffer);
        if (frames_available == 0) {
            *state = music->frames_completed < music->length_in_frames ? STATE_STALLING : STATE_EOD;
            break;
        }

        ma_uint64 frames_to_convert = ma_data_converter_get_required_input_frame_count(converter, frames_remaining);

        ma_uint32 frames_to_consume = (frames_to_convert > frames_available) ? frames_available : frames_to_convert;
        void *read_buffer;
        ma_pcm_rb_acquire_read(buffer, &frames_to_consume, &read_buffer);

        ma_uint64 frames_consumed = frames_to_consume;
        ma_uint64 frames_generated = frames_remaining;
        ma_data_converter_process_pcm_frames(converter, read_buffer, &frames_consumed, cursor, &frames_generated);

        ma_pcm_rb_commit_read(buffer, frames_consumed, read_buffer);

        cursor += frames_generated * MIXING_BUFFER_CHANNELS * SL_BYTES_PER_FRAME;

        frames_processed += frames_generated;
        frames_remaining -= frames_generated;
    }

    return frames_processed;
}

SL_Source_t *SL_music_create(SL_Callbacks_t callbacks, void *user_data, size_t length_in_frames, ma_format format, ma_uint32 sample_rate, ma_uint32 channels)
{
    Music_t *music = malloc(sizeof(Music_t));
    if (!music) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate music structure");
        return NULL;
    }

    bool cted = _music_ctor(music, callbacks, user_data, length_in_frames, format, sample_rate, channels);
    if (!cted) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize music structure");
        free(music);
        return NULL;
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music %p created", music);
    return music;
}

static bool _music_ctor(SL_Source_t *source, SL_Callbacks_t callbacks, void *user_data, size_t length_in_frames, ma_format format, ma_uint32 sample_rate, ma_uint32 channels)
{
    Music_t *music = (Music_t *)source;

    *music = (Music_t){
            .vtable = (Source_VTable_t){
                    .dtor = _music_dtor,
                    .reset = _music_reset,
                    .update = _music_update,
                    .mix = _music_mix
                },
            .callbacks = callbacks,
            .user_data = user_data,
            .length_in_frames = length_in_frames,
            .frames_completed = 0
        };

    ma_result result = ma_pcm_rb_init(format, channels, STREAMING_BUFFER_SIZE_IN_FRAMES, NULL, NULL, &music->buffer);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize music ring-buffer (%d frames)", STREAMING_BUFFER_SIZE_IN_FRAMES);
        return false;
    }

#ifdef __SL_MUSIC_PRELOAD__
    bool produces = _produce(music);
    if (!produced) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't pre-load music data");
        return false;
    }
#endif

    bool initialized = SL_props_init(&music->props, format, sample_rate, channels, MIXING_BUFFER_CHANNELS);
    if (!initialized) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize music properties");
        ma_pcm_rb_uninit(&music->buffer);
        return false;
    }

    return true;
}

static void _music_dtor(SL_Source_t *source)
{
    Music_t *music = (Music_t *)source;

    SL_props_deinit(&music->props);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music properties deinitialized");

    ma_pcm_rb_uninit(&music->buffer);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music ring-buffer deinitialized");
}

static bool _music_reset(SL_Source_t *source)
{
    Music_t *music = (Music_t *)source;

    bool reset = _reset(music);
    if (!reset) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't reset music stream");
        return false;
    }

#ifdef __SL_MUSIC_PRELOAD__
    bool produced = _produce(music);
    if (!produced) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't pre-load music data");
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

static bool _music_mix(SL_Source_t *source, void *output, size_t frames_requested, const SL_Mix_t *groups)
{
    Music_t *music = (Music_t *)source;

    uint8_t buffer[MIXING_BUFFER_SIZE_IN_FRAMES * MIXING_BUFFER_CHANNELS * SL_BYTES_PER_FRAME];

    const SL_Mix_t mix = SL_props_precompute(&music->props, groups);

    uint8_t *cursor = (uint8_t *)output;

    size_t frames_remaining = frames_requested;
    while (frames_remaining > 0) {
        States_t state = STATE_STREAMING;
        size_t frames_processed = _consume(music, frames_remaining, buffer, MIXING_BUFFER_SIZE_IN_FRAMES, &state);
        mix_2on2_additive(cursor, buffer, frames_processed, mix);
        if (state == STATE_STALLING) { // TODO: detect buffer underrun and stall!
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "buffer underrun for source %p - stalling", source);
            return false;
        } else
        if (state == STATE_EOD) {
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "end-of-data reached for source %p", source);
            return true;
        }
        cursor += frames_processed * MIXING_BUFFER_CHANNELS * SL_BYTES_PER_FRAME;
        frames_remaining -= frames_processed;
    }
    return false;
}
