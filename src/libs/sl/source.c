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

#include "source.h"

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

#define LOG_CONTEXT "sl"

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

SL_Source_t *SL_source_create(SL_Source_Read_Callback_t on_read, SL_Source_Seek_Callback_t on_seek, void *user_data, ma_format format, ma_uint32 sample_rate, ma_uint32 channels)
{
    SL_Source_t *source = malloc(sizeof(SL_Source_t));
    if (!source) {
        return NULL;
    }

    *source = (SL_Source_t){
            .on_read = on_read,
            .on_seek = on_seek,
            .user_data = user_data,
            .group = SL_DEFAULT_GROUP,
            .looped = false,
            .gain = 1.0,
            .pan = 0.0f,
            .speed = 1.0f,
            .time = 0.0f,
            .state = SL_SOURCE_STATE_STOPPED,
            .mix = _precompute_mix(0.0f, 1.0f)
        };

    ma_data_converter_config config = ma_data_converter_config_init(format, ma_format_f32, channels, SL_CHANNELS_PER_FRAME, sample_rate, SL_FRAMES_PER_SECOND);
    config.resampling.allowDynamicSampleRate = MA_TRUE; // required for speed throttling
    ma_result result = ma_data_converter_init(&config, &source->converter);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "failed to create source data conversion");
        free(source);
        return NULL;
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source created");
    return source;
}

void SL_source_destroy(SL_Source_t *source)
{
    if (!source) {
        return;
    }

    free(source);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source freed");
}

void SL_source_group(SL_Source_t *source, size_t group)
{
    source->group = group;
}

void SL_source_looped(SL_Source_t *source, bool looped)
{
    source->looped = looped;
}

void SL_source_gain(SL_Source_t *source, float gain)
{
    source->gain = fmaxf(0.0f, gain);
    source->mix = _precompute_mix(source->pan, source->gain);
}

void SL_source_pan(SL_Source_t *source, float pan)
{
    source->pan = fmaxf(-1.0f, fminf(pan, 1.0f));
    source->mix = _precompute_mix(source->pan, source->gain);
}

void SL_source_speed(SL_Source_t *source, float speed)
{
    source->speed = fmaxf(MIN_SPEED_VALUE, speed);
    ma_data_converter_set_rate_ratio(&source->converter, source->speed); // The ratio is `in` over `out`, i.e. actual speed-up factor.
}

void SL_source_play(SL_Source_t *source)
{
    source->state = SL_SOURCE_STATE_PLAYING;
}

void SL_source_stop(SL_Source_t *source)
{
    source->state = SL_SOURCE_STATE_STOPPED;
}

void SL_source_rewind(SL_Source_t *source)
{
    if (source->state != SL_SOURCE_STATE_STOPPED) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "can't rewind while playing");
        return;
    }
    source->on_seek(source->user_data, 0);
}

void SL_source_update(SL_Source_t *source, float delta_time)
{
    // FIXME: useless? perhaps useful in the future for some kind of time-related effect?
    source->time += delta_time;
}

void SL_source_mix(SL_Source_t *source, float *output, size_t frames_requested, const SL_Mix_t *groups)
{
    if (source->state != SL_SOURCE_STATE_PLAYING) {
        return;
    }

    size_t frames_processed = 0;

    ma_uint8 input[1024];
    ma_uint32 input_frame_cap = sizeof(input) / ma_get_bytes_per_frame(source->converter.config.formatIn, source->converter.config.channelsIn);

    float buffer[frames_requested * SL_CHANNELS_PER_FRAME];
    float *cursor = buffer;

    size_t frames_remaining = frames_requested;
    while (frames_remaining > 0) { // Read as much data as possible, filling the buffer and eventually looping!
        ma_uint64 frames_to_read = ma_data_converter_get_required_input_frame_count(&source->converter, frames_remaining);
        if (frames_to_read > input_frame_cap) {
            frames_to_read = input_frame_cap;
        }

        size_t frames_read = source->on_read(source->user_data, input, frames_to_read);

        ma_uint64 frames_input = frames_read;
        ma_uint64 frames_converted = frames_remaining;
        ma_data_converter_process_pcm_frames(&source->converter, input, &frames_input, cursor, &frames_converted);

        frames_processed += frames_converted;
        if (frames_read < frames_to_read) { // Less than requested, we reached end-of-data!
            if (!source->looped) {
                source->state = SL_SOURCE_STATE_COMPLETED;
                break;
            }
            source->on_seek(source->user_data, 0);
        }
        frames_remaining -= frames_converted;

        cursor += frames_converted * SL_CHANNELS_PER_FRAME;
    }

    float *sptr = buffer; // Apply panning and gain to the data.
    float *dptr = output;

    const float left = source->mix.left * groups[source->group].left;
    const float right = source->mix.right * groups[source->group].right;

    for (size_t i = 0; i < frames_processed; ++i) { // Each source adds up in the output buffer.
        *(dptr++) += *(sptr++) * left;
        *(dptr++) += *(sptr++) * right;
    }
}
