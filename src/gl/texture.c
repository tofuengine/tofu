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

#include "texture.h"

#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "../log.h"

bool GL_create_texture(GL_Texture_t *texture, const char *pathfile, GL_Texture_Callback_t callback, void *parameters)
{
    int width, height, components;
    unsigned char* data = stbi_load(pathfile, &width, &height, &components, STBI_rgb_alpha); //STBI_default);
    if (!data) {
        Log_write(LOG_LEVELS_ERROR, "<GL> can't load texture '%s': %s", pathfile, stbi_failure_reason());
        return false;
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

    Log_write(LOG_LEVELS_DEBUG, "<GL> texture '%s' loaded as #%d (%dx%d)", pathfile, id, width, height);

    *texture = (GL_Texture_t){
            .id = id,
            .width = width,
            .height = height
        };

    return true;
}

void GL_delete_texture(GL_Texture_t *texture)
{
    glDeleteBuffers(1, &texture->id);
    Log_write(LOG_LEVELS_DEBUG, "<GL> texture #%d unloaded", texture->id);
    *texture = (GL_Texture_t){};
}

void GL_draw_texture(const GL_Texture_t *texture,
                     const GL_Rectangle_t source, const GL_Rectangle_t target,
                     const GL_Point_t origin, GLfloat rotation,
                     const GL_Color_t color)
{
#ifdef __DEFENSIVE_CHECKS__
    if (texture->id == 0) {
        // TODO: output log here?
        return;
    }
#endif

    GLfloat width = texture->width;
    GLfloat height = texture->height;

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->id);

    glPushMatrix();
    glTranslatef(target.x, target.y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);
    glTranslatef(-origin.x, -origin.y, 0.0f);

    glBegin(GL_QUADS);
        glColor4ub(color.r, color.g, color.b, color.a);
        glNormal3f(0.0f, 0.0f, 1.0f); // Normal vector pointing towards viewer

        // Bottom-left corner for texture and quad
        glTexCoord2f(source.x / width, source.y / height);
        glVertex2f(0.0f, 0.0f);

        // Bottom-right corner for texture and quad
        glTexCoord2f(source.x / width, (source.y + source.height) / height);
        glVertex2f(0.0f, target.height);

        // Top-right corner for texture and quad
        glTexCoord2f((source.x + source.width) / width, (source.y + source.height) / height);
        glVertex2f(target.width, target.height);

        // Top-left corner for texture and quad
        glTexCoord2f((source.x + source.width) / width, source.y / height);
        glVertex2f(target.width, 0.0f);
    glEnd();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}
