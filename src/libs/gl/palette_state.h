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
#include <core/platform.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct GL_Palette_State_s {
    GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS]; // Remap a color into another

    bool transparent[GL_MAX_PALETTE_COLORS]; // Transparency flag per color

    // The LUT map for fast access during drawing. It combines shifting
    // and transparency:
    // 0x0000 = SKIP
    // 0x0100 | index = WRITE idx (index into lower 8 bits)
    uint16_t map[GL_MAX_PALETTE_COLORS];
} GL_Palette_State_t;

#define GL_PALETTE_TRANSPARENT_MASK     0xFF00
#define GL_PALETTE_SHIFTING_MASK        0x00FF

#define GL_PALETTE_SKIP                 0x0000

extern void gl_palette_state_init(GL_Palette_State_t *state);
extern void gl_palette_state_shifting(GL_Palette_State_t *state, GL_Pixel_t from, GL_Pixel_t to);
extern void gl_palette_state_transparent(GL_Palette_State_t *state, GL_Pixel_t index, bool is_transparent);
extern void gl_palette_state_reset(GL_Palette_State_t *state);

#endif  /* TOFU_LIBS_GL_PALETTE_STATE_H */
