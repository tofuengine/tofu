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

#include "images.h"

#include <strings.h>

typedef struct _Resource_Image_t {
    const char *id;
    Image_t data;
} Resource_Image_t;

static const unsigned char _spleen_5x8_pixels[] = {
#include <spleen/spleen-5x8.inc>
};
static const unsigned char _spleen_6x12_pixels[] = {
#include <spleen/spleen-6x12.inc>
};
static const unsigned char _spleen_8x16_pixels[] = {
#include <spleen/spleen-8x16.inc>
};
static const unsigned char _spleen_12x24_pixels[] = {
#include <spleen/spleen-12x24.inc>
};
static const unsigned char _spleen_16x32_pixels[] = {
#include <spleen/spleen-16x32.inc>
};
static const unsigned char _spleen_32x64_pixels[] = {
#include <spleen/spleen-32x64.inc>
};

static const Resource_Image_t _images[] = {
    { "5x8", { 475, 8, _spleen_5x8_pixels } },
    { "6x12", { 570, 12, _spleen_6x12_pixels } },
    { "8x16", { 760, 16, _spleen_8x16_pixels } },
    { "12x24", { 1140, 24, _spleen_12x24_pixels } },
    { "16x32", { 1520, 32, _spleen_16x32_pixels } },
    { "32x64", { 3040, 64, _spleen_32x64_pixels } },
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

bool resources_images_exists(const char *id)
{
    for (const Resource_Image_t *image = _images; image->id != NULL; ++image) {
        if (strcasecmp(image->id, id) == 0) {
            return true;
        }
    }
    return false;
}
