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

#include "blobs.h"

#include <stdint.h>
#include <strings.h>

typedef struct _Resource_Blob_t {
    const char *id;
    Blob_t data;
} Resource_Blob_t;

static const uint8_t _gamecontrollerdb[] = {
#include <assets/gamecontrollerdb.inc>
    0x00
};
static const uint8_t _crt_pi_vertical[] = {
#include <assets/shaders/crt-pi-vertical.inc>
    0x00
};
static const uint8_t _crt_pi[] = {
#include <assets/shaders/crt-pi.inc>
    0x00
};
static const uint8_t _zfast_crt_vertical[] = {
#include <assets/shaders/zfast-crt-vertical.inc>
    0x00
};
static const uint8_t _zfast_crt[] = {
#include <assets/shaders/zfast-crt.inc>
    0x00
};
static const uint8_t _zfast_lcd[] = {
#include <assets/shaders/zfast-lcd.inc>
    0x00
};
static const uint8_t _scanline_fract[] = {
#include <assets/shaders/scanline-fract.inc>
    0x00
};
static const uint8_t _scanlines_sine_abs[] = {
#include <assets/shaders/scanlines-sine-abs.inc>
    0x00
};
static const uint8_t _color_acromatopsia[] = {
#include <assets/shaders/color-acromatopsia.inc>
    0x00
};
static const uint8_t _color_deuteranopia[] = {
#include <assets/shaders/color-deuteranopia.inc>
    0x00
};
static const uint8_t _color_protanopia[] = {
#include <assets/shaders/color-protanopia.inc>
    0x00
};
static const uint8_t _color_tritanopia[] = {
#include <assets/shaders/color-tritanopia.inc>
    0x00
};

static const Resource_Blob_t _blobs[] = {
    { "gamecontrollerdb", { _gamecontrollerdb, sizeof(_gamecontrollerdb) } },
    { "crt-pi-vertical", { _crt_pi_vertical, sizeof(_crt_pi_vertical) } },
    { "crt-pi", { _crt_pi, sizeof(_crt_pi) } },
    { "zfast-crt-vertical", { _zfast_crt_vertical, sizeof(_zfast_crt_vertical) } },
    { "zfast-crt", { _zfast_crt, sizeof(_zfast_crt) } },
    { "zfast-lcd", { _zfast_lcd, sizeof(_zfast_lcd) } },
    { "scanline-fract", { _scanline_fract, sizeof(_scanline_fract) } },
    { "scanlines-sine-abs", { _scanlines_sine_abs, sizeof(_scanlines_sine_abs) } },
    { "color-acromatopsia", { _color_acromatopsia, sizeof(_color_acromatopsia) } },
    { "color-deuteranopia", { _color_deuteranopia, sizeof(_color_deuteranopia) } },
    { "color-protanopia", { _color_protanopia, sizeof(_color_protanopia) } },
    { "color-tritanopia", { _color_tritanopia, sizeof(_color_tritanopia) } },
    { NULL, { 0 } }
};

const Blob_t *resources_blobs_find(const char *id)
{
    for (const Resource_Blob_t *blob = _blobs; blob->id != NULL; ++blob) {
        if (strcasecmp(blob->id, id) == 0) {
            return &blob->data;
        }
    }
    return NULL;
}

bool resources_blobs_exists(const char *id)
{
    for (const Resource_Blob_t *blob = _blobs; blob->id != NULL; ++blob) {
        if (strcasecmp(blob->id, id) == 0) {
            return true;
        }
    }
    return false;
}
