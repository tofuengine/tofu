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

bool GL_create_program(GL_Program_t *program, const char *vertex_shader, const char *fragment_shader)
{
    const char *shaders[2] = { vertex_shader, fragment_shader };
    GLuint shader_ids[2] = {};
    bool result = true;

    GLuint program_id = glCreateProgram(); // Compile shaders.
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
            GLint length = 0;
            glGetProgramiv(shader_ids[i], GL_INFO_LOG_LENGTH, &length);

            GLchar description[length];
            glGetShaderInfoLog(shader_ids[i], length, NULL, description);
            Log_write(LOG_LEVELS_ERROR, "<GL> shader compile error: %s", description);
            result = false;
            break;
        }
        glAttachShader(program_id, shader_ids[i]);
    }

    if (result) { // Link shaders into the program.
        glLinkProgram(program_id);
        GLint success;
        glGetShaderiv(program_id, GL_LINK_STATUS, &success);
        if (!success) {
            GLint length = 0;
            glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &length);

            GLchar description[length];
            glGetProgramInfoLog(program_id, length, NULL, description);
            Log_write(LOG_LEVELS_ERROR, "<GL> program link error: %s", description);
            result = false;
        }
    }

    if (result) {
        Log_write(LOG_LEVELS_DEBUG, "<GL> shaders compiled into program #%d", program_id);
        program->id = program_id;
    } else {
        glDeleteProgram(program_id);
    }

    for (int i = 0; i < 2; ++i) {
        if (shader_ids[i] == 0) {
            continue;
        }
        glDeleteShader(shader_ids[i]);
    }

    return result;
}

void GL_delete_program(GL_Program_t *program)
{
    glDeleteProgram(program->id);
    Log_write(LOG_LEVELS_DEBUG, "<GL> shader program #%d deleted", program->id);
    *program = (GL_Program_t){};
}

void GL_use_program(const GL_Program_t *program)
{
    glUseProgram(program->id);
//    Log_write(LOG_LEVELS_TRACE, "<GL> using shader program", program->id);
}
