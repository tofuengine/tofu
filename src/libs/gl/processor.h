/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2026 Marco Lizza
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

#ifndef TOFU_LIBS_GL_PROCESSOR_H
#define TOFU_LIBS_GL_PROCESSOR_H

#include "common.h"
#include "palette.h"
#include "program.h"
#include "surface.h"

// During the blitting phase, we combine the pixel index with the palette bank
// to get the actual color index. So, if we have `N` banks, each with `M`
// colors, we can have up to `N * M` colors in the palette. However, the amount
// of color index is still `2^8`, so we need to limit the amount of colors per
// bank to `2^8 / N`. For example, if we have 4 banks, we can have up to 64
// colors per bank, for a total of 256 colors in the palette.
#define GL_PROCESSOR_MAX_COLORS (GL_PALETTE_MAX_COLORS * GL_PALETTE_MAX_BANKS)

typedef struct GL_Processor_State_s {
    GL_Color_t palette[GL_PROCESSOR_MAX_COLORS]; // TODO: combine the shift-palette into a single table? Is shift even worth?
    GL_Pixel_t shifting[GL_PROCESSOR_MAX_COLORS];
    GL_Program_t *program;
} GL_Processor_State_t;

typedef void (*GL_Processor_Surface_To_Pixels_t)(const GL_Processor_State_t *state, const GL_Surface_t *surface, GL_Color_t *pixels);

typedef struct GL_Processor_s {
    GL_Processor_State_t state;
    GL_Processor_Surface_To_Pixels_t surface_to_pixels;
} GL_Processor_t;

extern GL_Processor_t *GL_processor_create(void);
extern void GL_processor_destroy(GL_Processor_t *processor);

extern void GL_processor_reset(GL_Processor_t *processor);

extern const GL_Color_t *GL_processor_get_palette(const GL_Processor_t *processor);

extern void GL_processor_set_palette(GL_Processor_t *processor, const GL_Color_t *palette, size_t bank);
extern void GL_processor_set_shifting(GL_Processor_t *processor, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count);
extern void GL_processor_set_program(GL_Processor_t *processor, const GL_Program_t *program);

extern void GL_processor_surface_to_pixels(const GL_Processor_t *processor, const GL_Surface_t *surface, GL_Color_t *pixels);

#endif  /* TOFU_LIBS_GL_PROCESSOR_H */
