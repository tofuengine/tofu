/*
 * MIT License
 * 
 * Copyright (c) 2019-2021 Marco Lizza
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
 */

#include "program.h"

#include <libs/log.h>
#include <libs/stb.h>

#define LOG_CONTEXT "program"

bool program_create(Program_t *program)
{
    *program = (Program_t){ 0 }; // Initialzed the object structure to clear all the fields.

    program->id = glCreateProgram();
    if (program->id == 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create shader program");
        return false;
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shader program #%d created", program->id);

    return true;
}

void program_delete(Program_t *program)
{
    GLint count = 0;
    glGetProgramiv(program->id, GL_ATTACHED_SHADERS, &count);
    if (count > 0) {
        GLuint shaders[count];
        glGetAttachedShaders(program->id, count, NULL, shaders);
        for (GLint i = 0; i < count; ++i) {
            glDetachShader(program->id, shaders[i]);
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shader #%d detached from program #%d", shaders[i], program->id);
        }
    }

    glDeleteProgram(program->id);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shader program #%d deleted", program->id);

    free(program->locations); // Safe when passing NULL.
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shader uniforms LUT for program #%d freed", program->id);
}

bool program_attach(Program_t *program, const char *shader_code, Program_Shaders_t shader_type)
{
#ifdef __DEFENSIVE_CHECKS__
    if (program->id == 0) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "shader program can't be zero");
        return false;
    }
    if (!shader_code) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "shader code can't be null");
        return false;
    }
#endif

    GLuint shader_id = glCreateShader(shader_type == PROGRAM_SHADER_VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
    if (shader_id == 0) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create shader");
        return false;
    }

    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "compiling shader\n<SHADER type=\"%d\">\n%s\n</SHADER>", shader_type, shader_code);
    glShaderSource(shader_id, 1, &shader_code, NULL);
    glCompileShader(shader_id);

    GLint success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);

        GLchar description[length];
        glGetShaderInfoLog(shader_id, length, NULL, description);
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "shader compile error: %s", description);
    } else {
        glAttachShader(program->id, shader_id);

        glLinkProgram(program->id);

        glGetProgramiv(program->id, GL_LINK_STATUS, &success);
        if (!success) {
            GLint length = 0;
            glGetProgramiv(program->id, GL_INFO_LOG_LENGTH, &length);

            GLchar description[length];
            glGetProgramInfoLog(program->id, length, NULL, description);
            Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "program link error: %s", description);

            glDetachShader(program->id, shader_id);
        } else {
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shader #%d compiled into program #%d", shader_id, program->id);
        }
    }

    glDeleteShader(shader_id);

    return success;
}

void program_prepare(Program_t *program, const char *ids[], size_t count)
{
    if (program->locations) {
        free(program->locations);
        program->locations = NULL;
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shader uniforms LUT for program #%d freed", program->id);
    }
    if (count == 0) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "no uniforms to prepare for program #%d", program->id);
        return;
    }
    program->locations = malloc(sizeof(GLuint) * count);
    for (size_t i = 0; i < count; ++i) {
        GLint location = glGetUniformLocation(program->id, ids[i]);
        Log_assert(location != -1, LOG_LEVELS_WARNING, LOG_CONTEXT, "uniform `%s` not found for program #%d", ids[i], program->id);
        program->locations[i] = location;
    }
}

// `program_use` need to be called prior sending data to the program.
void program_send(const Program_t *program, size_t index, Program_Uniforms_t type, size_t count, const void *value)
{
#ifdef __DEFENSIVE_CHECKS__
    if (!program->locations) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "program uniforms are not prepared");
        return;
    }
#endif
    GLint location = program->locations[index];
    if (location == -1) {
#ifdef __DEBUG_SHADER_CALLS__
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "can't find uniform `%s` for program #%d", id, program->id);
#endif
        return;
    }
    switch (type) {
        case PROGRAM_UNIFORM_BOOL: { glUniform1iv(location, (GLsizei)count, value); } break;
        case PROGRAM_UNIFORM_INT: { glUniform1iv(location, (GLsizei)count, value); } break;
        case PROGRAM_UNIFORM_FLOAT: { glUniform1fv(location, (GLsizei)count, value); } break;
        case PROGRAM_UNIFORM_VEC2: { glUniform2fv(location, (GLsizei)count, value); } break;
        case PROGRAM_UNIFORM_VEC3: { glUniform3fv(location, (GLsizei)count, value); } break;
        case PROGRAM_UNIFORM_VEC4: { glUniform4fv(location, (GLsizei)count, value); } break;
        case PROGRAM_UNIFORM_VEC2I: { glUniform2iv(location, (GLsizei)count, value); } break;
        case PROGRAM_UNIFORM_VEC3I: { glUniform3iv(location, (GLsizei)count, value); } break;
        case PROGRAM_UNIFORM_VEC4I: { glUniform4iv(location, (GLsizei)count, value); } break;
        case PROGRAM_UNIFORM_TEXTURE: { glUniform1iv(location, (GLsizei)count, value); } break;
        default: { } break;
    }
}

void program_use(const Program_t *program)
{
    glUseProgram(program->id);
}
