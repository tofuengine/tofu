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

#ifndef __GL_COPPERLIST_H__
#define __GL_COPPERLIST_H__

#include "common.h"
#include "palette.h"
#include "surface.h"

typedef enum _GL_CopperList_Command_t {
    WAIT = 0x00000,
    SKIP = 0x10000,
    MOVE = 0x20000,
    MODULO,
    OFFSET,
    COLOR,
    SHIFT,
    GL_CopperList_Command_t_CountOf
} GL_CopperList_Command_t;

typedef union _GL_CopperList_Entry_t {
    GL_CopperList_Command_t command;
    size_t size;
    GL_Color_t color;
    GL_Pixel_t pixel;
    int integer;
} GL_CopperList_Entry_t;

typedef struct _GL_CopperList_t { // FIXME: rename to something better!!!
    GL_Palette_t palette;
    GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS];
    GL_CopperList_Entry_t *program;
} GL_CopperList_t;

extern GL_CopperList_t *GL_copperlist_create(void);
extern void GL_copperlist_destroy(GL_CopperList_t *copperlist);

extern void GL_copperlist_reset(GL_CopperList_t *copperlist);

extern void GL_copperlist_set_palette(GL_CopperList_t *copperlist, const GL_Palette_t *palette);
extern void GL_copperlist_set_shifting(GL_CopperList_t *copperlist, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count);
extern void GL_copperlist_set_program(GL_CopperList_t *copperlist, const GL_CopperList_Entry_t *program, size_t length);

// TODO: add `GL_program_XXX()` functions.
// typedef GL_CopperList_Entry_t *GL_Program_t;
// GL_program_create();
// GL_program_destroy();
// GL_program_clear(GL_Program_t *program)
// GL_program_wait(GL_Program_t *program, size_t x, size_y)
// GL_program_modulo(GL_Program_t *program, int amount)
// GL_program_offset(GL_Program_t *program, int amount)
// GL_program_color(GL_Program_t *program, GL_Color_t color)
// GL_program_shift(GL_Program_t *program, GL_Pixel_t from, GL_Pixel_t to)

extern void GL_copperlist_surface_to_rgba(const GL_Surface_t *surface, const GL_CopperList_t *copperlist, GL_Color_t *pixels);

#endif  /* __GL_COPPERLIST_H__ */
