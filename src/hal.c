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

#include "../../config.h"
#include "log.h"

static int find_nearest_color(const Color *palette, int count, Color color)
{
    int index = -1;
    float minimum = __FLT_MAX__;
    for (int i = 0; i < count; ++i) {
        const Color *current = &palette[i];
#ifdef __FIND_NEAREST_COLOR_EUCLIDIAN__
        float distance = sqrtf(powf(color.r - current->r, 2.0f)
            + powf(color.g - current->g, 2.0f)
            + powf(color.b - current->b, 2.0f)
            + powf(color.a - current->a, 2.0f));
#else
        float distance = powf(color.r - current->r, 2.0f) // Faster, no need to get the Euclidean distance.
            + powf(color.g - current->g, 2.0f)
            + powf(color.b - current->b, 2.0f)
            + powf(color.a - current->a, 2.0f);
#endif
        if (minimum > distance) {
            minimum = distance;
            index = i;
        }
    }
    return index;
}

static void convert_image_to_palette(Image *image, const Color *palette, int colors)
{
    int extractCount = 0;
    Color *extractPalette = ImageExtractPalette(*image, MAX_DISTINCT_COLORS, &extractCount);
    Log_write(LOG_LEVELS_DEBUG, "[TOFU] Image '%p' has %d distinct color(s)", image, extractCount);

    for (int i = 0; i < extractCount; ++i) {
        int index = find_nearest_color(palette, colors, extractPalette[i]);
        ImageColorReplace(image, extractPalette[i], (Color){ index, index, index, 255 });
    }
    free(extractPalette);
}

Bank_t load_bank(const char *pathfile, int cell_width, int cell_height, const Color *palette, int colors)
{
    Image image = LoadImage(pathfile);
    if (!image.data) {
        return (Bank_t){};
    }
    convert_image_to_palette(&image, palette, colors);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    Log_write(LOG_LEVELS_DEBUG, "[TOFU] Bank '%s' loaded as texture w/ id #%d", pathfile, texture.id);
    return (Bank_t){
            .loaded = texture.id != 0,
            .atlas = texture,
            .cell_width = cell_width,
            .cell_height = cell_height
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
