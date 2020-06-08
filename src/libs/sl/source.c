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

void SL_source_destroy(SL_Source_t *source)
{
    ((Source_t *)source)->vtable.dtor(source);
}

void SL_source_reset(SL_Source_t *source)
{
    ((Source_t *)source)->vtable.reset(source);
}

size_t SL_source_get_group(SL_Source_t *source)
{
    return ((Source_t *)source)->props.group;
}

bool SL_source_get_looping(SL_Source_t *source)
{
    return ((Source_t *)source)->props.looping;
}

float SL_source_get_gain(SL_Source_t *source)
{
    return ((Source_t *)source)->props.gain;
}

float SL_source_get_pan(SL_Source_t *source)
{
    return ((Source_t *)source)->props.pan;
}

float SL_source_get_speed(SL_Source_t *source)
{
    return ((Source_t *)source)->props.speed;
}

void SL_source_set_group(SL_Source_t *source, size_t group)
{
    SL_props_group(&((Source_t *)source)->props, group);
}

void SL_source_set_looping(SL_Source_t *source, bool looping)
{
    SL_props_looping(&((Source_t *)source)->props, looping);
}

void SL_source_set_gain(SL_Source_t *source, float gain)
{
    SL_props_gain(&((Source_t *)source)->props, gain);
}

void SL_source_set_pan(SL_Source_t *source, float pan)
{
    SL_props_pan(&((Source_t *)source)->props, pan);
}

void SL_source_set_speed(SL_Source_t *source, float speed)
{
    SL_props_speed(&((Source_t *)source)->props, speed);
}
