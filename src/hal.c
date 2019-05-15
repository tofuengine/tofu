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

#include "hal.h"

#include <math.h>
#include <stdlib.h>

#include "config.h"
#include "log.h"

#define RED_WEIGHT      2.0f
#define GREEN_WEIGHT    4.0f
#define BLUE_WEIGHT     3.0f

// https://en.wikipedia.org/wiki/Color_difference
static size_t find_nearest_color(const Palette_t *palette, Color color)
{
    size_t index = 0;
    double minimum = __DBL_MAX__;
    for (size_t i = 0; i < palette->count; ++i) {
        const Color *current = &palette->colors[i];

        double delta_r = (double)(color.r - current->r);
        double delta_g = (double)(color.g - current->g);
        double delta_b = (double)(color.b - current->b);
#ifdef __FIND_NEAREST_COLOR_EUCLIDIAN__
        double distance = sqrt((delta_r * delta_r) * RED_WEIGHT
            + (delta_g * delta_g) * GREEN_WEIGHT
            + (delta_b * delta_b)) * BLUE_WEIGHT;
#else
        double distance = (delta_r * delta_r) * RED_WEIGHT
            + (delta_g * delta_g) * GREEN_WEIGHT
            + (delta_b * delta_b) * BLUE_WEIGHT; // Faster, no need to get the Euclidean distance.
#endif
        if (minimum > distance) {
            minimum = distance;
            index = i;
        }
    }
    return index;
}

// TODO: convert image with a shader.
static void convert_image_to_palette(Image *image, const Palette_t *palette)
{
    Color *pixels = GetImageData(*image);

    for (int y = 0; y < image->height; ++y) {
        int row_offset = image->width * y;
        for (int x = 0; x < image->width; ++x) {
            int offset = row_offset + x;

            Color color = pixels[offset];
            if (color.a == 0) { // Skip transparent colors.
                continue;
            }

            size_t index = find_nearest_color(palette, color);
            pixels[offset] = (Color){ index, index, index, color.a };
        }
    }

    Image processed = LoadImageEx(pixels, image->width, image->height);
    ImageFormat(&processed, image->format);
    UnloadImage(*image);
    free(pixels);

    image->data = processed.data;
}

Bank_t load_bank(const char *pathfile, int cell_width, int cell_height, const Palette_t *palette)
{
    Image image = LoadImage(pathfile);
    if (!image.data) {
        return (Bank_t){};
    }
    convert_image_to_palette(&image, palette);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    Log_write(LOG_LEVELS_DEBUG, "[TOFU] Bank '%s' loaded as texture w/ id #%d", pathfile, texture.id);
    return (Bank_t){
            .loaded = texture.id != 0,
            .atlas = texture,
            .cell_width = cell_width,
            .cell_height = cell_height,
            .origin = (Vector2){ cell_width * 0.5f, cell_height * 0.5f} // Rotate along center
        };
}

void unload_bank(Bank_t *bank)
{
    Log_write(LOG_LEVELS_DEBUG, "[TOFU] Bank texture w/ id #%d unloaded", bank->atlas.id);
    UnloadTexture(bank->atlas);
    *bank = (Bank_t){};
}

Font_t load_font(const char *pathfile)
{
    Font font = LoadFont(pathfile);
    if (font.texture.id == 0) {
        return (Font_t){};
    }
    Log_write(LOG_LEVELS_DEBUG, "[TOFU] Font '%s' loaded as texture w/ id #%d", pathfile, font.texture.id);
    return (Font_t){
            .loaded = true,
            .is_default = false,
            .font = font
        };
}

void unload_font(Font_t *font)
{
    Log_write(LOG_LEVELS_DEBUG, "[TOFU] Font texture w/ id #%d unloaded", font->font.texture.id);
    UnloadFont(font->font);
    *font = (Font_t){};
}

Map_t load_map(const char *pathfile)
{
    return (Map_t){
            .loaded = true,
        };
}

void unload_map(Map_t *map)
{
    Log_write(LOG_LEVELS_DEBUG, "[TOFU] Map w/ id #%d unloaded", map->bank.atlas.id);
    *map = (Map_t){};
}
