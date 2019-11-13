/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "audio.h"

#include <core/platform.h>
#include <libs/log.h>

#include <stdbool.h>
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

static const char *_backends[] = {
    "wasapi",
    "dsound",
    "winmm",
    "coreaudio",
    "sndio",
    "audio4",
    "oss",
    "pulseaudio",
    "alsa",
    "jack",
    "aaudio",
    "opensl",
    "webaudio",
    "null"
};

static const char *_formats[] = {
    "unknown",
    "u8",
    "s16",
    "s24",
    "s32",
    "f32"
};

static void device_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
    Audio_t *audio = (Audio_t *)device->pUserData;
    Log_write(LOG_LEVELS_DEBUG, "<AUDIO> %d frames requested for instance %p", frame_count, audio);
}

bool Audio_initialize(Audio_t *audio, const Audio_Configuration_t *configuration)
{
    *audio = (Audio_t){ 0 };

    audio->configuration = *configuration;

    audio->device_config = ma_device_config_init(ma_device_type_playback);

    audio->device_config.playback.format    = ma_format_u8;
    audio->device_config.playback.channels  = configuration->channels ? configuration->channels : audio->device_config.playback.channels;
    audio->device_config.sampleRate         = configuration->sample_rate ? configuration->sample_rate : audio->device_config.sampleRate;
    audio->device_config.dataCallback       = device_callback;
    audio->device_config.pUserData          = (void *)audio;

    ma_result result = ma_device_init(NULL, &audio->device_config, &audio->device);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, "<AUDIO> can't initialize device");
        return false;
    }

    Log_write(LOG_LEVELS_INFO, "<AUDIO> Backend: %s", _backends[audio->device.pContext->backend]);
    Log_write(LOG_LEVELS_INFO, "<AUDIO> Format: %s/%d", _formats[audio->device.playback.format], audio->device.playback.channels);
    Log_write(LOG_LEVELS_INFO, "<AUDIO> Internal format: %s/%d", _formats[audio->device.playback.internalFormat], audio->device.playback.internalChannels);
    Log_write(LOG_LEVELS_INFO, "<AUDIO> Sample-rate: %d", audio->device.sampleRate);
    Log_write(LOG_LEVELS_INFO, "<AUDIO> Device-name: %s", audio->device.playback.name);

    return true;
}

void Audio_terminate(Audio_t *audio)
{
    ma_device_uninit(&audio->device);
}
