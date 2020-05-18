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

#include <libs/log.h>

#include <stdlib.h>
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

#define MIXING_BUFFER_SIZE_IN_FRAMES        128

#define LOG_CONTEXT "sl"

static inline size_t _consume(SL_Sample_t *sample, size_t frames_requested, void *buffer, size_t buffer_size_in_frames)
{
    size_t frames_processed = 0;

    uint8_t *cursor = buffer;

    size_t frames_remaining = (frames_requested > buffer_size_in_frames) ? buffer_size_in_frames : frames_requested;
    while (frames_remaining > 0) { // Read as much data as possible, filling the buffer and eventually looping!
        ma_uint32 frames_available = sample->frames_count - sample->current_frame;
        if (frames_available == 0) {
//            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "buffer underrun, %d bytes missing", frames_remaining);
            break;
        }

        ma_uint64 frames_to_convert = ma_data_converter_get_required_input_frame_count(&sample->converter, frames_remaining);

        ma_uint64 frames_consumed = (frames_to_convert > frames_available) ? frames_available : frames_to_convert;
        ma_uint64 frames_generated = frames_remaining;
        ma_data_converter_process_pcm_frames(&sample->converter, sample->frames + sample->current_frame, &frames_consumed, cursor, &frames_generated);

        cursor += frames_generated * SL_CHANNELS_PER_FRAME * SL_BYTES_PER_FRAME;

        frames_processed += frames_generated;
        frames_remaining -= frames_generated;

        sample->current_frame += frames_consumed;
        if (sample->current_frame == sample->frames_count) {
            if (sample->looped) {
                sample->current_frame = 0;
            } else {
                sample->state = SL_SAMPLE_STATE_COMPLETED;
            }
        }
    }

    return frames_processed;
}

// Each stream adds up in the output buffer, that's why we call it "additive mix".
// TODO: reuse this with the sample object.
static inline void *_additive_mix(SL_Sample_t *sample, void *output, void *input, size_t frames, const SL_Mix_t *groups)
{
    const float left = sample->mix.left * groups[sample->group].left; // Apply panning and gain to the data.
    const float right = sample->mix.right * groups[sample->group].right;

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

SL_Sample_t *SL_sample_create(SL_Sample_Read_Callback_t on_read, void *user_data, size_t length_in_frames, ma_format format, ma_uint32 sample_rate, ma_uint32 channels)
{
    // TODO: test if the length is zero?

    SL_Sample_t *sample = malloc(sizeof(SL_Sample_t));
    if (!sample) {
        return NULL;
    }

    size_t bytes_per_frame = ma_get_bytes_per_frame(format, channels);

    size_t size_in_bytes = length_in_frames * sample_rate * bytes_per_frame;
    void *frames = malloc(size_in_bytes);
    if (!frames) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate %d bytes for sample", size_in_bytes);
        free(sample);
        return NULL;
    }

    *sample = (SL_Sample_t){
            .frames = frames,
            .frames_count = length_in_frames,
            .bytes_per_frame = bytes_per_frame,
            .current_frame = 0,
            .group = SL_DEFAULT_GROUP,
            .looped = false,
            .gain = 1.0,
            .pan = 0.0f,
            .speed = 1.0f,
            .time = 0.0f,
            .state = SL_SAMPLE_STATE_STOPPED,
            .mix = _precompute_mix(0.0f, 1.0f)
        };

    size_t frames_read = on_read(user_data, frames, length_in_frames);
    if (frames_read != length_in_frames) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't read %d bytes for sample (%d available)", size_in_bytes, frames_read);
        free(frames);
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
        free(frames);
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
    sample->mix = _precompute_mix(sample->pan, sample->gain);
}

void SL_sample_pan(SL_Sample_t *sample, float pan)
{
    sample->pan = fmaxf(-1.0f, fminf(pan, 1.0f));
    sample->mix = _precompute_mix(sample->pan, sample->gain);
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

    sample->current_frame = 0;
}

void SL_sample_update(SL_Sample_t *sample, float delta_time)
{
}

void SL_sample_mix(SL_Sample_t *sample, void *output, size_t frames_requested, const SL_Mix_t *groups)
{
    if (sample->state == SL_SAMPLE_STATE_STOPPED) {
        return;
    }

    uint8_t buffer[MIXING_BUFFER_SIZE_IN_FRAMES * SL_CHANNELS_PER_FRAME * SL_BYTES_PER_FRAME];

    uint8_t *cursor = (uint8_t *)output;

    size_t frames_remaining = frames_requested;
    while (frames_remaining > 0) {
        size_t frames_processed = _consume(sample, frames_remaining, buffer, MIXING_BUFFER_SIZE_IN_FRAMES);
        cursor = _additive_mix(sample, cursor, buffer, frames_processed, groups);
        frames_remaining -= frames_processed;
    }
}
