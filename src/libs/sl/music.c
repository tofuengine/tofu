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

#include <config.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <math.h>

#ifndef M_PI
  #define M_PI      3.14159265358979323846f
#endif
#ifndef M_PI_2
  #define M_PI_2    1.57079632679489661923f
#endif

// Being the speed implemented by dynamic resampling, there's an intrinsic theoretical limit given by the ratio
// between the minimum (8KHz) and the maximum (384KHz) supported sample rates.
#define MIN_SPEED_VALUE ((float)MA_MIN_SAMPLE_RATE / (float)MA_MAX_SAMPLE_RATE)

// We are going to buffer 1 second of non-converted data. As long as the `SL_music_update()` function is called
// once half a second we are good. Since it's very unlikely we will run at less than 2 FPS... well, we can sleep well. :)
#define STREAMING_BUFFER_SIZE_IN_FRAMES     SL_FRAMES_PER_SECOND

#define MIXING_BUFFER_SIZE_IN_FRAMES        128

// We are using `miniaudio`'s ring-buffer, but we could also go for an in-house implementation
// https://embedjournal.com/implementing-circular-buffer-embedded-c/
// https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/

#define LOG_CONTEXT "sl"

static inline void _produce(SL_Music_t *music, bool reset)
{
    if (reset) {
        ma_pcm_rb_reset(&music->buffer);
    }

    ma_uint32 frames_to_write = ma_pcm_rb_available_write(&music->buffer);
//    ma_uint32 frames_to_write = BUFFER_SIZE_IN_FRAMES; // FIXME: move to update thread, with smaller size.
    while (frames_to_write > 0) {
        void *write_buffer;
        ma_pcm_rb_acquire_write(&music->buffer, &frames_to_write, &write_buffer);

        size_t frames_written = music->on_read(music->user_data, write_buffer, frames_to_write);

        ma_pcm_rb_commit_write(&music->buffer, frames_written, write_buffer);

        if (frames_written < frames_to_write) {
            if (!music->looped) {
                music->state = SL_MUSIC_STATE_COMPLETED;
                break;
            }
            music->on_seek(music->user_data, 0);
        }

        frames_to_write -= frames_written;
    }
}

static inline size_t _consume(SL_Music_t *music, size_t frames_requested, void *buffer, size_t buffer_size_in_frames)
{
    size_t frames_processed = 0;

    uint8_t *cursor = buffer;

    size_t frames_remaining = (frames_requested > buffer_size_in_frames) ? buffer_size_in_frames : frames_requested;
    while (frames_remaining > 0) { // Read as much data as possible, filling the buffer and eventually looping!
        ma_uint32 frames_available = ma_pcm_rb_available_read(&music->buffer);
        if (frames_available == 0) {
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "buffer underrun, %d bytes missing", frames_remaining);
            break;
        }

        ma_uint64 frames_to_convert = ma_data_converter_get_required_input_frame_count(&music->converter, frames_remaining);

        ma_uint32 frames_to_consume = (frames_to_convert > frames_available) ? frames_available : frames_to_convert;
        void *read_buffer;
        ma_pcm_rb_acquire_read(&music->buffer, &frames_to_consume, &read_buffer);

        ma_uint64 frames_consumed = frames_to_consume;
        ma_uint64 frames_generated = frames_remaining;
        ma_data_converter_process_pcm_frames(&music->converter, read_buffer, &frames_consumed, cursor, &frames_generated);

        ma_pcm_rb_commit_read(&music->buffer, frames_consumed, read_buffer);

        cursor += frames_generated * SL_CHANNELS_PER_FRAME * SL_BYTES_PER_FRAME;

        frames_processed += frames_generated;
        frames_remaining -= frames_generated;
    }

    return frames_processed;
}

// Each portion adds up in the output buffer, that's why we call it "additive mix".
// TODO: reuse this with the sample object.
static inline void *_additive_mix(SL_Music_t *music, void *output, void *input, size_t frames, const SL_Mix_t *groups)
{
    const float left = music->mix.left * groups[music->group].left; // Apply panning and gain to the data.
    const float right = music->mix.right * groups[music->group].right;

#if SL_BYTES_PER_FRAME == 2
    int16_t *sptr = input;
    int16_t *dptr = output;

    for (size_t i = 0; i < frames; ++i) {
        *(dptr++) += (int16_t)((float)*(sptr++) * left);
        *(dptr++) += (int16_t)((float)*(sptr++) * right);
    }
#elif SL_BYTES_PER_FRAME == 4
    float *sptr = input;
    float *dptr = output;

    for (size_t i = 0; i < frames; ++i) {
        *(dptr++) += *(sptr++) * left;
        *(dptr++) += *(sptr++) * right;
    }
#endif
    return dptr;
}

static inline SL_Mix_t _precompute_mix(float pan, float gain)
{
#if __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_GAIN
    const float theta = (pan + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    return (SL_Mix_t){ .left = (1.0f - theta) * gain, .right = theta * gain }; // powf(theta, 1)
#elif __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_POWER_SINCOS
    const float theta = (pan + 1.0f) * 0.5f * M_PI_2; // [-1, 1] -> [0 , 1] -> [0, pi/2]
    return (SL_Mix_t){ .left = cosf(theta) * gain, .right = sinf(theta) * gain };
#elif __SL_PANNING_LAW__ == PANNING_LAW_CONSTANT_POWER_SQRT
    const float theta = (pan + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    return (SL_Mix_t){ .left = sqrtf(1.0f - theta) * gain, .right = sqrtf(theta) * gain }; // powf(theta, 0.5)
#endif
}

SL_Music_t *SL_music_create(SL_Music_Read_Callback_t on_read, SL_Music_Seek_Callback_t on_seek, void *user_data, ma_format format, ma_uint32 sample_rate, ma_uint32 channels)
{
    SL_Music_t *music = malloc(sizeof(SL_Music_t));
    if (!music) {
        return NULL;
    }

    *music = (SL_Music_t){
            .on_read = on_read,
            .on_seek = on_seek,
            .user_data = user_data,
            .group = SL_DEFAULT_GROUP,
            .looped = false,
            .gain = 1.0,
            .pan = 0.0f,
            .speed = 1.0f,
            .time = 0.0f,
            .state = SL_MUSIC_STATE_STOPPED,
            .mix = _precompute_mix(0.0f, 1.0f)
        };

    ma_pcm_rb_init(format, channels, STREAMING_BUFFER_SIZE_IN_FRAMES, NULL, NULL, &music->buffer);

#if SL_BYTES_PER_FRAME == 2
    ma_data_converter_config config = ma_data_converter_config_init(format, ma_format_s16, channels, SL_CHANNELS_PER_FRAME, sample_rate, SL_FRAMES_PER_SECOND);
#elif SL_BYTES_PER_FRAME == 4
    ma_data_converter_config config = ma_data_converter_config_init(format, ma_format_f32, channels, SL_CHANNELS_PER_FRAME, sample_rate, SL_FRAMES_PER_SECOND);
#endif
    config.resampling.allowDynamicSampleRate = MA_TRUE; // required for speed throttling
    ma_result result = ma_data_converter_init(&config, &music->converter);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "failed to create music data converter");
        ma_pcm_rb_uninit(&music->buffer);
        free(music);
        return NULL;
    }

    _produce(music, true);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music created");
    return music;
}

void SL_music_destroy(SL_Music_t *music)
{
    if (!music) {
        return;
    }

    ma_data_converter_uninit(&music->converter);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music data converted uninitialized");

    ma_pcm_rb_uninit(&music->buffer);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music ring-buffer uninitialized");

    free(music);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music freed");
}

void SL_music_group(SL_Music_t *music, size_t group)
{
    music->group = group;
}

void SL_music_looped(SL_Music_t *music, bool looped)
{
    music->looped = looped;
}

void SL_music_gain(SL_Music_t *music, float gain)
{
    music->gain = fmaxf(0.0f, gain);
    music->mix = _precompute_mix(music->pan, music->gain);
}

void SL_music_pan(SL_Music_t *music, float pan)
{
    music->pan = fmaxf(-1.0f, fminf(pan, 1.0f));
    music->mix = _precompute_mix(music->pan, music->gain);
}

void SL_music_speed(SL_Music_t *music, float speed)
{
    music->speed = fmaxf(MIN_SPEED_VALUE, speed);
    ma_data_converter_set_rate_ratio(&music->converter, music->speed); // The ratio is `in` over `out`, i.e. actual speed-up factor.
}

void SL_music_play(SL_Music_t *music)
{
    music->state = SL_MUSIC_STATE_PLAYING;
}

void SL_music_stop(SL_Music_t *music)
{
    music->state = SL_MUSIC_STATE_STOPPED;
}

void SL_music_rewind(SL_Music_t *music)
{
    if (music->state != SL_MUSIC_STATE_STOPPED) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "can't rewind while playing");
        return;
    }

    music->on_seek(music->user_data, 0);
    _produce(music, true);
}

void SL_music_update(SL_Music_t *music, float delta_time)
{
    music->time += delta_time;

    if (music->state != SL_MUSIC_STATE_PLAYING) {
        return;
    }

    _produce(music, false);
}

void SL_music_mix(SL_Music_t *music, void *output, size_t frames_requested, const SL_Mix_t *groups)
{
    if (music->state == SL_MUSIC_STATE_STOPPED) {
        return;
    }

    uint8_t buffer[MIXING_BUFFER_SIZE_IN_FRAMES * SL_CHANNELS_PER_FRAME * SL_BYTES_PER_FRAME];

    uint8_t *cursor = (uint8_t *)output;

    size_t frames_remaining = frames_requested;
    while (frames_remaining > 0) {
        size_t frames_processed = _consume(music, frames_remaining, buffer, MIXING_BUFFER_SIZE_IN_FRAMES);
        cursor = _additive_mix(music, cursor, buffer, frames_processed, groups);
        frames_remaining -= frames_processed;
    }
}
