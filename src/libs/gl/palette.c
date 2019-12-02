/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "palette.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RED_WEIGHT      2.0f
#define GREEN_WEIGHT    4.0f
#define BLUE_WEIGHT     3.0f

void GL_palette_greyscale(GL_Palette_t *palette, const size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        unsigned char v = (unsigned char)(((float)i / (float)(count - 1)) * 255.0f);
        palette->colors[i] = (GL_Color_t){ v, v, v, 255 };
    }
    palette->count = count;
}

GL_Color_t GL_palette_parse_color(const char *argb)
{
    GL_Color_t color;
    char hex[3];
    if (strlen(argb) == 8) {
        strncpy(hex, argb    , 2); color.a = strtol(hex, NULL, 16);
        strncpy(hex, argb + 2, 2); color.r = strtol(hex, NULL, 16);
        strncpy(hex, argb + 4, 2); color.g = strtol(hex, NULL, 16);
        strncpy(hex, argb + 6, 2); color.b = strtol(hex, NULL, 16);
    } else
    if (strlen(argb) == 6) {
        color.a = 255; // Fully opaque.
        strncpy(hex, argb    , 2); color.r = strtol(hex, NULL, 16);
        strncpy(hex, argb + 2, 2); color.g = strtol(hex, NULL, 16);
        strncpy(hex, argb + 4, 2); color.b = strtol(hex, NULL, 16);
    }
    return color;
}

GL_Color_t GL_palette_unpack_color(const unsigned int argb)
{
    return (GL_Color_t){
            .a = (uint8_t)((argb >> 24) & 0xff),
            .r = (uint8_t)((argb >> 16) & 0xff),
            .g = (uint8_t)((argb >>  8) & 0xff),
            .b = (uint8_t)( argb        & 0xff)
        };
}

void GL_palette_format_color(char *argb, const GL_Color_t color)
{
#ifdef __LOWERCASE_ARGB__
    sprintf(argb, "%02x%02x%02x%02x", color.a, color.r, color.g, color.b);
#else
    sprintf(argb, "%02X%02X%02X%02X", color.a, color.r, color.g, color.b);
#endif
}

unsigned int GL_palette_pack_color(const GL_Color_t color)
{
    return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
}

// https://en.wikipedia.org/wiki/Color_difference
GL_Pixel_t GL_palette_find_nearest_color(const GL_Palette_t *palette, const GL_Color_t color)
{
    GL_Pixel_t index = 0;
    float minimum = __FLT_MAX__;
    for (size_t i = 0; i < palette->count; ++i) {
        const GL_Color_t *current = &palette->colors[i];

        float delta_r = (float)(color.r - current->r);
        float delta_g = (float)(color.g - current->g);
        float delta_b = (float)(color.b - current->b);
#ifdef __FIND_NEAREST_COLOR_EUCLIDIAN__
        float distance = sqrtf((delta_r * delta_r) * RED_WEIGHT
            + (delta_g * delta_g) * GREEN_WEIGHT
            + (delta_b * delta_b)) * BLUE_WEIGHT;
#else
        float distance = (delta_r * delta_r) * RED_WEIGHT
            + (delta_g * delta_g) * GREEN_WEIGHT
            + (delta_b * delta_b) * BLUE_WEIGHT; // Faster, no need to get the Euclidean distance.
#endif
        if (minimum > distance) {
            minimum = distance;
            index = (GL_Pixel_t)i;
        }
    }
    return index;
}