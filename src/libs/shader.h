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

#ifndef TOFU_LIBS_SHADER_H
#define TOFU_LIBS_SHADER_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdbool.h>

typedef struct Shader_s {
    GLuint id;
    GLint *locations;
} Shader_t;

typedef enum Shader_Uniforms_e {
    SHADER_UNIFORM_BOOL,
    SHADER_UNIFORM_INT,
    SHADER_UNIFORM_FLOAT,
    SHADER_UNIFORM_VEC2,
    SHADER_UNIFORM_VEC3,
    SHADER_UNIFORM_VEC4,
    SHADER_UNIFORM_VEC2I,
    SHADER_UNIFORM_VEC3I,
    SHADER_UNIFORM_VEC4I,
    SHADER_UNIFORM_MAT4,
    SHADER_UNIFORM_TEXTURE
} Shader_Uniforms_t;

// TODO: rename to first char uppercase.
extern Shader_t *shader_create(const char *vertex, const char *fragment, const char *ids[], size_t count);
extern void shader_destroy(Shader_t *shader);
extern void shader_send(const Shader_t *shader, size_t index, Shader_Uniforms_t type, size_t count, const void *value);
extern void shader_use(const Shader_t *shader);

#endif  /* TOFU_LIBS_SHADER_H */
