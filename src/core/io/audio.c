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

#include "audio.h"

#include <libs/log.h>
#include <libs/stb.h>
#include <libs/wave.h>

#include <stdbool.h>
#if 0
#define DR_FLAC_IMPLEMENTATION
#include <miniaudio/extras/dr_flac.h>    // Enables FLAC decoding.
#define DR_MP3_IMPLEMENTATION
#include <miniaudio/extras/dr_mp3.h>     // Enables MP3 decoding.
#define DR_WAV_IMPLEMENTATION
#include <miniaudio/extras/dr_wav.h>     // Enables WAV decoding.
#endif
#define MA_DEBUG_OUTPUT
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#define LOG_CONTEXT "audio"

static float seconds_offset = 0.0f;

// Using floating point format for simpler and more consistent mixing.
#define DEVICE_FORMAT           ma_format_f32
#define DEVICE_CHANNELS         2
#define DEVICE_SAMPLE_RATE      44100

Audio_Mix_t _0db_linear_mix(float balance, float gain)
{
#if 1
    if (balance < 0.0f) {
        return (Audio_Mix_t){ .left = gain, .right = (1.0f + balance) * gain };
    } else
    if (balance > 0.0f) {
        return (Audio_Mix_t){ .left = (1.0f - balance) * gain, .right = gain };
    } else {
        return (Audio_Mix_t){ .left = gain, .right = gain };
    }
#else
    const float theta = (balance + 1.0f) * 0.5f * M_PI_2; // [-1, 1] -> [0 , 1] -> [0, pi/2]
    return (Audio_Mix_t){ .left = powf(cosf(theta), 1.5f) * gain, .right = powf(sinf(theta), 1.5f) * gain };
#endif
}

static void _log_callback(ma_context *context, ma_device *device, ma_uint32 log_level, const char *message)
{
    Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "[%p:%p] %d %s", context, device, log_level, message);
}

static void _data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
//    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d frames requested for instance %p", frame_count, audio);
    Audio_t *audio = (Audio_t *)device->pUserData;

    float *to_device = (float *)output;
//    memset(to_device, 0, frame_count * device->playback.channels * ma_get_bytes_per_sample(device->playback.format));
    memset(to_device, 0x00, frame_count * DEVICE_CHANNELS * sizeof(float));

    ma_mutex_lock(&audio->lock);

    float from_source[frame_count * DEVICE_CHANNELS];
    memset(to_device, 0x00, sizeof(from_source));

    size_t count = arrlen(audio->sources);
    for (int i = count - 1; i >= 0; --i) {
        Audio_Source_t *source = audio->sources[i];

        if (source->state != AUDIO_SOURCE_STATE_PLAYING) {
            continue;
        }

        size_t read_frames = frame_count; //source->reader(source->user_data, buffer, frame_count);

        // TODO: mix the read buffer into the output one.
        for (size_t i = 0; i < read_frames; ++i) {
            size_t index = i * 2;

            float left = from_source[index];
            float right = from_source[index + 1];

            left *= source->mix.left;
            right *= source->mix.right;

            to_device[index] += left;
            to_device[index + 1] += right;
        }
    }

    float seconds_per_frame = 1.0f / DEVICE_SAMPLE_RATE;
    float pitch = 440.0f;

    float mix[DEVICE_CHANNELS] = { audio->mix.left, audio->mix.right };

    float *ptr = to_device;
    for (ma_uint32 frame = 0; frame < frame_count; ++frame) {
        float sample = wave_sine(seconds_offset * pitch);
        for (int channel = 0; channel < DEVICE_CHANNELS; ++channel) {
            *(ptr++) = sample * mix[channel];
        }
        seconds_offset += seconds_per_frame;
    }

    ma_mutex_unlock(&audio->lock);
}

bool Audio_initialize(Audio_t *audio, const Audio_Configuration_t *configuration)
{
    *audio = (Audio_t){ 0 };

    audio->configuration = *configuration;

    audio->volume = 1.0f;
    audio->balance = 0.0f;
    audio->mix = _0db_linear_mix(0.0f, 1.0f);

    audio->context_config = ma_context_config_init();
    audio->context_config.pUserData = (void *)audio;
    audio->context_config.logCallback = _log_callback;

    ma_result result = ma_context_init(NULL, 0, &audio->context_config, &audio->context);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize the context");
        return false;
    }

    audio->device_config = ma_device_config_init(ma_device_type_playback);
    // TODO: loop over available devices and use the one specified in the configuration. Useful when more than one device is available.
    //    config.playback.pDeviceID = &pPlaybackDeviceInfos[chosenPlaybackDeviceIndex].id; 
    audio->device_config.playback.format    = DEVICE_FORMAT;
    audio->device_config.playback.channels  = DEVICE_CHANNELS;
    audio->device_config.sampleRate         = DEVICE_SAMPLE_RATE; // TODO: detect and pass internal sample rate.
    audio->device_config.dataCallback       = _data_callback;
//    audio->device_config.stopCallback       = _stop_callback;
    audio->device_config.pUserData          = (void *)audio;

    result = ma_device_init(&audio->context, &audio->device_config, &audio->device);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize then device");
        ma_context_uninit(&audio->context);
        return false;
    }

    result = ma_mutex_init(&audio->context, &audio->lock);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create synchronization object");
        ma_device_uninit(&audio->device);
        ma_context_uninit(&audio->context);
        return false;
    }

    // FIXME: start only on incoming data and pause (in the update function) when no more data is present.
    result = ma_device_start(&audio->device); // The audio device will be always running, waiting to process data.
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't start then device");
        ma_mutex_uninit(&audio->lock);
        ma_device_uninit(&audio->device);
        ma_context_uninit(&audio->context);
        return false;
    }

    ma_device_set_master_volume(&audio->device, configuration->master_volume); // Set the initial volume.

    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "device-name: %s", audio->device.playback.name);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "back-end: miniaudio / %s", ma_get_backend_name(audio->context.backend));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "format: %s / %s", ma_get_format_name(audio->device.playback.format), ma_get_format_name(audio->device.playback.internalFormat));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "channels: %d / %d", audio->device.playback.channels, audio->device.playback.internalChannels);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "sample-rate: %d / %d", audio->device.sampleRate, audio->device.playback.internalSampleRate);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "period-in-frames: %d", audio->device.playback.internalPeriodSizeInFrames);

    return true;
}

void Audio_terminate(Audio_t *audio)
{
    size_t count = arrlen(audio->sources);
    for (int i = count - 1; i >= 0; --i) {
        Audio_source_destroy(audio->sources[i]);
    }
    arrfree(audio->sources);

    ma_device_stop(&audio->device);
    ma_mutex_uninit(&audio->lock);
    ma_device_uninit(&audio->device);
    ma_context_uninit(&audio->context);
}

void Audio_volume(Audio_t *audio, float volume)
{
    audio->volume = volume;
    audio->mix = _0db_linear_mix(audio->balance, volume);
}

void Audio_balance(Audio_t *audio, float balance)
{
    // TODO: add mutex? no...
    audio->balance = balance;
    audio->mix = _0db_linear_mix(balance, audio->volume);
}

void Audio_mix(Audio_t *audio, Audio_Mix_t mix)
{
    audio->mix = mix;
}

void Audio_track(Audio_t *audio, Audio_Source_t *source)
{
    ma_mutex_lock(&audio->lock);
    arrpush(audio->sources, source);
    ma_mutex_unlock(&audio->lock);
}

void Audio_untrack(Audio_t *audio, Audio_Source_t *source)
{
    ma_mutex_lock(&audio->lock);
    size_t count = arrlen(audio->sources);
    for (int i = count - 1; i >= 0; --i) {
        if (audio->sources[i] == source) {
            arrdel(audio->sources, i);
            break;
        }
    }
    ma_mutex_unlock(&audio->lock);
}

Audio_Source_t *Audio_source_create(Audio_Source_Read_Callback_t reader, Audio_Source_Seek_Callback_t seeker, void *user_data)
{
    ma_format format = ma_format_s16;
    ma_uint32 channels = 0;
    ma_uint32 sample_rate = 0;

    ma_data_converter_config config = ma_data_converter_config_init(format, DEVICE_FORMAT, channels, DEVICE_CHANNELS, sample_rate, DEVICE_SAMPLE_RATE);

    ma_data_converter converter;
    ma_result result = ma_data_converter_init(&config, &converter);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialized data converter");
        return NULL;
    }

    Audio_Source_t *source = malloc(sizeof(Audio_Source_t));
    if (!source) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate audio source");
        ma_data_converter_uninit(&converter);
        return NULL;
    }

    *source = (Audio_Source_t){
            .converter = converter,
            .user_data = user_data,
            .reader = reader,
            .seeker = seeker,
            .state = AUDIO_SOURCE_STATE_STOPPED,
            .looping = false,
            .mix = (Audio_Mix_t){ .left = 1.0f, .right = 1.0f }
        };

    return source;
}

void Audio_source_destroy(Audio_Source_t *source)
{
    if (!source) {
        return;
    }

    ma_data_converter_uninit(&source->converter);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio-source converter uninitialized");

    free(source);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio-source freed");
}

void Audio_source_mix(Audio_Source_t *source, Audio_Mix_t mix)
{
    source->mix = mix;
}

void Audio_source_play(Audio_Source_t *source)
{
    if (source->state != AUDIO_SOURCE_STATE_STOPPED) {
        return;
    }
    source->state = AUDIO_SOURCE_STATE_PLAYING;
    source->seeker(source->user_data, 0, 0);
}

void Audio_source_pause(Audio_Source_t *source)
{
    if (source->state != AUDIO_SOURCE_STATE_PLAYING) {
        return;
    }
    source->state = AUDIO_SOURCE_STATE_STOPPED;
}

void Audio_source_resume(Audio_Source_t *source)
{
    if (source->state != AUDIO_SOURCE_STATE_STOPPED) {
        return;
    }
    source->state = AUDIO_SOURCE_STATE_PLAYING;
}

void Audio_source_stop(Audio_Source_t *source)
{
    if (source->state != AUDIO_SOURCE_STATE_PLAYING) {
        return;
    }
    source->state = AUDIO_SOURCE_STATE_STOPPED;
    source->seeker(source->user_data, 0, 0);
}

void Audio_update(Audio_t *audio, float delta_time)
{
    audio->time += delta_time;
}
