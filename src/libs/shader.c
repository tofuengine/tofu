/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "shader.h"

#include <core/config.h>
#define _LOG_TAG "shader"
#include <libs/log.h>
#include <libs/stb.h>

Shader_t *shader_create(void)
{
    Shader_t *shader = malloc(sizeof(Shader_t));
    if (!shader) {
        LOG_E("can't allocate shader");
        goto error_exit;
    }
#if defined(VERBOSE_DEBUG)
    LOG_D("shader created at %p", shader);
#endif  /* VERBOSE_DEBUG */

    *shader = (Shader_t){ 0 }; // Initialzed the object structure to clear all the fields.

    shader->id = glCreateProgram();
    if (shader->id == 0) {
        LOG_E("can't create shader program");
        goto error_free;
    }
    LOG_D("shader program #%d created", shader->id);

    return shader;

error_free:
    free(shader);
error_exit:
    return NULL;
}

void shader_destroy(Shader_t *shader)
{
    GLint count = 0;
    glGetProgramiv(shader->id, GL_ATTACHED_SHADERS, &count);
    if (count > 0) {
        GLuint shaders[count];
        glGetAttachedShaders(shader->id, count, NULL, shaders);
        for (GLint i = 0; i < count; ++i) {
            glDetachShader(shader->id, shaders[i]);
            LOG_D("shader #%d detached from program #%d", shaders[i], shader->id);
        }
    }

    glDeleteProgram(shader->id);
    LOG_D("shader program #%d deleted", shader->id);

    free(shader->locations); // Safe when passing NULL.
    LOG_D("shader uniforms LUT for program #%d freed", shader->id);

    free(shader);
    LOG_D("shader %p freed", shader);
}

bool shader_attach(Shader_t *shader, const char *code, Shader_Types_t type)
{
#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (shader->id == 0) {
        LOG_W("shader program can't be zero");
        return false;
    }
    if (!code) {
        LOG_W("shader code can't be null");
        return false;
    }
#endif

    GLuint shader_id = glCreateShader(type == SHADER_TYPE_VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
    if (shader_id == 0) {
        LOG_E("can't create shader");
        return false;
    }

    LOG_T("compiling shader\n<SHADER type=\"%d\">\n%s\n</SHADER>", type, code);
    glShaderSource(shader_id, 1, &code, NULL);
    glCompileShader(shader_id);

    GLint success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);

        GLchar description[length]; // FIXME: remove VLAs!!!
        glGetShaderInfoLog(shader_id, length, NULL, description);
        LOG_E("shader compile error: %s", description);
    } else {
        glAttachShader(shader->id, shader_id);

        glLinkProgram(shader->id);

        glGetProgramiv(shader->id, GL_LINK_STATUS, &success);
        if (!success) {
            GLint length = 0;
            glGetProgramiv(shader->id, GL_INFO_LOG_LENGTH, &length);

            GLchar description[length];
            glGetProgramInfoLog(shader->id, length, NULL, description);
            LOG_E("program link error: %s", description);

            glDetachShader(shader->id, shader_id);
        } else {
            LOG_D("shader #%d compiled into program #%d", shader_id, shader->id);
        }
    }

    glDeleteShader(shader_id);

    return success;
}

void shader_prepare(Shader_t *shader, const char *ids[], size_t count)
{
    if (shader->locations) {
        free(shader->locations);
        shader->locations = NULL;
        LOG_D("shader uniforms LUT for program #%d freed", shader->id);
    }
    if (count == 0) {
        LOG_D("no uniforms to prepare for program #%d", shader->id);
        return;
    }
    shader->locations = malloc(sizeof(GLuint) * count);
    for (size_t i = 0; i < count; ++i) {
        GLint location = glGetUniformLocation(shader->id, ids[i]);
        LOG_IF_W(location == -1, "uniform `%s` not found for program #%d", ids[i], shader->id);
        shader->locations[i] = location;
    }
}

// `shader_use` need to be called prior sending data to the program.
void shader_send(const Shader_t *shader, size_t index, Shader_Uniforms_t type, size_t count, const void *value)
{
#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (!shader->locations) {
        LOG_W("program uniforms are not prepared");
        return;
    }
#endif
    GLint location = shader->locations[index];
    if (location == -1) {
#if defined(TOFU_GRAPHICS_REPORT_SHADERS_ERRORS)
        LOG_W("can't find uniform #%d for program #%d", index, shader->id);
#endif
        return;
    }
    switch (type) {
        case SHADER_UNIFORM_BOOL: { glUniform1iv(location, (GLsizei)count, value); } break;
        case SHADER_UNIFORM_INT: { glUniform1iv(location, (GLsizei)count, value); } break;
        case SHADER_UNIFORM_FLOAT: { glUniform1fv(location, (GLsizei)count, value); } break;
        case SHADER_UNIFORM_VEC2: { glUniform2fv(location, (GLsizei)count, value); } break;
        case SHADER_UNIFORM_VEC3: { glUniform3fv(location, (GLsizei)count, value); } break;
        case SHADER_UNIFORM_VEC4: { glUniform4fv(location, (GLsizei)count, value); } break;
        case SHADER_UNIFORM_VEC2I: { glUniform2iv(location, (GLsizei)count, value); } break;
        case SHADER_UNIFORM_VEC3I: { glUniform3iv(location, (GLsizei)count, value); } break;
        case SHADER_UNIFORM_VEC4I: { glUniform4iv(location, (GLsizei)count, value); } break;
        case SHADER_UNIFORM_MAT4: { glUniformMatrix4fv(location, (GLsizei)count, GL_FALSE, value); } break;
        case SHADER_UNIFORM_TEXTURE: { glUniform1iv(location, (GLsizei)count, value); } break;
        default: { } break;
    }
}

void shader_use(const Shader_t *shader)
{
    glUseProgram(shader ? shader->id : 0);
}
