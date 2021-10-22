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

#include "images.h"

#include <stdint.h>
#include <strings.h>

typedef struct Resource_Image_s {
    const char *id;
    Image_t data;
} Resource_Image_t;

static const uint8_t _icon[] = {
#include <assets/images/icon.inc>
};
static const uint8_t _icon_bw[] = {
#include <assets/images/icon-bw.inc>
};
static const uint8_t _spleen_5x8_pixels[] = {
#include <spleen/spleen-5x8.inc>
};
static const uint8_t _spleen_6x12_pixels[] = {
#include <spleen/spleen-6x12.inc>
};
static const uint8_t _spleen_8x16_pixels[] = {
#include <spleen/spleen-8x16.inc>
};
static const uint8_t _spleen_12x24_pixels[] = {
#include <spleen/spleen-12x24.inc>
};
static const uint8_t _spleen_16x32_pixels[] = {
#include <spleen/spleen-16x32.inc>
};
static const uint8_t _spleen_32x64_pixels[] = {
#include <spleen/spleen-32x64.inc>
};

static const Resource_Image_t _images[] = {
    { "icon.png", { 64, 64, _icon } },
    { "icon-bw.png", { 64, 64, _icon_bw } },
    { "5x8.png", { 475, 8, _spleen_5x8_pixels } },
    { "6x12.png", { 570, 12, _spleen_6x12_pixels } },
    { "8x16.png", { 760, 16, _spleen_8x16_pixels } },
    { "12x24.png", { 1140, 24, _spleen_12x24_pixels } },
    { "16x32.png", { 1520, 32, _spleen_16x32_pixels } },
    { "32x64.png", { 3040, 64, _spleen_32x64_pixels } },
    { NULL, { 0 } }
};

const Image_t *resources_images_find(const char *id)
{
    for (const Resource_Image_t *image = _images; image->id != NULL; ++image) {
        if (strcasecmp(image->id, id) == 0) {
            return &image->data;
        }
    }
    return NULL;
}
