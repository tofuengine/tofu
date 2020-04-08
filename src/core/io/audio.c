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

#include <stdbool.h>
#if 0
#define DR_FLAC_IMPLEMENTATION
#include <miniaudio/extras/dr_flac.h>    // Enables FLAC decoding.
#define DR_MP3_IMPLEMENTATION
#include <miniaudio/extras/dr_mp3.h>     // Enables MP3 decoding.
#define DR_WAV_IMPLEMENTATION
#include <miniaudio/extras/dr_wav.h>     // Enables WAV decoding.
#endif
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#define LOG_CONTEXT "audio"

#define DEVICE_FORMAT           ma_format_f32
#define DEVICE_CHANNELS         2
#define DEVICE_SAMPLE_RATE      44100

static void _log_callback(ma_context *context, ma_device *device, ma_uint32 log_level, const char *message)
{
    Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "[%p:%p] %d %s", context, device, log_level, message);
}

static void _device_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
//    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d frames requested for instance %p", frame_count, audio);
    Audio_t *audio = (Audio_t *)device->pUserData;

    memset(output, 0, frame_count * device->playback.channels * ma_get_bytes_per_sample(device->playback.format));

    ma_mutex_lock(&audio->lock);

//    float buffer[frame_count];

    size_t count = arrlen(audio->sources);
    for (int i = count - 1; i >= 0; --i) {
        Audio_Source_t *source = audio->sources[i];

        if (source->state != AUDIO_SOURCE_STATE_PLAYING) {
            continue;
        }

//        size_t read_frames = source->reader(source->user_data, buffer, frame_count);

        // TODO: mix the read buffer into the output one.
    }

    ma_mutex_unlock(&audio->lock);
}

static Audio_Source_t *_source_create(const Audio_t *audio, Audio_Source_Read_Callback_t reader, Audio_Source_Seek_Callback_t seeker, void *user_data, ma_format format, ma_uint32 channels, ma_uint32 sample_rate)
{
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

static void _source_destroy(Audio_Source_t *source)
{
    if (!source) {
        return;
    }

    ma_data_converter_uninit(&source->converter);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio-source converter uninitialized");

    free(source);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio-source freed");
}

bool Audio_initialize(Audio_t *audio, const Audio_Configuration_t *configuration)
{
    *audio = (Audio_t){ 0 };

    audio->configuration = *configuration;

    audio->mix = (Audio_Mix_t){ .left = 1.0f, .right = 1.0f }; // TODO: call panning law?

    audio->context_config = ma_context_config_init();
    audio->context_config.pUserData = (void *)audio;
    audio->context_config.logCallback = _log_callback;

    ma_result result = ma_context_init(NULL, 0, &audio->context_config, &audio->context);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize the context");
        return false;
    }

    audio->device_config = ma_device_config_init(ma_device_type_playback);
    audio->device_config.playback.format    = DEVICE_FORMAT; // Using floating point format for simpler mixing.
    audio->device_config.playback.channels  = DEVICE_CHANNELS;
    audio->device_config.sampleRate         = DEVICE_SAMPLE_RATE;
    audio->device_config.dataCallback       = _device_callback;
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
        _source_destroy(audio->sources[i]);
    }
    arrfree(audio->sources);

    ma_mutex_uninit(&audio->lock);
    ma_device_uninit(&audio->device);
    ma_context_uninit(&audio->context);
}

void Audio_mix(Audio_t *audio, Audio_Mix_t mix)
{
    audio->mix = mix;
}

Audio_Source_t *Audio_source_create(Audio_t *audio, Audio_Source_Read_Callback_t reader, Audio_Source_Seek_Callback_t seeker, void *user_data)
{
    Audio_Source_t *source = _source_create(audio, reader, seeker, user_data, 0, 0, 0); // TODO: pass the correct audio format.
    if (!source) {
        return NULL;
    }

    arrpush(audio->sources, source);

    return source;
}

void Audio_source_destroy(Audio_t *audio, Audio_Source_t *source)
{
    _source_destroy(source);

    size_t count = arrlen(audio->sources);
    for (int i = count - 1; i >= 0; --i) {
        if (audio->sources[i] == source) {
            arrdel(audio->sources, i);
            break;
        }
    }
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
