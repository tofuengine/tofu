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

void gl_palette_state_init(GL_Palette_State_t *state)
{
    *state = (GL_Palette_State_t){ 0 };

    gl_palette_state_reset(state);
}

void gl_palette_state_shifting(GL_Palette_State_t *state, GL_Pixel_t from, GL_Pixel_t to)
{
    state->map[from] = (state->map[from] & GL_PALETTE_FLAGS_MASK)
        | to;

}

void gl_palette_state_transparent(GL_Palette_State_t *state, GL_Pixel_t index, bool is_transparent)
{
    state->map[index] = (is_transparent ? GL_PALETTE_FLAG_TRANSPARENT : 0)
        | (state->map[index] & ~GL_PALETTE_FLAG_TRANSPARENT);
}

void gl_palette_state_reset(GL_Palette_State_t *state)
{
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        state->map[i] = (i == 0 ? GL_PALETTE_FLAG_TRANSPARENT : 0)
            | (GL_Pixel_t)i;
    }
}
