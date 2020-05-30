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
#define STREAMING_BUFFER_CHUNK_IN_FRAMES    (SL_FRAMES_PER_SECOND / 4)

#define MIXING_BUFFER_SIZE_IN_FRAMES    128

#define LOG_CONTEXT "sl-music"

typedef enum _Music_States_t {
    MUSIC_STATE_STOPPED,
    MUSIC_STATE_PLAYING,
    MUSIC_STATE_FINISHING,
    Music_States_t_CountOf
} Music_States_t;

typedef struct _Music_t {
    Source_VTable_t vtable;
    SL_Props_t props;

    SL_Read_Callback_t on_read;
    SL_Seek_Callback_t on_seek;
    void *user_data;

    ma_pcm_rb buffer;

    double time; // ???
    volatile Music_States_t state;
} Music_t;

static void _music_dtor(SL_Source_t *source);
static void _music_play(SL_Source_t *source);
static void _music_stop(SL_Source_t *source);
static void _music_rewind(SL_Source_t *source);
static bool _music_is_playing(SL_Source_t *source);
static void _music_update(SL_Source_t *source, float delta_time);
static void _music_mix(SL_Source_t *source, void *output, size_t frames_requested, const SL_Mix_t *groups);

static inline void _produce(Music_t *music, bool reset)
{
    ma_pcm_rb *buffer = &music->buffer;

    if (reset) {
        ma_pcm_rb_reset(buffer);
        music->on_seek(music->user_data, 0);
    }

    ma_uint32 frames_to_write = ma_pcm_rb_available_write(buffer);
#ifdef STREAMING_BUFFER_CHUNK_IN_FRAMES
    if (frames_to_write > STREAMING_BUFFER_CHUNK_IN_FRAMES) {
        frames_to_write = STREAMING_BUFFER_CHUNK_IN_FRAMES;
    }
#endif
    while (frames_to_write > 0) {
        void *write_buffer;
        ma_pcm_rb_acquire_write(buffer, &frames_to_write, &write_buffer);

        size_t frames_written = music->on_read(music->user_data, write_buffer, frames_to_write);

        ma_pcm_rb_commit_write(buffer, frames_written, write_buffer);

        if (frames_written < frames_to_write) {
            if (!music->props.looping) {
                music->state = MUSIC_STATE_FINISHING;
                break;
            }
            music->on_seek(music->user_data, 0);
        }

        frames_to_write -= frames_written;
    }
}

static inline size_t _consume(Music_t *music, size_t frames_requested, void *output, size_t size_in_frames)
{
    ma_data_converter *converter = &music->props.converter;
    ma_pcm_rb *buffer = &music->buffer;

    size_t frames_processed = 0;

    uint8_t *cursor = output;

    size_t frames_remaining = (frames_requested > size_in_frames) ? size_in_frames : frames_requested;
    while (frames_remaining > 0) { // Read as much data as possible, filling the buffer and eventually looping!
        ma_uint32 frames_available = ma_pcm_rb_available_read(buffer);
        if (frames_available == 0) {
            if (music->state == MUSIC_STATE_FINISHING) {
                music->state = MUSIC_STATE_STOPPED;
            } else {
                Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "buffer underrun, %d bytes missing (state #%d)", frames_remaining, music->state);
            }
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

        cursor += frames_generated * SL_CHANNELS_PER_FRAME * SL_BYTES_PER_FRAME;

        frames_processed += frames_generated;
        frames_remaining -= frames_generated;
    }

    return frames_processed;
}

SL_Source_t *SL_music_create(SL_Read_Callback_t on_read, SL_Seek_Callback_t on_seek, void *user_data, ma_format format, ma_uint32 sample_rate, ma_uint32 channels)
{
    Music_t *music = malloc(sizeof(Music_t));
    if (!music) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate music structure");
        return NULL;
    }

    *music = (Music_t){
            .vtable = (Source_VTable_t){
                    .dtor = _music_dtor,
                    .play = _music_play,
                    .stop = _music_stop,
                    .rewind = _music_rewind,
                    .is_playing = _music_is_playing,
                    .update = _music_update,
                    .mix = _music_mix
                },
            .on_read = on_read,
            .on_seek = on_seek,
            .user_data = user_data,
            .time = 0.0f,
            .state = MUSIC_STATE_STOPPED
        };

    ma_result result = ma_pcm_rb_init(format, channels, STREAMING_BUFFER_SIZE_IN_FRAMES, NULL, NULL, &music->buffer);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize music ring-buffer");
        free(music);
        return NULL;
    }

    bool initialized = SL_props_init(&music->props, format, sample_rate, channels);
    if (!initialized) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialize music properties");
        ma_pcm_rb_uninit(&music->buffer);
        free(music);
        return NULL;
    }

#ifdef __SL_MUSIC_PRELOAD_ON_CREATION__
    _produce(music, true);
#endif

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music created");
    return music;
}

static void _music_dtor(SL_Source_t *source)
{
    Music_t *music = (Music_t *)source;

    if (!music) {
        return;
    }

    SL_props_deinit(&music->props);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music properties deinitialized");

    ma_pcm_rb_uninit(&music->buffer);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music ring-buffer deinitialized");

    free(music);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music structure freed");
}

static void _music_play(SL_Source_t *source)
{
    Music_t *music = (Music_t *)source;

    music->state = MUSIC_STATE_PLAYING;
}

static void _music_stop(SL_Source_t *source)
{
    Music_t *music = (Music_t *)source;

    music->state = MUSIC_STATE_STOPPED;
}

static void _music_rewind(SL_Source_t *source)
{
    Music_t *music = (Music_t *)source;

    if (music->state != MUSIC_STATE_STOPPED) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "can't rewind while playing");
        return;
    }

    _produce(music, true);
}

static bool _music_is_playing(SL_Source_t *source)
{
    Music_t *music = (Music_t *)source;

    return music->state != MUSIC_STATE_STOPPED;
}

static void _music_update(SL_Source_t *source, float delta_time)
{
    Music_t *music = (Music_t *)source;

    music->time += delta_time;

    if (music->state != MUSIC_STATE_PLAYING) {
        return;
    }

    _produce(music, false);
}

static void _music_mix(SL_Source_t *source, void *output, size_t frames_requested, const SL_Mix_t *groups)
{
    Music_t *music = (Music_t *)source;

    if (music->state == MUSIC_STATE_STOPPED) {
        return;
    }

    uint8_t buffer[MIXING_BUFFER_SIZE_IN_FRAMES * SL_CHANNELS_PER_FRAME * SL_BYTES_PER_FRAME];

    const SL_Mix_t mix = SL_props_precompute(&music->props, groups);

    uint8_t *cursor = (uint8_t *)output;

    size_t frames_remaining = frames_requested;
    while (frames_remaining > 0 && music->state != MUSIC_STATE_STOPPED) { // State can change during the loop.
        size_t frames_processed = _consume(music, frames_remaining, buffer, MIXING_BUFFER_SIZE_IN_FRAMES);
        mix_additive(cursor, buffer, frames_processed, mix);
        cursor += frames_processed * SL_CHANNELS_PER_FRAME * SL_BYTES_PER_FRAME;
        frames_remaining -= frames_processed;
    }
}
