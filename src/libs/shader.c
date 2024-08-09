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

static const GLenum _index_to_shader_type[] = {
    GL_VERTEX_SHADER,
    GL_FRAGMENT_SHADER
};

static GLuint _compile_shader(GLenum type, const char *code)
{
    GLuint shader_id = glCreateShader(type);
    if (shader_id == 0) {
        LOG_E("can't create shader w/ type %d", type);
        goto error_exit;
    }

    LOG_T("loading source for shader w/ id %d\n%s", shader_id, code);
    glShaderSource(shader_id, 1, &code, NULL);

    LOG_T("compiling shader %d", shader_id);
    glCompileShader(shader_id);

    GLint success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);

        GLchar description[length]; // FIXME: remove VLAs!!!
        glGetShaderInfoLog(shader_id, length, NULL, description);
        LOG_E("shader %d compile error: %s", shader_id, description);
        goto error_delete_shader;
    }

    return shader_id;

error_delete_shader:
    glDeleteShader(shader_id);
error_exit:
    return 0;
}

static bool _link_shader_program(GLuint program_id)
{
    glLinkProgram(program_id);

    GLint success;
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (success) {
        return true;
    }

    GLint length = 0;
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &length);

    GLchar description[length];
    glGetProgramInfoLog(program_id, length, NULL, description);
    LOG_E("program link error: %s", description);
    return false;
}

static GLint *_prepare(GLuint program_id, const char *ids[], size_t count)
{
    if (count == 0) {
        LOG_W("no uniforms to prepare for program #%d", program_id);
        return NULL;
    }
    GLint *locations = malloc(sizeof(GLuint) * count);
    if (!locations) {
        LOG_E("can't allocate shader uniforms LUT for program #%d", program_id);
        return NULL;
    }
    for (size_t i = 0; i < count; ++i) {
        GLint location = glGetUniformLocation(program_id, ids[i]);
        LOG_IF_W(location == -1, "uniform `%s` not found for shader program #%d", ids[i], program_id);
        locations[i] = location;
    }
    return locations;
}

Shader_t *shader_create(const char *codes[2], const char *ids[], size_t count)
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

    GLuint program_id = glCreateProgram();
    if (program_id == 0) {
        LOG_E("can't create shader program");
        goto error_free_shader;
    }
    LOG_D("shader program #%d created", program_id);

    GLuint shader_ids[2] = { 0 };
    for (size_t i = 0; i < 2; ++i) {
        const GLenum type = _index_to_shader_type[i];

        shader_ids[i] = _compile_shader(type, codes[i]);
        if (shader_ids[i] == 0) {
            LOG_E("can't compile shader w/ type %d", type);
            goto error_delete_shaders;
        }
    }

    for (size_t i = 0; i < 2; ++i) {
        glAttachShader(program_id, shader_ids[i]);
        LOG_D("shader %d attached to program #%d", shader_ids[i], program_id);
    }

    bool linked = _link_shader_program(program_id);
    if (!linked) {
        LOG_E("can't link shader program");
        goto error_detach_shaders;
    }

    GLint *locations = _prepare(program_id, ids, count);
    if (!locations) {
        LOG_E("can't prepare shader program");
        goto error_detach_shaders;
    }

    shader->id = program_id;
    shader->locations = locations;
    LOG_D("shaders compiled into program #%d", program_id);

    return shader;

error_detach_shaders:
    for (size_t i = 0; i < 2; ++i) {
        GLuint shader_id = shader_ids[i];
        if (shader_id == 0) {
            continue;
        }
        glDetachShader(program_id, shader_id);
    }
error_delete_shaders:
    for (size_t i = 0; i < 2; ++i) {
        GLuint shader_id = shader_ids[i];
        if (shader_id == 0) {
            continue;
        }
        glDeleteShader(shader_id);
    }
error_free_shader:
    free(shader);
error_exit:
    return false;
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
            glDeleteShader(shaders[i]);
            LOG_D("shader #%d detached deleted", shaders[i]);
        }
    }

    glDeleteProgram(shader->id);
    LOG_D("shader program #%d deleted", shader->id);

    free(shader->locations);
    LOG_D("shader uniforms LUT for program #%d freed", shader->id);

    free(shader);
    LOG_D("shader %p freed", shader);
}

// `shader_use` need to be called prior sending data to the program.
void shader_send(const Shader_t *shader, size_t index, Shader_Uniforms_t type, size_t count, const void *value)
{
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
        default: { /* Nothing to do */ } break;
    }
}

void shader_use(const Shader_t *shader)
{
    glUseProgram(shader ? shader->id : 0);
}
