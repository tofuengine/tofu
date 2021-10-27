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

#include "resolution.h"

#include <string.h>

typedef struct Resolution_Variant_t {
    const char *id;
    Resolution_t resolution;
} Resolution_Variant_t;

static const Resolution_Variant_t _resolutions[] = {
    // Commodore
    { "commodore-c64", { 320, 200 } },
    { "commodore-amiga-pal", { 320, 200 } },
    { "commodore-amiga-ntsc", { 320, 256 } },
    // Arcade
    { "arcade-cps", { 384, 224 } },
    // Atari
    { "atari-vcs2600", { 160, 192 } },
    // Nintendo
    { "nintendo-nes", { 256, 240 } },
    { "nintendo-snes", { 256, 224 } },
    { "nintendo-gb", { 160, 144 } },
    { "nintendo-gba", { 240, 160 } },
    { "nintendo-ds", { 400, 240 } },
    // Sony
    { "sony-psp", { 480, 272 } },
    // Others
    { "default", { 320, 180 } },
    { "pico8", { 128, 128 } },
    // Generic VGA
    { "vga-mode13h", { 320, 200 } },
    { "vga-modex", { 320, 240 } },
    { "vga-qvga", { 320, 240 } },
    { "vga-wqvga", { 400, 240 } },
    { "vga-hvga", { 480, 320 } },
    { "vga-vga", { 640, 480 } },
    { NULL, { 0, 0 } }
};

const Resolution_t *Resolution_find(const char *id)
{
    for (size_t i = 0; _resolutions[i].id; ++i) {
        if (strcasecmp(id, _resolutions[i].id) == 0) {
            return &_resolutions[i].resolution;
        }
    }
    return NULL;
}
