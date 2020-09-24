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

#include "source.h"

#include "internals.h"
#include "props.h"

#include <libs/stb.h>

void SL_source_destroy(SL_Source_t *source)
{
    ((Source_t *)source)->vtable.dtor(source);
    free(source);
}

bool SL_source_reset(SL_Source_t *source)
{
    return ((Source_t *)source)->vtable.reset(source);
}

size_t SL_source_get_group(SL_Source_t *source)
{
    return ((Source_t *)source)->props.group_id;
}

bool SL_source_get_looping(SL_Source_t *source)
{
    return ((Source_t *)source)->props.looping;
}

SL_Mix_t SL_source_get_mix(SL_Source_t *source)
{
    return ((Source_t *)source)->props.mix;
}

float SL_source_get_gain(SL_Source_t *source)
{
    return ((Source_t *)source)->props.gain;
}

float SL_source_get_speed(SL_Source_t *source)
{
    return ((Source_t *)source)->props.speed;
}

void SL_source_set_group(SL_Source_t *source, size_t group_id)
{
    SL_props_group(&((Source_t *)source)->props, group_id);
}

void SL_source_set_looping(SL_Source_t *source, bool looping)
{
    SL_props_looping(&((Source_t *)source)->props, looping);
}

void SL_source_set_mix(SL_Source_t *source, SL_Mix_t mix)
{
    SL_props_mix(&((Source_t *)source)->props, mix);
}

void SL_source_set_pan(SL_Source_t *source, float pan)
{
    SL_props_pan(&((Source_t *)source)->props, pan);
}

void SL_source_set_balance(SL_Source_t *source, float balance)
{
    SL_props_balance(&((Source_t *)source)->props, balance);
}

void SL_source_set_gain(SL_Source_t *source, float gain)
{
    SL_props_gain(&((Source_t *)source)->props, gain);
}

void SL_source_set_speed(SL_Source_t *source, float speed)
{
    SL_props_speed(&((Source_t *)source)->props, speed);
}

void SL_source_on_group_changed(SL_Source_t *source, size_t group_id)
{
    SL_props_on_group_changed(&((Source_t *)source)->props, group_id);
}
