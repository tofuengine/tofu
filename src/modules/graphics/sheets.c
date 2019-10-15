/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "sheets.h"

#include <spleen/spleen.h>

#include <strings.h>

typedef struct _Predefined_Sheet_t {
    const char *id;
    Sheet_Data_t data;
} Predefined_Sheet_t;

static const Predefined_Sheet_t _sheets[] = {
    { "5x8", { spleen_5x8_png, spleen_5x8_png_len, 5, 8 } },
    { "8x16", { spleen_8x16_png, spleen_8x16_png_len, 8, 16 } },
    { "12x24", { spleen_12x24_png, spleen_12x24_png_len, 12,24 } },
    { "16x32", { spleen_16x32_png, spleen_16x32_png_len, 16, 32 } },
    { "32x64", { spleen_32x64_png, spleen_32x64_png_len, 32, 64 } },
    { NULL, { } }
};

const Sheet_Data_t *graphics_sheets_find(const char *id)
{
    for (const Predefined_Sheet_t *sheet = _sheets; sheet->id != NULL; ++sheet) {
        if (strcasecmp(sheet->id, id) == 0) {
            return &sheet->data;
        }
    }
    return NULL;
}
