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

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "config.h"
#include "log.h"

#define RED_WEIGHT      2.0f
#define GREEN_WEIGHT    4.0f
#define BLUE_WEIGHT     3.0f

// https://en.wikipedia.org/wiki/Color_difference
static size_t find_nearest_color(const Palette_t *palette, Color_t color)
{
    size_t index = 0;
    double minimum = __DBL_MAX__;
    for (size_t i = 0; i < palette->count; ++i) {
        const Color_t *current = &palette->colors[i];

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
static void palettize_callback(void *parameters, void *data, int width, int height)
{
    const Palette_t *palette = (const Palette_t *)parameters;

    Color_t *pixels = (Color_t *)data;

    for (int y = 0; y < height; ++y) {
        int row_offset = width * y;
        for (int x = 0; x < width; ++x) {
            int offset = row_offset + x;

            Color_t color = pixels[offset];
            if (color.a == 0) { // Skip transparent colors.
                continue;
            }

            size_t index = find_nearest_color(palette, color);
            pixels[offset] = (Color_t){ index, index, index, color.a };
        }
    }
}

Texture_t load_texture(const char *pathfile, Texture_Callback_t callback, void *parameters)
{
    int width, height, components;
    unsigned char* data = stbi_load(pathfile, &width, &height, &components, STBI_rgb_alpha); //STBI_default);
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, "Can't load '%s': %s", pathfile, stbi_failure_reason());
        return (Texture_t){ 0, 0, 0 };
    }

    if (callback != NULL) {
        callback(parameters, data, width, height);
    }

    GLuint id;
    glGenTextures(1, &id); //allocate the memory for texture
    glBindTexture(GL_TEXTURE_2D, id); //Binding the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    return (Texture_t){ id, width, height };
}

void unload_texture(Texture_t *texture)
{
    glDeleteBuffers(1, &texture->id);
}

Bank_t load_bank(const char *pathfile, int cell_width, int cell_height, const Palette_t *palette)
{
    Texture_t texture = load_texture(pathfile, palettize_callback, (void *)palette);
    Log_write(LOG_LEVELS_DEBUG, "<HAL> Bank '%s' loaded as texture w/ id #%d", pathfile, texture.id);
    return (Bank_t){
            .loaded = texture.id != 0,
            .atlas = texture,
            .cell_width = cell_width,
            .cell_height = cell_height,
            .origin = (Point_t){ cell_width * 0.5, cell_height * 0.5} // Rotate along center
        };
}

void unload_bank(Bank_t *bank)
{
    Log_write(LOG_LEVELS_DEBUG, "<HAL> Bank texture w/ id #%d unloaded", bank->atlas.id);
    unload_texture(&bank->atlas);
    *bank = (Bank_t){};
}

Font_t load_font(const char *pathfile)
{
    Texture_t texture = load_texture(pathfile, NULL, NULL);
    Log_write(LOG_LEVELS_DEBUG, "<HAL> Font '%s' loaded as texture w/ id #%d", pathfile, texture.id);
    return (Font_t){
            .loaded = true,
            .atlas = texture
        };
}

void unload_font(Font_t *font)
{
    Log_write(LOG_LEVELS_DEBUG, "<HAL> Font texture w/ id #%d unloaded", font->atlas.id);
    unload_texture(&font->atlas);
    *font = (Font_t){};
}
