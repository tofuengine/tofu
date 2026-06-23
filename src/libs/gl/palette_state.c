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

#include "palette_state.h"

#define _LOG_TAG "gl-palette_state"
#include <libs/log.h>

void gl_palette_state_init(GL_Palette_State_t *state)
{
    *state = (GL_Palette_State_t){ 0 };

    gl_palette_state_reset(state);
    gl_palette_state_bank(state, GL_PALETTE_DEFAULT_BANK);
}

void gl_palette_state_shifting(GL_Palette_State_t *state, GL_Pixel_t from, GL_Pixel_t to)
{
    if (from > GL_PALETTE_LAST_INDEX || to > GL_PALETTE_LAST_INDEX) { // Protect the upper half of the map.
        LOG_W("palette index out of range (from: %u, to: %u), skipping shifting", from, to);
        return;
    }
    // Always limit the access to the map for the actually used colors. The
    // upper half of the map is reserved for transparent pixels (as they have
    // the MSB set).
#if defined(TOFU_GRAPHICS_PALETTE_BANK_SYNC)
    for (size_t i = 0; i < GL_PALETTE_MAX_BANKS; ++i) {
        uint16_t *map = state->map[i];
#else
        uint16_t *map = state->map[state->bank];
#endif
        map[from] = (map[from] & ~GL_PALETTE_SHIFTING_MASK) | to;
#if defined(TOFU_GRAPHICS_PALETTE_BANK_SYNC)
    }
#endif
}

void gl_palette_state_transparent(GL_Palette_State_t *state, GL_Pixel_t index, bool is_transparent)
{
    if (index > GL_PALETTE_LAST_INDEX) { // Ditto.
        LOG_W("palette index %u out of range, skipping transparency setting", index);
        return;
    }
#if defined(TOFU_GRAPHICS_PALETTE_BANK_SYNC)
    for (size_t i = 0; i < GL_PALETTE_MAX_BANKS; ++i) {
        uint16_t *map = state->map[i];
#else
        uint16_t *map = state->map[state->bank];
#endif
        if (is_transparent) { // We operate on the single flag, leaving the shifting part of the map entry unchanged.
            map[index] |= GL_PALETTE_FLAG_TRANSPARENCY;
        } else {
            map[index] &= ~GL_PALETTE_FLAG_TRANSPARENCY;
        }
#if defined(TOFU_GRAPHICS_PALETTE_BANK_SYNC)
    }
#endif
}

void gl_palette_state_bank(GL_Palette_State_t *state, int bank)
{
    state->bank = bank; // Well, this way bank selection is actually `O(1)` :)
}

void gl_palette_state_reset(GL_Palette_State_t *state)
{
    // Colors in the upper half of the map are reserved for transparent pixels
    // (as they have the MSB set).
    for (size_t i = 0; i < GL_PALETTE_MAX_BANKS; ++i) {
        uint8_t bank = i;
        uint16_t *map = state->map[i];

        for (size_t j = 0; j < GL_PALETTE_AVAILABLE_COLORS; ++j) {
            uint8_t index = j & GL_PIXEL_INDEX_MASK;

            uint8_t packed = (bank << GL_PALETTE_BANK_SHIFT) | index;

            // To be precise, the shifting part of the map entry is not relevant
            // for transparent pixels as the transparency flag is set. However,
            // we set it to the original color index just for consistency and
            // debugging purposes (e.g. when dumping the palette state).
            map[j] = (j > GL_PALETTE_LAST_INDEX)
                ? GL_PALETTE_FLAG_TRANSPARENCY | packed
                : packed;
        }
    }
}
