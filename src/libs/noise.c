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

#include "noise.h"

#include <perlin-noise/noise1234.h>
#include <perlin-noise/simplexnoise1234.h>

#include <string.h>

static float _perlin1d(float x, float y, float z)
{
    return noise1(x);
}

static float _perlin2d(float x, float y, float z)
{
    return noise2(x, y);
}

static float _perlin3d(float x, float y, float z)
{
    return noise3(x, y, z);
}

static float _simplex1d(float x, float y, float z)
{
    return snoise1(x);
}

static float _simplex2d(float x, float y, float z)
{
    return snoise2(x, y);
}

static float _simplex3d(float x, float y, float z)
{
    return snoise3(x, y, z);
}

static const Noise_t _entries[] = {
    { "perlin-1d", _perlin1d },
    { "perlin-2d", _perlin2d },
    { "perlin-3d", _perlin3d },
    { "simplex-1d", _simplex1d },
    { "simplex-2d", _simplex2d },
    { "simplex-3d", _simplex3d },
    { NULL, NULL }
};

const Noise_t *noise_from_type(const char *type)
{
    for (const Noise_t *entry = _entries; entry->type; ++entry) {
        if (strcasecmp(entry->type, type) == 0) {
            return entry;
        }
    }
    return NULL;
}
