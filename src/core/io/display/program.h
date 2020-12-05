/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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

#ifndef __DISPLAY_PROGRAM_H__
#define __DISPLAY_PROGRAM_H__

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdbool.h>

typedef struct _Program_t {
    GLuint id;
    GLint *locations;
} Program_t;

typedef enum _Program_Uniforms_t {
    PROGRAM_UNIFORM_BOOL,
    PROGRAM_UNIFORM_INT,
    PROGRAM_UNIFORM_FLOAT,
    PROGRAM_UNIFORM_VEC2,
    PROGRAM_UNIFORM_VEC3,
    PROGRAM_UNIFORM_VEC4,
    PROGRAM_UNIFORM_VEC2I,
    PROGRAM_UNIFORM_VEC3I,
    PROGRAM_UNIFORM_VEC4I,
    PROGRAM_UNIFORM_TEXTURE
} Program_Uniforms_t;

typedef enum _Program_Shaders_t {
    PROGRAM_SHADER_VERTEX,
    PROGRAM_SHADER_FRAGMENT,
    Program_Shaders_t_CountOf
} Program_Shaders_t;

extern bool program_create(Program_t *program);
extern void program_delete(Program_t *program);
extern bool program_attach(Program_t *program, const char *shader_code, Program_Shaders_t shader_type);
extern void program_prepare(Program_t *program, const char *ids[], size_t count);
extern void program_send(const Program_t *program, size_t index, Program_Uniforms_t type, size_t count, const void *value);
extern void program_use(const Program_t *program);

#endif  /* __DISPLAY_PROGRAM_H__ */
