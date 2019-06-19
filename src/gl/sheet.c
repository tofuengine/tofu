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

#include "sheet.h"

#include "../GL/gl.h"
#include "../log.h"

#include <stdlib.h>

GL_Quad_t *precompute_quads(GLuint width, GLuint height, GLuint quad_width, GLuint quad_height)
{
    GLuint columns = width / quad_width;
    GLuint rows = height / quad_height;
    GLuint amount = columns * rows;
    GL_Quad_t *quads = malloc(amount * sizeof(GL_Quad_t));
    GLuint k = 0;
    for (GLuint i = 0; i < rows; ++i) {
        GLfloat y = i * quad_height;
        for (GLuint j = 0; j < columns; ++j) {
            GLfloat x = j * quad_width;
            quads[k++] = (GL_Quad_t){ // Pre-normalized.
                    .x0 = x / width,
                    .y0 = y / height,
                    .x1 = (x + quad_width) / width,
                    .y1 = (y + quad_height) / height
                };
        }
    }
    return quads;
}

bool GL_sheet_load(GL_Sheet_t *sheet, const char *pathfile, GLuint quad_width, GLuint quad_height, const GL_Texture_Callback_t callback, void *parameters)
{
    GL_Texture_t atlas;
    if (!GL_texture_load(&atlas, pathfile, callback, parameters)) {
        return false;
    }

    *sheet = (GL_Sheet_t){
            .atlas = atlas,
            .quads = precompute_quads(atlas.width, atlas.height, quad_width, quad_height),
            .quad = (GL_Size_t){ quad_width, quad_height }
        };
    Log_write(LOG_LEVELS_DEBUG, "<GL> sheet #%p created", sheet);

    return true;
}

bool GL_sheet_decode(GL_Sheet_t *sheet, const void *buffer, size_t size, GLuint quad_width, GLuint quad_height, const GL_Texture_Callback_t callback, void *parameters)
{
    GL_Texture_t atlas;
    if (!GL_texture_decode(&atlas, buffer, size, callback, parameters)) {
        return false;
    }

    *sheet = (GL_Sheet_t){
            .atlas = atlas,
            .quads = precompute_quads(atlas.width, atlas.height, quad_width, quad_height),
            .quad = (GL_Size_t){ quad_width, quad_height }
        };
    Log_write(LOG_LEVELS_DEBUG, "<GL> sheet #%p created", sheet);

    return true;
}

void GL_sheet_delete(GL_Sheet_t *sheet)
{
    free(sheet->quads);
    GL_texture_delete(&sheet->atlas);
    Log_write(LOG_LEVELS_DEBUG, "<GL> sheet #%p deleted", sheet);
    *sheet = (GL_Sheet_t){};
}

void GL_sheet_blit(const GL_Sheet_t *sheet, size_t quad, const GL_Quad_t destination, const GL_Point_t origin, GLfloat rotation, const GL_Color_t color)
{
#ifdef __DEFENSIVE_CHECKS__
    if (texture->id == 0) {
        // TODO: output log here?
        return;
    }
#endif

    const GL_Quad_t *source = sheet->quads + quad;

    GLfloat sx0 = source->x0;
    GLfloat sy0 = source->y0;
    GLfloat sx1 = source->x1;
    GLfloat sy1 = source->y1;

    GLfloat dx0 = 0.0f;
    GLfloat dy0 = 0.0f;
    GLfloat dx1 = destination.x1 - destination.x0;
    GLfloat dy1 = destination.y1 - destination.y0;

    glBindTexture(GL_TEXTURE_2D, sheet->atlas.id);

    glPushMatrix();
        glTranslatef(destination.x0, destination.y0, 0.0f);
        glRotatef(rotation, 0.0f, 0.0f, 1.0f);
        glTranslatef(-origin.x, -origin.y, 0.0f);
        glBegin(GL_TRIANGLE_STRIP);
            glColor4ub(color.r, color.g, color.b, color.a);

            glTexCoord2f(sx0, sy0); // CCW strip, top-left is <0,0> (the face direction of the strip is determined by the winding of the first triangle)
            glVertex2f(dx0, dy0);
            glTexCoord2f(sx0, sy1);
            glVertex2f(dx0, dy1);
            glTexCoord2f(sx1, sy0);
            glVertex2f(dx1, dy0);
            glTexCoord2f(sx1, sy1);
            glVertex2f(dx1, dy1);
        glEnd();
    glPopMatrix();
}

void GL_sheet_blit_fast(const GL_Sheet_t *sheet, size_t quad, const GL_Quad_t destination, const GL_Color_t color)
{
#ifdef __DEFENSIVE_CHECKS__
    if (texture->id == 0) {
        // TODO: output log here?
        return;
    }
#endif

    const GL_Quad_t *source = sheet->quads + quad;

    GLfloat sx0 = source->x0;
    GLfloat sy0 = source->y0;
    GLfloat sx1 = source->x1;
    GLfloat sy1 = source->y1;

    GLfloat dx0 = destination.x0;
    GLfloat dy0 = destination.y0;
    GLfloat dx1 = destination.x1;
    GLfloat dy1 = destination.y1;

    glBindTexture(GL_TEXTURE_2D, sheet->atlas.id);

    glBegin(GL_TRIANGLE_STRIP);
        glColor4ub(color.r, color.g, color.b, color.a);

        glTexCoord2f(sx0, sy0); // CCW strip, top-left is <0,0> (the face direction of the strip is determined by the winding of the first triangle)
        glVertex2f(dx0, dy0);
        glTexCoord2f(sx0, sy1);
        glVertex2f(dx0, dy1);
        glTexCoord2f(sx1, sy0);
        glVertex2f(dx1, dy0);
        glTexCoord2f(sx1, sy1);
        glVertex2f(dx1, dy1);
    glEnd();
}
