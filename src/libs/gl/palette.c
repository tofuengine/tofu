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

#include "palette.h"

#include <core/config.h>
#include <libs/imath.h>
#include <libs/fmath.h>
#define _LOG_TAG "gl-palette"
#include <libs/log.h>

#include <string.h>

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

void GL_palette_set_greyscale(GL_Color_t *palette, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        uint8_t y = _quantize(i, 256, size);
        palette[i] = (GL_Color_t){ .r = y, .g = y, .b = y, .a = 255 };
    }

    for (size_t i = size; i < GL_MAX_PALETTE_COLORS; ++i) {
        palette[i] = (GL_Color_t){ .r = 0, .g = 0, .b = 0, .a = 255 };
    }
}

void GL_palette_set_quantized(GL_Color_t *palette, size_t red_bits, size_t green_bits, size_t blue_bits)
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
                palette[size++] = (GL_Color_t){ .r = r8, .g = g8, .b = b8, .a = 255 };
            }
        }
    }

    for (size_t i = size; i < GL_MAX_PALETTE_COLORS; ++i) {
        palette[i] = (GL_Color_t){ .r = 0, .g = 0, .b = 0, .a = 255 };
    }
}

#if TOFU_GRAPHICS_COLOR_MATCHING_ALGORITHM == COLOR_MATCH_PERCEPTUAL
typedef struct CIELAB_s {
    float L, a, b;
} CIELAB_t;

static inline float _gamma_correct(float v)
{
    return 100.0f * (v <= 0.04045f ? v / 12.92f : powf((v + 0.055f) / 1.055f, 2.4f));
}

static inline float _nonlinear_to_linear(float v)
{
    return v > 0.008856f ? cbrtf(v) : 7.787f * v + 16.0f / 116.0f;
}

static inline CIELAB_t _rgb_to_cielab(uint8_t r, uint8_t g, uint8_t b)
{
    float red = _gamma_correct((float)r / 255.0f);
    float green = _gamma_correct((float)g / 255.0f);
    float blue = _gamma_correct((float)b / 255.0f);

    float xr = _nonlinear_to_linear((red * 0.4124564f + green * 0.3575761f + blue * 0.1804375f) / 95.047f);
    float yr = _nonlinear_to_linear((red * 0.2126729f + green * 0.7151522f + blue * 0.0721750f) / 100.000f);
    float zr = _nonlinear_to_linear((red * 0.0193339f + green * 0.1191920f + blue * 0.9503041f) / 108.883f);

    return (CIELAB_t){ 116.0f * yr - 16.0f, 500.0f * (xr - yr), 200.0f * (yr - zr) };
}
#endif

GL_Pixel_t GL_palette_find_nearest_color(const GL_Color_t *palette, GL_Color_t color)
{
#if TOFU_GRAPHICS_COLOR_MATCHING_ALGORITHM == COLOR_MATCH_PERCEPTUAL
    CIELAB_t color_lab = _rgb_to_cielab(color.r, color.g, color.b);
#endif

    GL_Pixel_t index = 0;
    float minimum = __FLT_MAX__;
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        const GL_Color_t *current = &palette[i];

#if TOFU_GRAPHICS_COLOR_MATCHING_ALGORITHM == COLOR_MATCH_EUCLIDIAN
        const float delta_r = (float)(color.r - current->r);
        const float delta_g = (float)(color.g - current->g);
        const float delta_b = (float)(color.b - current->b);

        const float distance = delta_r * delta_r
            + delta_g * delta_g
            + delta_b * delta_b;
#elif TOFU_GRAPHICS_COLOR_MATCHING_ALGORITHM == COLOR_MATCH_WEIGHTED
        // https://www.compuphase.com/cmetric.htm
        const float delta_r = (float)(color.r - current->r);
        const float delta_g = (float)(color.g - current->g);
        const float delta_b = (float)(color.b - current->b);

        const float r_mean = (float)(color.r + current->r) * 0.5f;

        const float distance = (delta_r * delta_r) * (2.0f + (r_mean / 255.0f))
            + (delta_g * delta_g) * 4.0f
            + (delta_b * delta_b) * (2.0f + ((255.0f - r_mean) / 255.0f));
#elif TOFU_GRAPHICS_COLOR_MATCHING_ALGORITHM == COLOR_MATCH_PERCEPTUAL
        CIELAB_t current_lab = _rgb_to_cielab(current->r, current->g, current->b);

        float delta_L = color_lab.L - current_lab.L;
        float delta_a = color_lab.a - current_lab.a;
        float delta_b = color_lab.b - current_lab.b;

        const float distance = delta_L * delta_L
            + delta_a * delta_a
            + delta_b * delta_b;
#endif  /* TOFU_GRAPHICS_COLOR_MATCHING_ALGORITHM */

        // Note: computing the square root is unnecessary as the function is
        //       crescent invariant.

        if (minimum > distance) {
            minimum = distance;
            index = (GL_Pixel_t)i;
        }
    }

    return index;
}

GL_Color_t GL_palette_mix(GL_Color_t from, GL_Color_t to, float ratio)
{
    return (GL_Color_t){
            .r = (uint8_t)FLERP((float)from.r, (float)to.r, ratio),
            .g = (uint8_t)FLERP((float)from.g, (float)to.g, ratio),
            .b = (uint8_t)FLERP((float)from.b, (float)to.b, ratio),
            .a = 255
        };
}

void GL_palette_copy(GL_Color_t *palette, const GL_Color_t *source)
{
    memcpy(palette, source, sizeof(GL_Color_t) * GL_MAX_PALETTE_COLORS);
}

static bool _contains(const GL_Color_t *palette, GL_Color_t color)
{
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        if (memcmp(&palette[i], &color, sizeof(GL_Color_t)) == 0) {
            return true;
        }
    }
    return false;
}

size_t GL_palette_merge(GL_Color_t *palette, size_t to, const GL_Color_t *other, size_t from, size_t count, bool remove_duplicates)
{
    size_t to_i = to;
    for (size_t i = 0; i < count; ++i) {
        if (to_i == GL_MAX_PALETTE_COLORS) {
            LOG_W("maximum palette size reached when merging palette %p w/ %p", palette, other);
            break;
        }
        size_t from_i = from + i;
        if (remove_duplicates && _contains(palette, other[from_i])) {
            continue;
        }
        palette[to_i++] = other[from_i];
    }
    return to_i;
}

void GL_palette_lerp(GL_Color_t *palette, GL_Color_t color, float ratio)
{
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        palette[i] = GL_palette_mix(palette[i], color, ratio);
    }
}
