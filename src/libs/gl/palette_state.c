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

#define _WRITE_PIXEL 0x0100

static inline void _compute_map_entry(GL_Palette_State_t *state, GL_Pixel_t index)
{
#if defined(TOFU_GRAPHICS_PALETTE_SHITFING_AWARE_TRANSPARENCY)
    GL_Pixel_t to = state->shifting[index]; // We test the transparency on the target color!
    if (state->transparent[to]) {
        state->map[index] = GL_PALETTE_SKIP;
    } else {
        state->map[index] = (uint16_t)(_WRITE_PIXEL | to);
    }
#else
    state->map[index] = state->transparent[index]
        ? GL_PALETTE_SKIP
        : (uint16_t)(_WRITE_PIXEL | state->shifting[index]);
#endif
}

void gl_palette_state_init(GL_Palette_State_t *state)
{
    *state = (GL_Palette_State_t){ 0 };

    gl_palette_state_reset(state);
}

void gl_palette_state_shifting(GL_Palette_State_t *state, GL_Pixel_t from, GL_Pixel_t to)
{
    if (state->shifting[from] == to) {
        return;
    }
    state->shifting[from] = to;

    _compute_map_entry(state, from);
}

void gl_palette_state_transparent(GL_Palette_State_t *state, GL_Pixel_t index, bool is_transparent)
{
    if (state->transparent[index] == is_transparent) {
        return;
    }
    state->transparent[index] = is_transparent;

    _compute_map_entry(state, index);
}

void gl_palette_state_reset(GL_Palette_State_t *state)
{
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        state->shifting[i] = (uint8_t)i;
        state->transparent[i] = i == 0;

        _compute_map_entry(state, i);
    }
}
