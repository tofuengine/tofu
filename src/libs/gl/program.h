/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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

#ifndef __GL_PROGRAM_H__
#define __GL_PROGRAM_H__

#include "common.h"

typedef enum GL_Program_Command_e {
    GL_PROGRAM_COMMAND_NOP,
    GL_PROGRAM_COMMAND_WAIT,
    GL_PROGRAM_COMMAND_SKIP,
    GL_PROGRAM_COMMAND_MODULO,
    GL_PROGRAM_COMMAND_OFFSET,
    GL_PROGRAM_COMMAND_COLOR,
    GL_PROGRAM_COMMAND_SHIFT,
    GL_Program_Command_t_CountOf
} GL_Program_Command_t;

typedef struct GL_Program_Entry_s {
    GL_Program_Command_t command;
    union {
        size_t size;
        GL_Color_t color;
        GL_Pixel_t pixel;
        int integer;
    } args[2];
} GL_Program_Entry_t;

typedef struct GL_Program_s {
    GL_Program_Entry_t *entries;
} GL_Program_t;

extern GL_Program_t *GL_program_create(void);
extern GL_Program_t *GL_program_clone(const GL_Program_t *program);
extern void GL_program_destroy(GL_Program_t *program);

extern void GL_program_copy(GL_Program_t *program, const GL_Program_t *other);
extern void GL_program_clear(GL_Program_t *program);
extern void GL_program_erase(GL_Program_t *program, size_t position, size_t length);

extern void GL_program_nop(GL_Program_t *program, int position);
extern void GL_program_wait(GL_Program_t *program, int position, size_t x, size_t y);
extern void GL_program_skip(GL_Program_t *program, int position, int delta_x, int delta_y);
extern void GL_program_modulo(GL_Program_t *program, int position, int amount);
extern void GL_program_offset(GL_Program_t *program, int position, int amount);
extern void GL_program_color(GL_Program_t *program, int position, GL_Pixel_t index, GL_Color_t color);
extern void GL_program_shift(GL_Program_t *program, int position, GL_Pixel_t from, GL_Pixel_t to);

#endif  /* __GL_PROGRAM_H__ */
