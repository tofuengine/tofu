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

#include "sheets.h"

#include <strings.h>

typedef struct _Resource_Sheet_t {
    const char *id;
    Sheet_Data_t data;
} Resource_Sheet_t;

static const unsigned char _spleen_5x8_pixels[] = {
#include <spleen/spleen-5x8.inc>
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

static const Resource_Sheet_t _sheets[] = {
    { "5x8", { 475, 8, _spleen_5x8_pixels, 5, 8 } },
    { "8x16", { 760, 16, _spleen_8x16_pixels, 8, 16 } },
    { "12x24", { 1140, 24, _spleen_12x24_pixels, 12,24 } },
    { "16x32", { 1520, 32, _spleen_16x32_pixels, 16, 32 } },
    { "32x64", { 3040, 64, _spleen_32x64_pixels, 32, 64 } },
    { NULL, { 0 } }
};

const Sheet_Data_t *resources_sheets_find(const char *id)
{
    for (const Resource_Sheet_t *sheet = _sheets; sheet->id != NULL; ++sheet) {
        if (strcasecmp(sheet->id, id) == 0) {
            return &sheet->data;
        }
    }
    return NULL;
}
