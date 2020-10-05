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

#include "props.h"

#include "mix.h"

#include <config.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <math.h>

// Being the speed implemented by dynamic resampling, there's an intrinsic theoretical limit given by the ratio
// between the minimum (8KHz) and the maximum (384KHz) supported sample rates.
#define MIN_SPEED_VALUE ((float)MA_MIN_SAMPLE_RATE / (float)MA_MAX_SAMPLE_RATE)

#if SL_BYTES_PER_SAMPLE == 2
  #define INTERNAL_FORMAT   ma_format_s16
#elif SL_BYTES_PER_SAMPLE == 4
  #define INTERNAL_FORMAT   ma_format_f32
#else
  #error Wrong internal format.
#endif

#define LOG_CONTEXT "sl-props"

static void _precompute(SL_Props_t *props);

SL_Props_t *SL_props_create(const SL_Context_t *context, ma_format format, ma_uint32 sample_rate, ma_uint32 channels_in, ma_uint32 channels_out)
{
    SL_Props_t *props = malloc(sizeof(SL_Props_t));
    if (!props) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate properties");
        return NULL;
    }

    *props = (SL_Props_t){
            .context = context,
            .channels = channels_in,
            .group_id = SL_DEFAULT_GROUP,
            .looped = false,
            .mix = channels_in == 1 ? mix_pan(0.0f) : mix_balance(0.0f), // mono -> center panned, stereo -> separated
            .gain = 1.0,
            .speed = 1.0f
        };

    ma_data_converter_config config = ma_data_converter_config_init(format, INTERNAL_FORMAT, channels_in, channels_out, sample_rate, SL_FRAMES_PER_SECOND);
    config.resampling.allowDynamicSampleRate = MA_TRUE; // required for speed throttling
    ma_result result = ma_data_converter_init(&config, &props->converter);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "failed to create data converter");
        free(props);
        return NULL;
    }

    return props;
}

void SL_props_destroy(SL_Props_t *props)
{
    ma_data_converter_uninit(&props->converter);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "data converted deinitialized");

    free(props);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "properties freed");
}

void SL_props_set_group(SL_Props_t *props, size_t group_id)
{
    props->group_id = group_id;
}

void SL_props_set_looped(SL_Props_t *props, bool looped)
{
    props->looped = looped;
}

// mix, pan, and balance are mutually exclusive, that is pan is a special case of mix.
void SL_props_set_mix(SL_Props_t *props, SL_Mix_t mix)
{
    props->mix = mix;
    _precompute(props);
}

void SL_props_set_pan(SL_Props_t *props, float pan)
{
    props->mix = props->channels == 1
        ? mix_pan(fmaxf(-1.0f, fminf(pan, 1.0f)))
        : mix_twin_pan(fmaxf(-1.0f, fminf(pan, 1.0f)), fmaxf(-1.0f, fminf(pan, 1.0f)));
    _precompute(props);
}

void SL_props_set_twin_pan(SL_Props_t *props, float left_pan, float right_pan)
{
    props->mix = mix_twin_pan(fmaxf(-1.0f, fminf(left_pan, 1.0f)), fmaxf(-1.0f, fminf(right_pan, 1.0f)));
    _precompute(props);
}

void SL_props_set_balance(SL_Props_t *props, float balance)
{
    props->mix = mix_balance(fmaxf(-1.0f, fminf(balance, 1.0f)));
    _precompute(props);
}

void SL_props_set_gain(SL_Props_t *props, float gain)
{
    props->gain = fmaxf(0.0f, gain);
    _precompute(props);
}

void SL_props_set_speed(SL_Props_t *props, float speed)
{
    props->speed = fmaxf(MIN_SPEED_VALUE, speed);
    ma_data_converter_set_rate_ratio(&props->converter, props->speed); // The ratio is `in` over `out`, i.e. actual speed-up factor.
}

void SL_props_on_group_changed(SL_Props_t *props, size_t group_id)
{
    if (props->group_id != group_id && group_id != SL_ANY_GROUP) {
        return;
    }
    _precompute(props);
}

//
// Sm * v = u -> Gm * u'
//
//   or
//
// Gm * Sm * v = (Gm * Sm) * v = GSm * v
//
static void _precompute(SL_Props_t *props)
{
    const SL_Group_t *group = SL_context_get_group(props->context, props->group_id);

    const SL_Mix_t S = props->mix;
    const SL_Mix_t G = group->mix;

    const float left_to_left = S.left_to_left * G.left_to_left + S.right_to_left * G.left_to_right;
    const float left_to_right = S.left_to_right * G.left_to_left + S.right_to_right * G.left_to_right;
    const float right_to_left = S.left_to_left * G.right_to_left + S.right_to_left * G.right_to_right;
    const float right_to_right = S.left_to_right * G.right_to_left + S.right_to_right * G.right_to_right;

    const float S_gain = props->gain;
    const float G_gain = group->gain;
    const float gain = S_gain * G_gain;

    props->precomputed_mix = (SL_Mix_t){
            .left_to_left = left_to_left * gain,
            .left_to_right = left_to_right * gain,
            .right_to_left = right_to_left * gain,
            .right_to_right = right_to_right * gain
        };
}

