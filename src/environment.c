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

#include "environment.h"

#include <stdlib.h>
#include <string.h>

void Environment_initialize(Environment_t *environment, const char *base_path, int width, int height)
{
    strcpy(environment->base_path, base_path);
    environment->should_close = false;

    Graphics_t *graphics = &environment->graphics;
    *graphics = (Graphics_t){
            .width = width,
            .height = height
        };

    for (size_t i = 0; i < MAX_PALETTE_COLORS; ++i) {
        unsigned char v = ((float)i / (float)(MAX_PALETTE_COLORS - 1)) * 255;
        Color *color = &graphics->palette[i];
        *color = (Color){ v, v, v, 255 };
    }

    for (size_t i = 0; i < MAX_GRAPHIC_BANKS; ++i) {
        Bank_t *bank = &graphics->banks[i];
        *bank = (Bank_t){};
    }
}

void Environment_terminate(Environment_t *environment)
{
    Graphics_t *graphics = &environment->graphics;

    for (size_t i = 0; i < MAX_GRAPHIC_BANKS; ++i) {
        Bank_t *bank = &graphics->banks[i];
        if (bank->atlas.id > 0) {
            UnloadTexture(bank->atlas);
            bank->atlas.id = 0;
        }
    }
}
