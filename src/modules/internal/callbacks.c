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
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "callbacks.h"

#include <core/config.h>
#include <libs/stb.h>

#pragma pack(push, 1)
typedef struct rgba_s {
    uint8_t r, g, b, a;
} rgba_t;
#pragma pack(pop)

// Given an `MxN` RGBA8888 image, the naive conversion to the color-indexed format requires `MxN` scans to find the
// nearest-matching color in the palette. This is a computationally demanding operation, since it computes the Euclidean
// distance for each palette-entry. Even for small images the load-and-convert times are non negligible.
//
// We can get a huge performance boost by adopting a "memoization" technique. Each nearest match is dynamically stored
// into a hash-map during the conversion: a color is first checked if has been already encountered and converted; if not
// it is converted and stored for later usage.
//
// Since the total amount of distinct colors in a single image is typically small, the additional memory usage is worth
// the effort.
void surface_callback_palette(const void *user_data, GL_Surface_t *surface, const void *pixels)
{
    const Callback_Palette_Closure_t *closure = (const Callback_Palette_Closure_t *)user_data;
#if defined(TOFU_GRAPHICS_PALETTE_MATCH_MEMOIZATION)
    struct {
        GL_Color_t key;
        GL_Pixel_t value;
    } *cache = NULL; // Stores past executed colors matches.
#endif  /* TOFU_GRAPHICS_PALETTE_MATCH_MEMOIZATION */

    const rgba_t *src = (const rgba_t *)pixels;
    GL_Pixel_t *dst = surface->data;

    for (size_t i = surface->data_size; i; --i) {
        rgba_t rgba = *(src++);
        if (rgba.a <= closure->threshold) { // Colors is transparent if not above threshold (can't disable it).
            *(dst++) = closure->transparent;
        } else {
            GL_Color_t color = (GL_Color_t){ .r = rgba.r, .g = rgba.g, .b = rgba.b, .a = rgba.a };

#if defined(TOFU_GRAPHICS_PALETTE_MATCH_MEMOIZATION)
            const int position = hmgeti(cache, color);
            if (position != -1) {
                const GL_Pixel_t index = cache[position].value;
                *(dst++) = index;
                continue;
            }
#endif  /* TOFU_GRAPHICS_PALETTE_MATCH_MEMOIZATION */

            const GL_Pixel_t index = GL_palette_find_nearest_color(closure->palette, color);
            *(dst++) = index;
#if defined(TOFU_GRAPHICS_PALETTE_MATCH_MEMOIZATION)
            hmput(cache, color, index);
#endif  /* TOFU_GRAPHICS_PALETTE_MATCH_MEMOIZATION */
        }
    }

#if defined(TOFU_GRAPHICS_PALETTE_MATCH_MEMOIZATION)
    hmfree(cache);
#endif  /* TOFU_GRAPHICS_PALETTE_MATCH_MEMOIZATION */
}

void surface_callback_indexes(const void *user_data, GL_Surface_t *surface, const void *pixels)
{
    const Callback_Indexes_Closure_t *closure = (const Callback_Indexes_Closure_t *)user_data;

    const uint32_t *src = (const uint32_t *)pixels; // Faster than the `rgba_t` struct, we don't need to unpack components.
    GL_Pixel_t *dst = surface->data;

    const uint32_t background = *src; // The top-left pixel color defines the background.

    for (size_t i = surface->data_size; i; --i) {
        uint32_t rgba = *(src++);
        *(dst++) = rgba == background ? closure->background : closure->foreground;
    }
}
