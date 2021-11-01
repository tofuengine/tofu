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

static inline GL_Program_Entry_t *_insert(GL_Program_Entry_t *program, int position, const GL_Program_Entry_t entry)
{
    const size_t length = arrlenu(program);
    size_t offset = position >= 0 ? (size_t)position : length + position;
    arrins(program, offset, entry);
    return program;
}

GL_Program_t *GL_program_create(void)
{
    GL_Program_t *program = malloc(sizeof(GL_Program_t));
    if (!program) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate program");
        return NULL;
    }
#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program created at %p", program);
#endif  /* VERBOSE_DEBUG */

    *program = (GL_Program_t){ 0 };

    // Add a special `WAIT` instruction to halt the Copper(tm) from reading outsize memory boundaries.
    // We will be adding the other entries at the end of the array, keeping the `WAIT/max/max` terminator.
    program->entries = _insert(NULL, 0,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_WAIT, .args = { { .size = SIZE_MAX }, { .size = SIZE_MAX } } });

    return program;
}

GL_Program_t *GL_program_clone(const GL_Program_t *program)
{
    GL_Program_t *clone = malloc(sizeof(GL_Program_t));
    if (!clone) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate program");
        return NULL;
    }
#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program created at %p", clone);
#endif  /* VERBOSE_DEBUG */

    *clone = (GL_Program_t){ 0 };

    const GL_Program_Entry_t *current = program->entries;
    for (size_t count = arrlenu(program->entries); count; --count) {
        GL_Program_Entry_t entry = *(current++);
        arrpush(clone->entries, entry);
    }

    return clone;
}

void GL_program_destroy(GL_Program_t *program)
{
    arrfree(program->entries);
#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program entries at %p freed", program->entries);
#endif  /* VERBOSE_DEBUG */

    free(program);
#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program %p freed", program);
#endif  /* VERBOSE_DEBUG */
}

void GL_program_clear(GL_Program_t *program)
{
    arrfree(program->entries);
#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program entries at %p freed", program->entries);
#endif  /* VERBOSE_DEBUG */
    program->entries = NULL;

    // Add a special `WAIT` instruction to halt the Copper(tm) from reading outsize memory boundaries.
    program->entries = _insert(NULL, 0,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_WAIT, .args = { { .size = SIZE_MAX }, { .size = SIZE_MAX } } });
}

void GL_program_wait(GL_Program_t *program, size_t x, size_t y)
{
    program->entries = _insert(program->entries, -1,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_WAIT, .args = { { .size = x }, { .size = y } } });
}

void GL_program_modulo(GL_Program_t *program, int amount)
{
    program->entries = _insert(program->entries, -1,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_MODULO, .args = { { .integer = amount } } });
}

void GL_program_offset(GL_Program_t *program, int amount)
{
    program->entries = _insert(program->entries, -1,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_OFFSET, .args = { { .integer = amount } } });
}

void GL_program_color(GL_Program_t *program, GL_Pixel_t index, GL_Color_t color)
{
    program->entries = _insert(program->entries, -1,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_COLOR, .args = { { .pixel = index }, { .color = color } } });
}

void GL_program_shift(GL_Program_t *program, GL_Pixel_t from, GL_Pixel_t to)
{
    program->entries = _insert(program->entries, -1,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_SHIFT, .args = { { .pixel = from }, { .pixel = to } } });
}
