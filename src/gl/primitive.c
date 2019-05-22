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

#include "primitive.h"

#include "../log.h"

static const GLubyte _white_pixel[4] = { 255, 255, 255, 255 };
static GLuint _default_texture_id;

bool GL_primitive_initialize()
{
    glGenTextures(1, &_default_texture_id); // We need a 1x1 white texture to properly color/texture the primitives!
    if (_default_texture_id == 0) {
        Log_write(LOG_LEVELS_DEBUG, "<GL> can't create default texture");
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, _default_texture_id); //Binding the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Just to be safe!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, _white_pixel);

    Log_write(LOG_LEVELS_DEBUG, "<GL> default (white) 1x1 texture created w/ id #%d", _default_texture_id);

    return true;
}

void GL_primitive_terminate()
{
    glDeleteTextures(1, &_default_texture_id);
    Log_write(LOG_LEVELS_DEBUG, "<GL> default texture w/ id #%d deleted", _default_texture_id);
    _default_texture_id = 0U;
}

void GL_primitive_point(const GL_Point_t position, const GL_Color_t color)
{
    glBindTexture(GL_TEXTURE_2D, _default_texture_id);
    glBegin(GL_POINTS);
        glColor4ub(color.r, color.g, color.b, color.a);

        glVertex2f(position.x, position.y);
    glEnd();
}

void GL_primitive_line(const GL_Point_t from, const GL_Point_t to, const GL_Color_t color)
{
    glBindTexture(GL_TEXTURE_2D, _default_texture_id);
    glBegin(GL_LINES);
        glColor4ub(color.r, color.g, color.b, color.a);

        glVertex2f(from.x, from.y);
        glVertex2f(to.x, to.y);
    glEnd();
}

void GL_primitive_polygon(const GL_Point_t *points, const size_t count, const GL_Color_t color, bool filled)
{
#ifdef __DEFENSIVE_CHECKS__
    if (count < 3) {
        return;
    }
#endif

    glBindTexture(GL_TEXTURE_2D, _default_texture_id);
    glBegin(GL_TRIANGLE_STRIP);
        glColor4ub(color.r, color.g, color.b, color.a);

        for (size_t i = 0; i < count; ++i) {
            glVertex2f(points[i].x, points[i].y);
        }
    glEnd();
}

void GL_primitive_circle(const GL_Point_t center, const GLfloat radius, const GL_Color_t color, bool filled)
{
}