/*
 * MIT License
 *
 * Copyright (c) 2019-2021 Marco Lizza
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
#include <libs/stb.h>

#ifdef DEBUG
  #define MA_DEBUG_OUTPUT

  #ifndef SANITIZE
    #define MA_MALLOC(sz)     stb_leakcheck_malloc((sz), __FILE__, __LINE__)
    #define MA_REALLOC(p, sz) stb_leakcheck_realloc((p), (sz), __FILE__, __LINE__)
    #define MA_FREE(p)        stb_leakcheck_free((p))
  #endif
#endif
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#define LOG_CONTEXT "audio"

static void _log_callback(void *user_data, ma_uint32 level, const char *message)
{
    static int _levels[] = {
        LOG_LEVELS_FATAL,   // !!!UNUSED!!!
        LOG_LEVELS_ERROR,   // #define MA_LOG_LEVEL_ERROR      1
        LOG_LEVELS_WARNING, // #define MA_LOG_LEVEL_WARNING    2
        LOG_LEVELS_INFO,    // #define MA_LOG_LEVEL_INFO       3
        LOG_LEVELS_DEBUG    // #define MA_LOG_LEVEL_DEBUG      4
    };
    Log_write(_levels[level], "miniaudio", message);
}

typedef struct enum_callback_closure_s {
    int current_index;
    int device_index;
    ma_device_id device_id;
    bool found;
} enum_callback_closure_t;

static ma_bool32 _enum_callback(ma_context *context, ma_device_type device_type, const ma_device_info *device_info, void *user_data)
{
    enum_callback_closure_t *closure = (enum_callback_closure_t *)user_data;

    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "device `%s` w/ type %d", device_info->name, device_type);

    if (device_type & ma_device_type_playback) { // We are considering the output devices only.
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "device #%d, `%s` available", closure->current_index, device_info->name, device_type);

        if (closure->current_index == closure->device_index) {
            closure->device_id = device_info->id;
            Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "device #%d, `%s` selected", closure->current_index, device_info->name, device_type);
            closure->found = true;
        }

        closure->current_index += 1;
    }

    return MA_TRUE;
}

// Note that output buffer is already pre-silenced upon call.
static void _data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
    Audio_t *audio = (Audio_t *)device->pUserData;

    ma_mutex_lock(&audio->driver.lock);
//    ma_silence_pcm_frames(output, frame_count, ma_format_s16, SL_CHANNELS_PER_FRAME);
//    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "%d frames requested for device %p", frame_count, device);
    SL_context_generate(audio->context, output, frame_count);
    ma_mutex_unlock(&audio->driver.lock);
}

static void _notification_callback(const ma_device_notification *notification)
{
    static const char *types[] = {
        "started",
        "stopped",
        "rerouted",
        "interruption-began",
        "interruption-ended"
    };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "device %p notified for event `%s`", notification->pDevice, types[notification->type]);
}

static void *_malloc(size_t sz, void *pUserData)
{
    return malloc(sz);
}

static void *_realloc(void *ptr, size_t sz, void *pUserData)
{
    return realloc(ptr, sz);
}

static void  _free(void *ptr, void *pUserData)
{
    free(ptr);
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

    audio->context = SL_context_create();
    if (!audio->context) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create the sound context");
        free(audio);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sound context created at %p", audio->context);

    ma_result result = ma_mutex_init(&audio->driver.lock);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create the synchronization object");
        SL_context_destroy(audio->context);
        free(audio);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio device mutex initialized");

    ma_log_init(&(ma_allocation_callbacks){
            .pUserData = NULL,
            .onMalloc = _malloc,
            .onRealloc = _realloc,
            .onFree = _free
        }, &audio->driver.log);
    ma_log_callback log_callback = ma_log_callback_init(_log_callback, (void *)audio);
    ma_log_register_callback(&audio->driver.log, log_callback);

    ma_context_config context_config = ma_context_config_init();
    context_config.pLog = &audio->driver.log;
    context_config.allocationCallbacks = (ma_allocation_callbacks){
            .pUserData = NULL,
            .onMalloc = _malloc,
            .onRealloc = _realloc,
            .onFree = _free
        };

    result = ma_context_init(NULL, 0, &context_config, &audio->driver.context);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize the audio context");
        ma_log_uninit(&audio->driver.log);
        ma_mutex_uninit(&audio->driver.lock);
        SL_context_destroy(audio->context);
        free(audio);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio driver context created");

    enum_callback_closure_t closure = {
            .device_index = configuration->device_index
        };
    result = ma_context_enumerate_devices(&audio->driver.context, _enum_callback, &closure);
    if (result != MA_SUCCESS || (!closure.found && configuration->device_index != -1)) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't detect audio device for context %p", audio->driver.context);
        ma_log_uninit(&audio->driver.log);
        ma_mutex_uninit(&audio->driver.lock);
        SL_context_destroy(audio->context);
        free(audio);
        return NULL;
    }

    ma_device_config device_config = ma_device_config_init(ma_device_type_playback);
    if (configuration->device_index == -1) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "using default device for context %p", audio->driver.context);
        device_config.playback.pDeviceID    = NULL;
    } else {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "using device #%d for context %p", closure.device_index, audio->driver.context);
        device_config.playback.pDeviceID    = &closure.device_id;
    }
#if SL_BYTES_PER_SAMPLE == 2
    device_config.playback.format           = ma_format_s16;
#elif SL_BYTES_PER_SAMPLE == 4
    device_config.playback.format           = ma_format_f32;
#endif
    device_config.playback.channels         = SL_CHANNELS_PER_FRAME;
    device_config.sampleRate                = SL_FRAMES_PER_SECOND;
    device_config.dataCallback              = _data_callback;
    device_config.notificationCallback      = _notification_callback;
    device_config.pUserData                 = (void *)audio;
    device_config.noPreSilencedOutputBuffer = MA_FALSE;

    result = ma_device_init(&audio->driver.context, &device_config, &audio->driver.device);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize the audio device");
        ma_context_uninit(&audio->driver.context);
        ma_log_uninit(&audio->driver.log);
        ma_mutex_uninit(&audio->driver.lock);
        SL_context_destroy(audio->context);
        free(audio);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio device initialized w/ %dHz, %d channel(s), %d bytes per sample", SL_FRAMES_PER_SECOND, SL_CHANNELS_PER_FRAME, SL_BYTES_PER_SAMPLE);

    ma_device_set_master_volume(&audio->driver.device, configuration->master_volume); // Set the initial volume.
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio master-volume set to %.2f", configuration->master_volume);

#ifndef __AUDIO_START_AND_STOP__
    result = ma_device_start(&audio->driver.device);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't start the audio device");
        ma_device_uninit(&audio->driver.device);
        ma_context_uninit(&audio->driver.context);
        ma_log_uninit(&audio->driver.log);
        ma_mutex_uninit(&audio->driver.lock);
        SL_context_destroy(audio->context);
        free(audio);
        return NULL;
    }
#endif

    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "miniaudio: v%s", ma_version_string());
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "device-name: %s", audio->driver.device.playback.name);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "back-end: %s", ma_get_backend_name(audio->driver.context.backend));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "format: %s / %s", ma_get_format_name(audio->driver.device.playback.format), ma_get_format_name(audio->driver.device.playback.internalFormat));
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "channels: %d / %d", audio->driver.device.playback.channels, audio->driver.device.playback.internalChannels);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "sample-rate: %d / %d", audio->driver.device.sampleRate, audio->driver.device.playback.internalSampleRate);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "period-in-frames: %d", audio->driver.device.playback.internalPeriodSizeInFrames);

    return audio;
}

void Audio_destroy(Audio_t *audio)
{
    ma_device_uninit(&audio->driver.device); // Device is automatically stopped on deinitialization.
    ma_context_uninit(&audio->driver.context);
    ma_log_uninit(&audio->driver.log);
    ma_mutex_uninit(&audio->driver.lock);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio deinitialized");

    SL_context_destroy(audio->context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sound context destroyed");

    free(audio);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "audio freed");
}

void Audio_halt(Audio_t *audio)
{
    ma_mutex_lock(&audio->driver.lock);
    SL_context_halt(audio->context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "halted, no more sources active");
    ma_mutex_unlock(&audio->driver.lock);
}

void Audio_set_volume(Audio_t *audio, float volume)
{
//    ma_mutex_lock(&audio->driver.lock);
    ma_device_set_master_volume(&audio->driver.device, volume);
//    ma_mutex_unlock(&audio->driver.lock);
}

void Audio_set_mix(Audio_t *audio, size_t group_id, SL_Mix_t mix)
{
    ma_mutex_lock(&audio->driver.lock);
    SL_context_set_mix(audio->context, group_id, mix);
    ma_mutex_unlock(&audio->driver.lock);
}

void Audio_set_pan(Audio_t *audio, size_t group_id, float pan)
{
    ma_mutex_lock(&audio->driver.lock);
    SL_context_set_pan(audio->context, group_id, pan);
    ma_mutex_unlock(&audio->driver.lock);
}

void Audio_set_balance(Audio_t *audio, size_t group_id, float balance)
{
    ma_mutex_lock(&audio->driver.lock);
    SL_context_set_balance(audio->context, group_id, balance);
    ma_mutex_unlock(&audio->driver.lock);
}

void Audio_set_gain(Audio_t *audio, size_t group_id, float gain)
{
    ma_mutex_lock(&audio->driver.lock);
    SL_context_set_gain(audio->context, group_id, gain);
    ma_mutex_unlock(&audio->driver.lock);
}

float Audio_get_volume(const Audio_t *audio)
{
//    ma_mutex_lock(&audio->driver.lock);
    float volume;
    ma_result result = ma_device_get_master_volume((ma_device *)&audio->driver.device, &volume);
    if (result != MA_SUCCESS) {
        return 0.0f;
    }
    return volume;
//    ma_mutex_unlock(&audio->driver.lock);
}

SL_Mix_t Audio_get_mix(const Audio_t *audio, size_t group_id)
{
    ma_mutex_lock((ma_mutex *)&audio->driver.lock);
    SL_Mix_t mix = SL_context_get_group(audio->context, group_id)->mix;
    ma_mutex_unlock((ma_mutex *)&audio->driver.lock);
    return mix;
}

float Audio_get_gain(const Audio_t *audio, size_t group_id)
{
    ma_mutex_lock((ma_mutex *)&audio->driver.lock);
    float gain = SL_context_get_group(audio->context, group_id)->gain;
    ma_mutex_unlock((ma_mutex *)&audio->driver.lock);
    return gain;
}

void Audio_track(Audio_t *audio, SL_Source_t *source, bool reset)
{
    ma_mutex_lock(&audio->driver.lock);
    if (reset) {
        bool success = SL_source_reset(source);
        Log_assert(success, LOG_LEVELS_WARNING, LOG_CONTEXT, "can't reset source %p", source);
    }
    if (!SL_context_is_tracked(audio->context, source)) {
        SL_context_track(audio->context, source);
    }
    ma_mutex_unlock(&audio->driver.lock);
}

void Audio_untrack(Audio_t *audio, SL_Source_t *source)
{
    ma_mutex_lock(&audio->driver.lock);
    if (SL_context_is_tracked(audio->context, source)) {
        SL_context_untrack(audio->context, source);
    }
    ma_mutex_unlock(&audio->driver.lock);
}

bool Audio_is_tracked(const Audio_t *audio, SL_Source_t *source)
{
    ma_mutex_lock((ma_mutex *)&audio->driver.lock);
    bool is_tracked = SL_context_is_tracked(audio->context, source);
    ma_mutex_unlock((ma_mutex *)&audio->driver.lock);
    return is_tracked;
}

bool Audio_update(Audio_t *audio, float delta_time)
{
    ma_mutex_lock(&audio->driver.lock);
    bool updated = SL_context_update(audio->context, delta_time);
#ifdef __AUDIO_START_AND_STOP__
    size_t count = SL_context_count_tracked(audio->context);
#endif
    ma_mutex_unlock(&audio->driver.lock);

    if (!updated) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't update context");
        return false;
    }

#ifdef __AUDIO_START_AND_STOP__
    const bool is_started = ma_device_is_started(&audio->driver.device);
    if (count == 0 && is_started) {
        audio->grace -= delta_time;
        if (audio->grace <= 0.0) {
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "no more sources and grace period elapsed, stopping device");
            ma_result result = ma_device_stop(&audio->driver.device);
            if (result != MA_SUCCESS) {
                Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't stop the audio device");
                return false;
            }
        }
    } else
    if (count > 0) {
        audio->grace = __AUDIO_START_AND_STOP_GRACE_PERIOD__;
        if (!is_started) {
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%d incoming source(s), starting device", count);
            ma_result result = ma_device_start(&audio->driver.device);
            if (result != MA_SUCCESS) {
                Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't start the audio device");
                return false;
            }
        }
    }
#endif

    return true;
}
