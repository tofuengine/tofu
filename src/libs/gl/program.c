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

#define LOG_CONTEXT "gl-program"

GL_Program_t *GL_program_create(void)
{
    GL_Program_t *program = malloc(sizeof(GL_Program_t));
    if (!program) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate program");
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program created at %p", program);

    *program = (GL_Program_t){ 0 };

    // Add a special `WAIT` instruction to halt the Copper(tm) from reading outsize memory boundaries.
    arrpush(program->entries, (GL_Program_Entry_t){ .command = WAIT });
    arrpush(program->entries, (GL_Program_Entry_t){ .size = SIZE_MAX });
    arrpush(program->entries, (GL_Program_Entry_t){ .size = SIZE_MAX });

    return program;
}

GL_Program_t *GL_program_clone(const GL_Program_t *program)
{
    GL_Program_t *clone = malloc(sizeof(GL_Program_t));
    if (!clone) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate program");
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program created at %p", clone);

    *clone = (GL_Program_t){ 0 };

    GL_Program_Entry_t *current = program->entries;
    for (size_t count = arrlen(program->entries); count; --count) {
        GL_Program_Entry_t entry = *(current++);
        arrpush(clone->entries, entry);
    }

    return clone;
}

void GL_program_destroy(GL_Program_t *program)
{
    arrfree(program->entries);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program entries at %p freed", program->entries);

    free(program);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program %p freed", program);
}

void GL_program_clear(GL_Program_t *program)
{
    arrfree(program->entries);
    program->entries = NULL;
//    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist %p freed", display->copperlist);

    // Add a special `WAIT` instruction to halt the Copper(tm) from reading outsize memory boundaries.
    arrpush(program->entries, (GL_Program_Entry_t){ .command = WAIT });
    arrpush(program->entries, (GL_Program_Entry_t){ .size = SIZE_MAX });
    arrpush(program->entries, (GL_Program_Entry_t){ .size = SIZE_MAX });
}

void GL_program_wait(GL_Program_t *program, size_t x, size_t y)
{
    size_t count = arrlen(program->entries);
    arrinsn(program->entries, count - 3, 3);
    count = arrlen(program->entries);
    program->entries[0] = (GL_Program_Entry_t){ .command = WAIT };
    program->entries[1] = (GL_Program_Entry_t){ .size = x };
    program->entries[2] = (GL_Program_Entry_t){ .size = y };
}

void GL_program_modulo(GL_Program_t *program, int amount)
{
    size_t count = arrlen(program->entries);
    arrinsn(program->entries, count - 3, 2);
    program->entries[0] = (GL_Program_Entry_t){ .command = MODULO };
    program->entries[1] = (GL_Program_Entry_t){ .integer = amount };
}

void GL_program_offset(GL_Program_t *program, int amount)
{
    size_t count = arrlen(program->entries);
    arrinsn(program->entries, count - 3, 2);
    program->entries[0] = (GL_Program_Entry_t){ .command = OFFSET };
    program->entries[1] = (GL_Program_Entry_t){ .integer = amount };
}

void GL_program_color(GL_Program_t *program, GL_Pixel_t index, GL_Color_t color)
{
    size_t count = arrlen(program->entries);
    arrinsn(program->entries, count - 3, 3);
    program->entries[0] = (GL_Program_Entry_t){ .command = COLOR };
    program->entries[1] = (GL_Program_Entry_t){ .pixel = index };
    program->entries[2] = (GL_Program_Entry_t){ .color = color };
}

void GL_program_shift(GL_Program_t *program, GL_Pixel_t from, GL_Pixel_t to)
{
    size_t count = arrlen(program->entries);
    arrinsn(program->entries, count - 3, 3);
    program->entries[0] = (GL_Program_Entry_t){ .command = SHIFT };
    program->entries[1] = (GL_Program_Entry_t){ .pixel = from };
    program->entries[2] = (GL_Program_Entry_t){ .pixel = to };
}
