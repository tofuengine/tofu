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

#ifndef __GL_PROGRAM_H__
#define __GL_PROGRAM_H__

#include "common.h"

typedef enum _GL_Program_Command_t {
    WAIT = 0x00000,
    SKIP = 0x10000,
    MOVE = 0x20000,
    MODULO,
    OFFSET,
    COLOR,
    SHIFT,
    GL_Program_Command_t_CountOf
} GL_Program_Command_t;

typedef union _GL_Program_Entry_t {
    GL_Program_Command_t command;
    size_t size;
    GL_Color_t color;
    GL_Pixel_t pixel;
    int integer;
} GL_Program_Entry_t;

typedef struct _GL_Program_t {
    GL_Program_Entry_t *entries;
} GL_Program_t;

extern GL_Program_t *GL_program_create(void);
extern GL_Program_t *GL_program_clone(const GL_Program_t *program);
extern void GL_program_destroy(GL_Program_t *program);

extern void GL_program_clear(GL_Program_t *program);
extern void GL_program_wait(GL_Program_t *program, size_t x, size_t y);
extern void GL_program_modulo(GL_Program_t *program, int amount);
extern void GL_program_offset(GL_Program_t *program, int amount);
extern void GL_program_color(GL_Program_t *program, GL_Pixel_t index, GL_Color_t color);
extern void GL_program_shift(GL_Program_t *program, GL_Pixel_t from, GL_Pixel_t to);

#endif  /* __GL_PROGRAM_H__ */
