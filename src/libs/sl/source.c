/*
 * MIT License
 *
 * Copyright (c) 2019-2022 Marco Lizza
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

#include "internals.h"

#include <libs/mumalloc.h>

void SL_source_destroy(SL_Source_t *source)
{
    source->vtable.dtor(source);
    mu_free(source);
}

bool SL_source_reset(SL_Source_t *source)
{
    return source->vtable.reset(source);
}

void SL_source_set_group(SL_Source_t *source, size_t group_id)
{
    SL_props_set_group(source->props, group_id);
}

void SL_source_set_looped(SL_Source_t *source, bool looped)
{
    SL_props_set_looped(source->props, looped);
}

void SL_source_set_mix(SL_Source_t *source, SL_Mix_t mix)
{
    SL_props_set_mix(source->props, mix);
}

void SL_source_set_pan(SL_Source_t *source, float pan)
{
    SL_props_set_pan(source->props, pan);
}

void SL_source_set_twin_pan(SL_Source_t *source, float left_pan, float right_pan)
{
    SL_props_set_twin_pan(source->props, left_pan, right_pan);
}

void SL_source_set_balance(SL_Source_t *source, float balance)
{
    SL_props_set_balance(source->props, balance);
}

void SL_source_set_gain(SL_Source_t *source, float gain)
{
    SL_props_set_gain(source->props, gain);
}

void SL_source_set_speed(SL_Source_t *source, float speed)
{
    SL_props_set_speed(source->props, speed);
}

size_t SL_source_get_group(const SL_Source_t *source)
{
    return source->props->group_id;
}

bool SL_source_get_looped(const SL_Source_t *source)
{
    return source->props->looped;
}

SL_Mix_t SL_source_get_mix(const SL_Source_t *source)
{
    return source->props->mix;
}

float SL_source_get_gain(const SL_Source_t *source)
{
    return source->props->gain;
}

float SL_source_get_speed(const SL_Source_t *source)
{
    return source->props->speed;
}

void SL_source_on_group_changed(SL_Source_t *source, size_t group_id)
{
    SL_props_on_group_changed(source->props, group_id);
}
