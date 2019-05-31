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
#include "../memory.h"

GL_Quad_t *precompute_quads(GLuint width, GLuint height, GLuint quad_width, GLuint quad_height)
{
    GLuint columns = width / quad_width;
    GLuint rows = height / quad_height;
    GLuint amount = columns * rows;
    GL_Quad_t *quads = Memory_calloc(amount, sizeof(GL_Quad_t));
    GLuint k = 0;
    for (GLuint i = 0; i < rows; ++i) {
        GLuint y = i * quad_height;
        for (GLuint j = 0; j < columns; ++j) {
            GLuint x = j * quad_width;
            quads[k++] = (GL_Quad_t){ .x0 = x, .y0 = y, .x1 = x + quad_width, .y1 = y + quad_height };
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
    Memory_free(sheet->quads);
    GL_texture_delete(&sheet->atlas);
    Log_write(LOG_LEVELS_DEBUG, "<GL> sheet #%p deleted", sheet);
    *sheet = (GL_Sheet_t){};
}
