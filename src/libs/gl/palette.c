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

#include "palette.h"

#include <config.h>
#include <libs/imath.h>
#include <libs/fmath.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <string.h>

#define LOG_CONTEXT "gl-palette"

GL_Palette_t *GL_palette_create(void)
{
    GL_Palette_t *palette = malloc(sizeof(GL_Palette_t));
    if (!palette) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate palette");
        return NULL;
    }

    *palette = (GL_Palette_t){ 0 };

    return palette;
}

void GL_palette_destroy(GL_Palette_t *palette)
{
#ifdef __PALETTE_COLOR_MEMOIZATION__
    hmfree(palette->cache);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "memoizing cache %p freed", palette->cache);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */

    free(palette);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "palette %p freed", palette);
}

void GL_palette_set_colors(GL_Palette_t *palette, const GL_Color_t *colors, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        palette->colors[i] = colors[i];
    }
    palette->size = size;

#ifdef __PALETTE_COLOR_MEMOIZATION__
    hmfree(palette->cache);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "memoizing cache %p freed", palette->cache);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */
}

//
// Due to the nature of the `GL_Palette_t` type, the total amount of bits for RGB components *cannot* be greater than
// `sizeof(GL_Pixel_t) * 8` bits.
//
// When converting an `n` bits color component to `8` bits, we need to pad the lower bits LERP-ing the full range of
// available values. Leaving them as zeros won't be correct.
//
// For example in case we are promoting a 3 bits component to 8:
//
//     N N N | P P P P P
//    -------+-----------
//     0 0 0 | 0 0 0 0 0
//     0 0 1 | 0 0 1 0 0
//     0 1 0 | 0 1 0 0 0
//     0 1 1 | 0 1 1 0 1
//     1 0 0 | 1 0 0 0 1
//     1 0 1 | 1 0 1 1 0
//     1 1 0 | 1 1 0 1 0
//     1 1 1 | 1 1 1 1 1
//
// or, for a 2 bits component
//
//     N N | P P P P P P
//    -----+-------------
//     0 0 | 0 0 0 0 0 0
//     0 1 | 0 1 0 1 0 1
//     1 0 | 1 0 1 0 1 0
//     1 1 | 1 1 1 1 1 1
//
// The "i-th" color padding value is calculated with the following formula
//
//     i * ((1 << (8 - bits)) - 1) / ((1 << bits) - 1))
//
static inline uint8_t _quantize(size_t value, size_t values, size_t count)
{
    return (uint8_t)((value * (values - 1)) / (count - 1));
}

void GL_palette_set_greyscale(GL_Palette_t *palette, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        uint8_t y = _quantize(i, 256, size);
        palette->colors[i] = (GL_Color_t){ .r = y, .g = y, .b = y, .a = 255 };
    }
    palette->size = size;

#ifdef __PALETTE_COLOR_MEMOIZATION__
    hmfree(palette->cache);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "memoizing cache %p freed", palette->cache);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */
}

void GL_palette_set_quantized(GL_Palette_t *palette, const size_t red_bits, const size_t green_bits, const size_t blue_bits)
{
    const size_t red_values = 1 << red_bits;
    const size_t green_values = 1 << green_bits;
    const size_t blue_values = 1 << blue_bits;

    const size_t red_lower_bits = 8 - red_bits;
    const size_t green_lower_bits = 8 - green_bits;
    const size_t blue_lower_bits = 8 - blue_bits;

    const size_t red_lower_values = 1 << red_lower_bits;
    const size_t green_lower_values = 1 << green_lower_bits;
    const size_t blue_lower_values = 1 << blue_lower_bits;

    size_t size = 0;
    for (size_t r = 0; r < red_values; ++r) {
        uint8_t r8 = (r << red_lower_bits) | _quantize(r, red_lower_values, red_values);
        for (size_t g = 0; g < green_values; ++g) {
            uint8_t g8 = (g << green_lower_bits) | _quantize(g, green_lower_values, green_values);
            for (size_t b = 0; b < blue_values; ++b) {
                uint8_t b8 = (b << blue_lower_bits) | _quantize(b, blue_lower_values, blue_values);
                palette->colors[size++] = (GL_Color_t){ .r = r8, .g = g8, .b = b8, .a = 255 };
            }
        }
    }
    palette->size = size;

#ifdef __PALETTE_COLOR_MEMOIZATION__
    hmfree(palette->cache);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "memoizing cache %p freed", palette->cache);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */
}

GL_Color_t GL_palette_get(const GL_Palette_t *palette, GL_Pixel_t index)
{
    return palette->colors[index];
}

void GL_palette_set(GL_Palette_t *palette, GL_Pixel_t index, GL_Color_t color)
{
    size_t size = palette->size;
    palette->size = (size_t)imax(palette->size, index + 1);
    for (size_t i = size; i < palette->size; ++i) { // Expand palette if setting a color outside current range.
        palette->colors[i] = (GL_Color_t){ .r = 0, .g = 0, .b = 0, .a = 255 };
    }

    palette->colors[index] = color;

#ifdef __PALETTE_COLOR_MEMOIZATION__
    hmfree(palette->cache);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "memoizing cache %p freed", palette->cache);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */
}

#ifdef __PALETTE_COLOR_MEMOIZATION__
GL_Pixel_t GL_palette_find_nearest_color(GL_Palette_t *palette, const GL_Color_t color)
#else
GL_Pixel_t GL_palette_find_nearest_color(const GL_Palette_t *palette, const GL_Color_t color)
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */
{
#ifdef __PALETTE_COLOR_MEMOIZATION__
    const int position = hmgeti(palette->cache, color);
    if (position != -1) {
        const GL_Pixel_t index = palette->cache[position].value;
        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "color <%d, %d, %d> found into memoizing cache at #%d (index #%d)", color.r, color.g, color.b, position, index);
        return index;
    }
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */

    GL_Pixel_t index = 0;
    float minimum = __FLT_MAX__;
    for (size_t i = 0; i < palette->size; ++i) {
        const GL_Color_t *current = &palette->colors[i];

        // https://www.compuphase.com/cmetric.htm
        const float r_mean = (float)(color.r + current->r) * 0.5f;

        const float delta_r = (float)(color.r - current->r);
        const float delta_g = (float)(color.g - current->g);
        const float delta_b = (float)(color.b - current->b);

        const float distance = (delta_r * delta_r) * (2.0f + (r_mean / 255.0f))
            + (delta_g * delta_g) * 4.0f
            + (delta_b * delta_b) * (2.0f + ((255.0f - r_mean) / 255.0f));
#ifdef __FIND_NEAREST_COLOR_EUCLIDIAN__
        distance = sqrtf(distance);
#endif
        if (minimum > distance) {
            minimum = distance;
            index = (GL_Pixel_t)i;
        }
    }

#ifdef __PALETTE_COLOR_MEMOIZATION__
    hmput(palette->cache, color, index);
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "color <%d, %d, %d> stored into memoizing cache w/ index #%d", color.r, color.g, color.b, index);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */

    return index;
}

GL_Color_t GL_palette_mix(const GL_Color_t from, const GL_Color_t to, float ratio)
{
    return (GL_Color_t){
            .r = (uint8_t)FLERP((float)from.r, (float)to.r, ratio),
            .g = (uint8_t)FLERP((float)from.g, (float)to.g, ratio),
            .b = (uint8_t)FLERP((float)from.b, (float)to.b, ratio),
            .a = 255
        };
}

void GL_palette_copy(GL_Palette_t *palette, const GL_Palette_t *source)
{
    GL_palette_set_colors(palette, source->colors, source->size);
}

static bool _contains(const GL_Palette_t *palette, GL_Color_t color)
{
    for (size_t i = 0; i < palette->size; ++i) {
        if (memcmp(&palette->colors[i], &color, sizeof(GL_Color_t)) == 0) {
            return true;
        }
    }
    return false;
}

void GL_palette_merge(GL_Palette_t *palette, const GL_Palette_t *other, bool remove_duplicates)
{
    for (size_t i = 0; i < other->size; ++i) {
        if (palette->size == GL_MAX_PALETTE_COLORS) {
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "maximum palette size reached when merging palette %p w/ %p", palette, other);
            break;
        }
        if (remove_duplicates && _contains(palette, other->colors[i])) {
            continue;
        }
        palette->colors[palette->size++] = other->colors[i];
    }

#ifdef __PALETTE_COLOR_MEMOIZATION__
    hmfree(palette->cache);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "memoizing cache %p freed", palette->cache);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */
}

void GL_palette_lerp(GL_Palette_t *palette, const GL_Color_t color, float ratio)
{
    GL_Color_t *colors = palette->colors;
    for (size_t i = 0; i < palette->size; ++i) {
        colors[i] = GL_palette_mix(colors[i], color, ratio);
    }

#ifdef __PALETTE_COLOR_MEMOIZATION__
    hmfree(palette->cache);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "memoizing cache %p freed", palette->cache);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */
}
