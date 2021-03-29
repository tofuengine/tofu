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

#include <string.h>

void GL_palette_generate_greyscale(GL_Palette_t *palette, const size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        unsigned char y = (unsigned char)(((float)i / (float)(size - 1)) * 255.0f);
        palette->colors[i] = (GL_Color_t){ .r = y, .g = y, .b = y, .a = 255  };
    }
    palette->size = size;
}

GL_Pixel_t GL_palette_find_nearest_color(const GL_Palette_t *palette, const GL_Color_t color)
{
    GL_Pixel_t index = 0;
    float minimum = __FLT_MAX__;
    for (size_t i = 0; i < palette->size; ++i) {
        const GL_Color_t *current = &palette->colors[i];

        // https://www.compuphase.com/cmetric.htm
        float r_mean = (float)(color.r + current->r) * 0.5;

        float delta_r = (float)(color.r - current->r);
        float delta_g = (float)(color.g - current->g);
        float delta_b = (float)(color.b - current->b);

        float distance = (delta_r * delta_r) * (2.0f + (r_mean / 256.0f))
            + (delta_g * delta_g) * 4.0f
            + (delta_b * delta_b) * (2.0f + (255.0f - r_mean) / 256.0f);
#ifdef __FIND_NEAREST_COLOR_EUCLIDIAN__
        distance = sqrtf(distance);
#endif
        if (minimum > distance) {
            minimum = distance;
            index = (GL_Pixel_t)i;
        }
    }
    return index;
}

static inline float _lerpf(float v0, float v1, float t)
{
    // More numerical stable than the following one.
    // return (v1 - v0) * t + v0
    // see: https://en.wikipedia.org/wiki/Linear_interpolation
    return v0 * (1.0f - t) + v1 * t;
}

GL_Color_t GL_palette_lerp(const GL_Color_t from, const GL_Color_t to, float ratio)
{
    return (GL_Color_t){
            .r = (uint8_t)_lerpf((float)from.r, (float)to.r, ratio),
            .g = (uint8_t)_lerpf((float)from.g, (float)to.g, ratio),
            .b = (uint8_t)_lerpf((float)from.b, (float)to.b, ratio),
            .a = 255
        };
}

GL_Color_t GL_palette_unpack_color(uint32_t argb)
{
    return (GL_Color_t){
#ifdef __IGNORE_ALPHA_ON_COLORS__
            .a = 255,
#else
            .a = (uint8_t)((argb >> 24) & 0xff),
#endif
            .r = (uint8_t)((argb >> 16) & 0xff),
            .g = (uint8_t)((argb >>  8) & 0xff),
            .b = (uint8_t)( argb        & 0xff)
        };
}

uint32_t GL_palette_pack_color(const GL_Color_t color)
{
    return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
}
