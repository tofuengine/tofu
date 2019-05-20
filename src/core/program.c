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

#include "program.h"

#include "../log.h"

Program_t Program_create(const char *vertex_shader, const char *fragment_shader)
{
    const char *shaders[2] = { vertex_shader, fragment_shader };
    GLuint shader_ids[2] = {};
    bool result = true;

    GLuint program_id = glCreateProgram();
    for (int i = 0; i < 2; ++i) {
        if (!shaders[i]) {
            continue;
        }
        shader_ids[i] = glCreateShader(i == 0 ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
        glShaderSource(shader_ids[i], 1, &shaders[i], NULL);
        glCompileShader(shader_ids[i]);

        GLint success;
        glGetShaderiv(shader_ids[i], GL_COMPILE_STATUS, &success);
        if (!success) {
            GLchar description[512] = {};
            glGetShaderInfoLog(shader_ids[i], 512, NULL, description);
            Log_write(LOG_LEVELS_ERROR, "<PROGRAM> shader error: %s", description);
            result = false;
            break;
        }
        glAttachShader(program_id, shader_ids[i]);
    }

    if (result) {
        glLinkProgram(program_id);
    } else {
        glDeleteProgram(program_id);
        program_id = 0;
    }

    for (int i = 0; i < 2; ++i) {
        if (shader_ids[i] == 0) {
            continue;
        }
        glDeleteShader(shader_ids[i]);
    }

    return (Program_t){ .id = program_id };
}

void Program_destroy(Program_t *program)
{
    glDeleteProgram(program->id);
    *program = (Program_t){};
}

void Program_use(const Program_t *program)
{
    glUseProgram(program->id);
}
