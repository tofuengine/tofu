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

#ifndef TOFU_LIBS_GL_PALETTE_STATE_H
#define TOFU_LIBS_GL_PALETTE_STATE_H

#include "common.h"

#include <core/config.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct GL_Palette_State_s {
    // This LUT dictates how to treat each palette index during drawing.
    // It is a precomputed combination of `flags` and `shifting` into a single
    // `uint8_t` value. The idea is to have a single look-up and avoid
    // multiple memory accesses per pixel during drawing with a single access.
    //
    // The layout is as follows:
    //   - 0x80 (MSB): transparency flag
    //   - 0x7F (LSB): shifted color index (0-127)
    //
    // The optimization goes even further as the upper half of the map
    // (128-255) is reserved for transparent pixels (as they have the MSB
    // set). This allows us to determine if a pixel is transparent just with
    // a single LUT access (and not by testing the alpha-bit).
    uint8_t map[GL_PALETTE_AVAILABLE_COLORS];
} GL_Palette_State_t;

#define GL_PALETTE_FLAGS_MASK    0x80
#define GL_PALETTE_SHIFTING_MASK 0x7F

#define GL_PALETTE_FLAG_TRANSPARENCY 0x80

#define GL_PALETTE_IS_TRANSPARENT(entry) \
    (((entry) & GL_PALETTE_FLAG_TRANSPARENCY) != 0)

#define GL_PALETTE_GET_SHIFTING(entry) \
    ((GL_Pixel_t)(entry & GL_PALETTE_SHIFTING_MASK))

extern void gl_palette_state_init(GL_Palette_State_t *state);
extern void gl_palette_state_shifting(GL_Palette_State_t *state, GL_Pixel_t from, GL_Pixel_t to);
extern void gl_palette_state_transparent(GL_Palette_State_t *state, GL_Pixel_t index, bool is_transparent);
extern void gl_palette_state_reset(GL_Palette_State_t *state);

#endif  /* TOFU_LIBS_GL_PALETTE_STATE_H */
