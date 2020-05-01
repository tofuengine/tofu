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

#include <config.h>
#include <libs/log.h>

#define MA_DEBUG_OUTPUT
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#define LOG_CONTEXT "audio"

// We are using floating point format for simpler and more consistent mixing. Two channels are enough to have some
// panning effects. A sample rate of 48kHz is the optimal choice since it's the internal default for many soundcards
// and converting from lower sample rates is simpler.
#define DEVICE_FORMAT           ma_format_f32
#define DEVICE_CHANNELS         2
#define DEVICE_SAMPLE_RATE      48000

static void _log_callback(ma_context *context, ma_device *device, ma_uint32 log_level, const char *message)
{
    Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "[%p:%p] %d %s", context, device, log_level, message);
}

static void _data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
//    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d frames requested for device %p", frame_count, device);

    Audio_t *audio = (Audio_t *)device->pUserData;

    float *to_device = (float *)output;
//    memset(to_device, 0, frame_count * device->playback.channels * ma_get_bytes_per_sample(device->playback.format));
    memset(to_device, 0x00, frame_count * DEVICE_CHANNELS * sizeof(float));

    ma_mutex_lock(&audio->lock);

    SL_context_process(audio->sl, to_device, frame_count);

    ma_mutex_unlock(&audio->lock);
}

bool Audio_initialize(Audio_t *audio, const Audio_Configuration_t *configuration)
{
    *audio = (Audio_t){ 0 };

    audio->configuration = *configuration;

    audio->context_config = ma_context_config_init();
    audio->context_config.pUserData = (void *)audio;
    audio->context_config.logCallback = _log_callback;

    audio->sl = SL_context_create();
    if (!audio->sl) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create the sound context");
        return false;
    }

    ma_result result = ma_context_init(NULL, 0, &audio->context_config, &audio->context);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize the audio context");
        SL_context_destroy(audio->sl);
        return false;
    }

    audio->device_config = ma_device_config_init(ma_device_type_playback);
    // TODO: loop over available devices and use the one specified in the configuration. Useful when more than one device is available.
    //    config.playback.pDeviceID = &pPlaybackDeviceInfos[chosenPlaybackDeviceIndex].id; 
    audio->device_config.playback.format    = DEVICE_FORMAT;
    audio->device_config.playback.channels  = DEVICE_CHANNELS;
    audio->device_config.sampleRate         = DEVICE_SAMPLE_RATE;
    audio->device_config.dataCallback       = _data_callback;
//    audio->device_config.stopCallback       = _stop_callback;
    audio->device_config.pUserData          = (void *)audio;

    result = ma_device_init(&audio->context, &audio->device_config, &audio->device);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize the audio device");
        ma_context_uninit(&audio->context);
        SL_context_destroy(audio->sl);
        return false;
    }

    result = ma_mutex_init(&audio->context, &audio->lock);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create the synchronization object");
        ma_device_uninit(&audio->device);
        ma_context_uninit(&audio->context);
        SL_context_destroy(audio->sl);
        return false;
    }

    // FIXME: start only on incoming data and pause (in the update function) when no more data is present.
    result = ma_device_start(&audio->device); // The audio device will be always running, waiting to process data.
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't start the audio device");
        ma_mutex_uninit(&audio->lock);
        ma_device_uninit(&audio->device);
        ma_context_uninit(&audio->context);
        SL_context_destroy(audio->sl);
        return false;
    }

    audio->volume = configuration->master_volume;
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
    ma_device_stop(&audio->device);
    ma_mutex_uninit(&audio->lock);
    ma_device_uninit(&audio->device);
    ma_context_uninit(&audio->context);
    SL_context_destroy(audio->sl);
}

void Audio_volume(Audio_t *audio, float volume)
{
    ma_mutex_lock(&audio->lock);
    audio->volume = volume;
    ma_device_set_master_volume(&audio->device, volume);
    ma_mutex_unlock(&audio->lock);
}

void Audio_update(Audio_t *audio, float delta_time)
{
    SL_context_update(audio->sl, delta_time);
}

void Audio_stop(Audio_t *audio)
{
    SL_context_stop(audio->sl);
}

SL_Context_t *Audio_lock(Audio_t *audio)
{
    ma_mutex_lock(&audio->lock);
    SL_Context_t *context = audio->sl;
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "audio context %p locked", context);
    return context;
}

void Audio_unlock(Audio_t *audio, SL_Context_t *context)
{
    if (context != audio->sl) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "mismatched context %p", context);
        return;
    }
    ma_mutex_unlock(&audio->lock);
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "audio context %p unlocked", context);
}