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

#include "font.h"

#include <spleen/spleen.h>

#include <string.h>

#include "../GL/gl.h"
#include "../log.h"

static GL_Font_t _default_font;

static void to_font_atlas_callback(void *parameters, void *data, int width, int height)
{
    GL_Color_t *pixels = (GL_Color_t *)data;

    for (int y = 0; y < height; ++y) {
        int row_offset = width * y;
        for (int x = 0; x < width; ++x) {
            int offset = row_offset + x;

            GL_Color_t color = pixels[offset];
            GLubyte index = color.a == 0 ? 0 : 255;
            pixels[offset] = (GL_Color_t){ index, index, index, color.a };
        }
    }
}

bool GL_font_initialize()
{
    return GL_font_create(&_default_font, spleen_5x8_png, spleen_5x8_png_len, 5, 8);
}

void GL_font_terminate()
{
    GL_font_delete(&_default_font);
}

const GL_Font_t *GL_font_default()
{
    return &_default_font;
}

bool GL_font_load(GL_Font_t *font, const char *pathfile, GLuint glyph_width, GLuint glyph_height)
{
    GL_Texture_t atlas;
    if (!GL_texture_load(&atlas, pathfile, to_font_atlas_callback, NULL)) {
        return false;
    }

    *font = (GL_Font_t){
            .atlas = atlas,
            .glyph_width = glyph_width,
            .glyph_height = glyph_height
        };
    Log_write(LOG_LEVELS_DEBUG, "<GL> font #%p created", font);

    return true;
}

bool GL_font_create(GL_Font_t *font, const void *buffer, size_t size, GLuint glyph_width, GLuint glyph_height)
{
    GL_Texture_t atlas;
    if (!GL_texture_decode(&atlas, buffer, size, to_font_atlas_callback, NULL)) {
        return false;
    }

    *font = (GL_Font_t){
            .atlas = atlas,
            .glyph_width = glyph_width,
            .glyph_height = glyph_height
        };
    Log_write(LOG_LEVELS_DEBUG, "<GL> font #%p created", font);

    return true;
}

void GL_font_delete(GL_Font_t *font)
{
    GL_texture_delete(&font->atlas);
    Log_write(LOG_LEVELS_DEBUG, "<GL> font #%p deleted", font);
    *font = (GL_Font_t){};
}

GL_Size_t GL_font_measure(const GL_Font_t *font, const char *text, const GLfloat scale)
{
    return (GL_Size_t){
            .width = (GLfloat)font->glyph_width * strlen(text) * scale,
            .height = (GLfloat)font->glyph_height * scale
        };
}

void GL_font_write(const GL_Font_t *font, const char *text, const GL_Point_t position, const GLfloat scale, const GL_Color_t color)
{
    const GLfloat width = font->glyph_width * scale, height = font->glyph_height * scale;

    GL_Quad_t source = { .x0 = 0.0f, .y0 = 0.0f, .x1 = font->glyph_width, .y1 = font->glyph_height };
    GL_Quad_t destination = { .x0 = position.x, .y0 = position.y, .x1 = position.x + width, .y1 = position.y + height };

    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
        source.x0 = (GLfloat)(*ptr - ' ') * font->glyph_width;
        source.x1 = source.x0 + font->glyph_width;

        if (source.x0 >= font->atlas.width) {
            continue;
        }

        GL_texture_blit_fast(&font->atlas, source, destination, color);

        destination.x0 += width;
        destination.x1 = destination.x0 + width;
    }
}
