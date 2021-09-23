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

static float _perlin(float x, float y, float z)
{
    return noise3(x, y, z);
}

static float _simplex(float x, float y, float z)
{
    return snoise3(x, y, z);
}

static const Noise_t _entries[] = {
    { "perlin", _perlin },
    { "simplex", _simplex },
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
