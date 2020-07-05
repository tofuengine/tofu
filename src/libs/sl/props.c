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

#if SL_BYTES_PER_FRAME == 2
  #define INTERNAL_FORMAT   ma_format_s16
#elif SL_BYTES_PER_FRAME == 4
  #define INTERNAL_FORMAT   ma_format_f32
#else
  #error Wrong internal format.
#endif

#define LOG_CONTEXT "sl-props"

bool SL_props_init(SL_Props_t *props, ma_format format, ma_uint32 sample_rate, ma_uint32 channels_in, ma_uint32 channels_out)
{
    *props = (SL_Props_t){
            .group = 0,
            .looping = false,
            .gain = 1.0,
            .pan = 0.0f,
            .speed = 1.0f,
            .mix = mix_precompute_pan(0.0f, 1.0f)
        };

    ma_data_converter_config config = ma_data_converter_config_init(format, INTERNAL_FORMAT, channels_in, channels_out, sample_rate, SL_FRAMES_PER_SECOND);
    config.resampling.allowDynamicSampleRate = MA_TRUE; // required for speed throttling
    ma_result result = ma_data_converter_init(&config, &props->converter);
    if (result != MA_SUCCESS) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "failed to create data converter");
        return false;
    }

    return true;
}

void SL_props_deinit(SL_Props_t *props)
{
    ma_data_converter_uninit(&props->converter);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "data converted deinitialized");
}

void SL_props_group(SL_Props_t *props, size_t group)
{
    props->group = group;
}

void SL_props_looping(SL_Props_t *props, bool looping)
{
    props->looping = looping;
}

void SL_props_gain(SL_Props_t *props, float gain)
{
    props->gain = fmaxf(0.0f, gain);
    props->mix = mix_precompute_pan(props->pan, props->gain);
}

void SL_props_pan(SL_Props_t *props, float pan)
{
    props->pan = fmaxf(-1.0f, fminf(pan, 1.0f));
    props->mix = mix_precompute_pan(props->pan, props->gain);
}

void SL_props_speed(SL_Props_t *props, float speed)
{
    props->speed = fmaxf(MIN_SPEED_VALUE, speed);
    ma_data_converter_set_rate_ratio(&props->converter, props->speed); // The ratio is `in` over `out`, i.e. actual speed-up factor.
}

SL_Mix_t SL_props_precompute(SL_Props_t *props, const SL_Mix_t *mixes)
{
    return (SL_Mix_t){
            .left = props->mix.left * mixes[props->group].left,
            .right = props->mix.right * mixes[props->group].right
        };
}
