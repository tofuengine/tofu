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

#ifdef DEBUG
  #define MA_DEBUG_OUTPUT
#endif
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#define LOG_CONTEXT "audio"

static void _log_callback(ma_context *context, ma_device *device, ma_uint32 log_level, const char *message)
{
    Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "[%p:%p] %d %s", context, device, log_level, message);
}

// Note that output buffer is already pre-zeroed upon call.
static void _data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
    Audio_t *audio = (Audio_t *)device->pUserData;

    ma_mutex_lock(&audio->lock);
//    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d frames requested for device %p", frame_count, device);
    SL_context_generate(audio->sl, output, frame_count);
    ma_mutex_unlock(&audio->lock);
}

Audio_t *Audio_create(const Audio_Configuration_t *configuration)
{
    Audio_t *audio = malloc(sizeof(Audio_t));
    if (!audio) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate audio");
        return NULL;
    }

    *audio = (Audio_t){
            .configuration = *configuration
        };

    audio->context_config = ma_context_config_init();
    audio->context_config.pUserData = (void *)audio;
    audio->context_config.logCallback = _log_callback;

    audio->sl = SL_context_create();
    if (!audio->sl) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create the sound context");
        free(audio);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sound context created at %p", audio->sl);

    ma_result result = ma_context_init(NULL, 0, &audio->context_config, &audio->context);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize the audio context");
        SL_context_destroy(audio->sl);
        free(audio);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio driver context created");

    audio->device_config = ma_device_config_init(ma_device_type_playback);
    // TODO: loop over available devices and use the one specified in the configuration. Useful when more than one device is available.
    //    config.playback.pDeviceID = &pPlaybackDeviceInfos[chosenPlaybackDeviceIndex].id; 
#if SL_BYTES_PER_SAMPLE == 2
    audio->device_config.playback.format         = ma_format_s16;
#elif SL_BYTES_PER_SAMPLE == 4
    audio->device_config.playback.format         = ma_format_f32;
#endif
    audio->device_config.playback.channels       = SL_CHANNELS_PER_FRAME;
    audio->device_config.sampleRate              = SL_FRAMES_PER_SECOND;
    audio->device_config.dataCallback            = _data_callback;
//    audio->device_config.stopCallback            = _stop_callback;
    audio->device_config.pUserData               = (void *)audio;
    audio->device_config.noPreZeroedOutputBuffer = MA_FALSE;
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio device configured w/ %dHz, %d channel(s), %d byte(s) per sample", SL_FRAMES_PER_SECOND, SL_CHANNELS_PER_FRAME, SL_BYTES_PER_SAMPLE);

    result = ma_device_init(&audio->context, &audio->device_config, &audio->device);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize the audio device");
        ma_context_uninit(&audio->context);
        SL_context_destroy(audio->sl);
        free(audio);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio device initialized");

    result = ma_mutex_init(&audio->lock);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create the synchronization object");
        ma_device_uninit(&audio->device);
        ma_context_uninit(&audio->context);
        SL_context_destroy(audio->sl);
        free(audio);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio device mutex initialized");

    audio->is_started = false;

    ma_device_set_master_volume(&audio->device, configuration->master_volume); // Set the initial volume.
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio master-volume set to %.2f", configuration->master_volume);

    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "miniaudio: v%s", ma_version_string());
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "device-name: %s", audio->device.playback.name);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "back-end: %s", ma_get_backend_name(audio->context.backend));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "format: %s / %s", ma_get_format_name(audio->device.playback.format), ma_get_format_name(audio->device.playback.internalFormat));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "channels: %d / %d", audio->device.playback.channels, audio->device.playback.internalChannels);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "sample-rate: %d / %d", audio->device.sampleRate, audio->device.playback.internalSampleRate);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "period-in-frames: %d", audio->device.playback.internalPeriodSizeInFrames);

    return audio;
}

void Audio_destroy(Audio_t *audio)
{
    if (audio->is_started) {
        ma_device_stop(&audio->device);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio device stopped");
    }
    ma_mutex_uninit(&audio->lock);
    ma_device_uninit(&audio->device);
    ma_context_uninit(&audio->context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio deinitialized");

    SL_context_destroy(audio->sl);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sound context destroyed");

    free(audio);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio freed");
}

void Audio_halt(Audio_t *audio)
{
    ma_mutex_lock(&audio->lock);
    SL_context_halt(audio->sl);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "halted, no more sources active");
    ma_mutex_unlock(&audio->lock);
}

void Audio_set_volume(Audio_t *audio, float volume)
{
//    ma_mutex_lock(&audio->lock);
    ma_device_set_master_volume(&audio->device, volume);
//    ma_mutex_unlock(&audio->lock);
}

void Audio_set_mix(Audio_t *audio, size_t group_id, SL_Mix_t mix)
{
    ma_mutex_lock(&audio->lock);
    SL_context_set_mix(audio->sl, group_id, mix);
    ma_mutex_unlock(&audio->lock);
}

void Audio_set_pan(Audio_t *audio, size_t group_id, float pan)
{
    ma_mutex_lock(&audio->lock);
    SL_context_set_pan(audio->sl, group_id, pan);
    ma_mutex_unlock(&audio->lock);
}

void Audio_set_balance(Audio_t *audio, size_t group_id, float balance)
{
    ma_mutex_lock(&audio->lock);
    SL_context_set_balance(audio->sl, group_id, balance);
    ma_mutex_unlock(&audio->lock);
}

void Audio_set_gain(Audio_t *audio, size_t group_id, float gain)
{
    ma_mutex_lock(&audio->lock);
    SL_context_set_gain(audio->sl, group_id, gain);
    ma_mutex_unlock(&audio->lock);
}

float Audio_get_volume(const Audio_t *audio)
{
//    ma_mutex_lock(&audio->lock);
    float volume;
    ma_result result = ma_device_get_master_volume((ma_device *)&audio->device, &volume);
    if (result != MA_SUCCESS) {
        return 0.0f;
    }
    return volume;
//    ma_mutex_unlock(&audio->lock);
}

SL_Mix_t Audio_get_mix(const Audio_t *audio, size_t group_id)
{
    ma_mutex_lock((ma_mutex *)&audio->lock);
    SL_Mix_t mix = SL_context_get_group(audio->sl, group_id)->mix;
    ma_mutex_unlock((ma_mutex *)&audio->lock);
    return mix;
}

float Audio_get_gain(const Audio_t *audio, size_t group_id)
{
    ma_mutex_lock((ma_mutex *)&audio->lock);
    float gain = SL_context_get_group(audio->sl, group_id)->gain;
    ma_mutex_unlock((ma_mutex *)&audio->lock);
    return gain;
}

void Audio_track(Audio_t *audio, SL_Source_t *source, bool reset)
{
    ma_mutex_lock(&audio->lock);
    if (reset) {
        SL_source_reset(source); // FIXME: use return value!!!
    }
    SL_context_track(audio->sl, source);
    size_t count = SL_context_count_tracked(audio->sl);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p tracked, #%d source(s) active", source, count);
    ma_mutex_unlock(&audio->lock);
}

void Audio_untrack(Audio_t *audio, SL_Source_t *source)
{
    ma_mutex_lock(&audio->lock);
    SL_context_untrack(audio->sl, source);
    size_t count = SL_context_count_tracked(audio->sl);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p untracked, #%d source(s) active", source, count);
    ma_mutex_unlock(&audio->lock);
}

bool Audio_is_tracked(const Audio_t *audio, SL_Source_t *source)
{
    ma_mutex_lock((ma_mutex *)&audio->lock);
    bool is_tracked = SL_context_is_tracked(audio->sl, source);
    ma_mutex_unlock((ma_mutex *)&audio->lock);
    return is_tracked;
}

bool Audio_update(Audio_t *audio, float delta_time)
{
    ma_mutex_lock(&audio->lock);
    bool updated = SL_context_update(audio->sl, delta_time);
    size_t count = SL_context_count_tracked(audio->sl);
    ma_mutex_unlock(&audio->lock);

    if (!updated) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't update context");
        return false;
    }

    if (!audio->is_started && count == 1) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source incoming, starting device");
        ma_result result = ma_device_start(&audio->device);
        if (result != MA_SUCCESS) {
            Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't start the audio device");
            return false;
        }
        audio->is_started = true;
    } else
    if (audio->is_started && count == 0) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "no more sources, stopping device");
        ma_result result = ma_device_stop(&audio->device);
        if (result != MA_SUCCESS) {
            Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't stop the audio device");
            return false;
        }
        audio->is_started = false;
    }

    return true;
}
