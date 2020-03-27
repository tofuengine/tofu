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
    Audio_t *audio = (Audio_t *)device->pUserData;

    memset(output, 0, frame_count * device->playback.channels * ma_get_bytes_per_sample(device->playback.format));

//    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d frames requested for instance %p", frame_count, audio);

    ma_mutex_lock(&audio->lock);
    ma_mutex_unlock(&audio->lock);
}

static Audio_Stream_t *_stream_create(void *data, size_t data_size, ma_format format, ma_uint32 channels, ma_uint32 sample_rate)
{
    ma_data_converter_config config = ma_data_converter_config_init(format, DEVICE_FORMAT, channels, DEVICE_CHANNELS, sample_rate, DEVICE_SAMPLE_RATE);

    ma_data_converter converter;
    ma_result result = ma_data_converter_init(&config, &converter);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't initialized data converter");
        return NULL;
    }

    Audio_Stream_t *stream = malloc(sizeof(Audio_Stream_t));
    if (!stream) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate audio stream");
        ma_data_converter_uninit(&converter);
        return NULL;
    }

    *stream = (Audio_Stream_t){
            .converter = converter,
            .data = data,
            .data_size = data_size,
            .index = 0,
            .state = AUDIO_STREAM_STATE_STOPPED,
            .volume = 1.0f,
            .panning = 0.0f
        };

    return stream;
}

static void _stream_destroy(Audio_Stream_t *stream)
{
    if (!stream) {
        return;
    }

    ma_data_converter_uninit(&stream->converter);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio-stream converter uninitialized");

    free(stream->data);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio-stream data freed");

    free(stream);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio-stream freed");
}

bool Audio_initialize(Audio_t *audio, const Audio_Configuration_t *configuration)
{
    *audio = (Audio_t){ 0 };

    audio->configuration = *configuration;

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

//    for (size_t i = 0; i < audio->configuration.voices; ++i) {
//        audio->voices[i] = _audio_buffer_create(DEVICE_FORMAT, DEVICE_CHANNELS, DEVICE_SAMPLE_RATE, 0);
//    }

    return true;
}

void Audio_terminate(Audio_t *audio)
{
    size_t count = arrlen(audio->streams);
    for (int i = count - 1; i >= 0; --i) {
        _stream_destroy(audio->streams[i]);
    }
    arrfree(audio->streams);

    ma_mutex_uninit(&audio->lock);
    ma_device_uninit(&audio->device);
    ma_context_uninit(&audio->context);
}

void Audio_set_master_volume(Audio_t *audio, float volume)
{
    ma_device_set_master_volume(&audio->device, volume);
}

float Audio_get_master_volume(Audio_t *audio)
{
    float volume;
    ma_device_get_master_volume(&audio->device, &volume);
    return volume;
}

Audio_Stream_t *Audio_stream_create(Audio_t *audio, void *data, size_t data_size)
{
    Audio_Stream_t *stream = _stream_create(data, data_size, 0, 0, 0);
    if (!stream) {
        return NULL;
    }

    arrpush(audio->streams, stream);

    return stream;
}

void Audio_stream_destroy(Audio_t *audio, Audio_Stream_t *stream)
{
    _stream_destroy(stream);

    size_t count = arrlen(audio->streams);
    for (int i = count - 1; i >= 0; --i) {
        if (audio->streams[i] == stream) {
            arrdel(audio->streams, i);
            break;
        }
    }
}

void Audio_stream_play(Audio_Stream_t *stream)
{
    if (stream->state != AUDIO_STREAM_STATE_STOPPED) {
        return;
    }
    stream->state = AUDIO_STREAM_STATE_PLAYING;
    stream->index = 0;
}

void Audio_stream_pause(Audio_Stream_t *stream)
{
    if (stream->state != AUDIO_STREAM_STATE_PLAYING) {
        return;
    }
    stream->state = AUDIO_STREAM_STATE_STOPPED;
}

void Audio_stream_resume(Audio_Stream_t *stream)
{
    if (stream->state != AUDIO_STREAM_STATE_STOPPED) {
        return;
    }
    stream->state = AUDIO_STREAM_STATE_PLAYING;
}

void Audio_stream_stop(Audio_Stream_t *stream)
{
    if (stream->state != AUDIO_STREAM_STATE_PLAYING) {
        return;
    }
    stream->state = AUDIO_STREAM_STATE_STOPPED;
    stream->index = 0;
}

void Audio_update(Audio_t *audio, float delta_time)
{
    audio->time += delta_time;
}
