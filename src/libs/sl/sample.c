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

#include "mix.h"

#include <libs/log.h>
#include <libs/stb.h>

#include <math.h>

// Being the speed implemented by dynamic resampling, there's an intrinsic theoretical limit given by the ratio
// between the minimum (8KHz) and the maximum (384KHz) supported sample rates.
#define MIN_SPEED_VALUE ((float)MA_MIN_SAMPLE_RATE / (float)MA_MAX_SAMPLE_RATE)

#define MIXING_BUFFER_SIZE_IN_FRAMES    128

#define LOG_CONTEXT "sl-sample"

static inline size_t _consume(SL_Sample_t *sample, size_t frames_requested, void *buffer, size_t buffer_size_in_frames)
{
    size_t frames_processed = 0;

    uint8_t *cursor = buffer;

    size_t frames_remaining = (frames_requested > buffer_size_in_frames) ? buffer_size_in_frames : frames_requested;
    while (frames_remaining > 0) { // Read as much data as possible, filling the buffer and eventually looping!
        ma_uint64 frames_to_convert = ma_data_converter_get_required_input_frame_count(&sample->converter, frames_remaining);

        size_t frames_available = frames_to_convert;
        void *read_buffer = buffer_lock(&sample->buffer, &frames_available);

        ma_uint64 frames_consumed = frames_available;
        ma_uint64 frames_generated = frames_remaining;
        ma_data_converter_process_pcm_frames(&sample->converter, read_buffer, &frames_consumed, cursor, &frames_generated);

        buffer_unlock(&sample->buffer, read_buffer, frames_consumed);

        cursor += frames_generated * SL_CHANNELS_PER_FRAME * SL_BYTES_PER_FRAME;

        frames_processed += frames_generated;
        frames_remaining -= frames_generated;

        if (frames_available == 0) {
            if (sample->looped) {
                buffer_reset(&sample->buffer);
            } else {
                sample->state = SL_SAMPLE_STATE_STOPPED;
                break;
            }
        }
    }

    return frames_processed;
}

SL_Sample_t *SL_sample_create(SL_Sample_Read_Callback_t on_read, void *user_data, size_t length_in_frames, ma_format format, ma_uint32 sample_rate, ma_uint32 channels)
{
    // TODO: test if the length is zero?

    SL_Sample_t *sample = malloc(sizeof(SL_Sample_t));
    if (!sample) {
        return NULL;
    }

    *sample = (SL_Sample_t){
            .group = SL_DEFAULT_GROUP,
            .looped = false,
            .gain = 1.0,
            .pan = 0.0f,
            .speed = 1.0f,
            .time = 0.0f,
            .state = SL_SAMPLE_STATE_STOPPED,
            .mix = mix_precompute(0.0f, 1.0f)
        };

    size_t bytes_per_frame = ma_get_bytes_per_frame(format, channels);

    bool initialized = buffer_init(&sample->buffer, length_in_frames, bytes_per_frame);
    if (!initialized) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d bytes for buffer", length_in_frames * bytes_per_frame);
        free(sample);
        return NULL;
    }

    size_t frames_read = on_read(user_data, sample->buffer.frames, length_in_frames);
    if (frames_read != length_in_frames) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read %d frames for sample (%d available)", length_in_frames, frames_read);
        buffer_uninit(&sample->buffer);
        free(sample);
        return NULL;
    }

#if SL_BYTES_PER_FRAME == 2
    ma_data_converter_config config = ma_data_converter_config_init(format, ma_format_s16, channels, SL_CHANNELS_PER_FRAME, sample_rate, SL_FRAMES_PER_SECOND);
#elif SL_BYTES_PER_FRAME == 4
    ma_data_converter_config config = ma_data_converter_config_init(format, ma_format_f32, channels, SL_CHANNELS_PER_FRAME, sample_rate, SL_FRAMES_PER_SECOND);
#endif
    config.resampling.allowDynamicSampleRate = MA_TRUE; // required for speed throttling
    ma_result result = ma_data_converter_init(&config, &sample->converter);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "failed to create sample data converter");
        buffer_uninit(&sample->buffer);
        free(sample);
        return NULL;
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sample created");
    return sample;
}

void SL_sample_destroy(SL_Sample_t *sample)
{
    if (!sample) {
        return;
    }

    ma_data_converter_uninit(&sample->converter);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "stream data converted uninitialized");

    free(sample);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "stream freed");
}

void SL_sample_group(SL_Sample_t *sample, size_t group)
{
    sample->group = group;
}

void SL_sample_looped(SL_Sample_t *sample, bool looped)
{
    sample->looped = looped;
}

void SL_sample_gain(SL_Sample_t *sample, float gain)
{
    sample->gain = fmaxf(0.0f, gain);
    sample->mix = mix_precompute(sample->pan, sample->gain);
}

void SL_sample_pan(SL_Sample_t *sample, float pan)
{
    sample->pan = fmaxf(-1.0f, fminf(pan, 1.0f));
    sample->mix = mix_precompute(sample->pan, sample->gain);
}

void SL_sample_speed(SL_Sample_t *sample, float speed)
{
    sample->speed = fmaxf(MIN_SPEED_VALUE, speed);
    ma_data_converter_set_rate_ratio(&sample->converter, sample->speed); // The ratio is `in` over `out`, i.e. actual speed-up factor.
}

void SL_sample_play(SL_Sample_t *sample)
{
    sample->state = SL_SAMPLE_STATE_PLAYING;
}

void SL_sample_stop(SL_Sample_t *sample)
{
    sample->state = SL_SAMPLE_STATE_STOPPED;
}

void SL_sample_rewind(SL_Sample_t *sample)
{
    if (sample->state != SL_SAMPLE_STATE_STOPPED) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "can't rewind while playing");
        return;
    }

    buffer_reset(&sample->buffer);
}

void SL_sample_update(SL_Sample_t *sample, float delta_time)
{
    sample->time += delta_time;
}

void SL_sample_mix(SL_Sample_t *sample, void *output, size_t frames_requested, const SL_Mix_t *groups)
{
    if (sample->state == SL_SAMPLE_STATE_STOPPED) {
        return;
    }

    uint8_t buffer[MIXING_BUFFER_SIZE_IN_FRAMES * SL_CHANNELS_PER_FRAME * SL_BYTES_PER_FRAME];

    const SL_Mix_t mix = (SL_Mix_t){
            .left = sample->mix.left * groups[sample->group].left,
            .right = sample->mix.right * groups[sample->group].right
        };

    uint8_t *cursor = (uint8_t *)output;

    size_t frames_remaining = frames_requested;
    while (frames_remaining > 0 && sample->state != SL_SAMPLE_STATE_STOPPED) { // State can change during the loop.
        size_t frames_processed = _consume(sample, frames_remaining, buffer, MIXING_BUFFER_SIZE_IN_FRAMES);
        cursor = mix_additive(cursor, buffer, frames_processed, mix);
        frames_remaining -= frames_processed;
    }
}
