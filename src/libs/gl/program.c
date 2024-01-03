/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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

#define _LOG_TAG "gl-program"
#include <libs/log.h>
#include <libs/stb.h>

static inline GL_Program_Entry_t *_insert(GL_Program_Entry_t *program, int position, const GL_Program_Entry_t entry)
{
    const int length = arrlen(program);
    const int entries = length - 1;
    const int index = position >= 0 ? position : length + position;
    if (index < entries) { // Overwrite an existing entry, if present.
        program[index] = entry;
    } else { // Otherwise, fill with `NOP` operations and append (moving the end-of-data marker).
        const GL_Program_Entry_t nop = (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_NOP, .args = { { 0 } } };
        for (int i = entries; i < index; ++i) {
            arrins(program, i, nop);
        }
        arrins(program, index, entry);
    }
    return program;
}

GL_Program_t *GL_program_create(void)
{
    GL_Program_t *program = malloc(sizeof(GL_Program_t));
    if (!program) {
        LOG_E("can't allocate program");
        return NULL;
    }
#if defined(VERBOSE_DEBUG)
    LOG_D("program created at %p", program);
#endif  /* VERBOSE_DEBUG */

    *program = (GL_Program_t){ 0 };

    // Add a special `WAIT` instruction to halt the Copper(tm) from reading outsize memory boundaries.
    // We will be adding the other entries at the end of the array, keeping the `WAIT/max/max` terminator.
    const GL_Program_Entry_t end_of_data = (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_WAIT, .args = { { .size = SIZE_MAX }, { .size = SIZE_MAX } } };
    arrpush(program->entries, end_of_data);

    return program;
}

GL_Program_t *GL_program_clone(const GL_Program_t *program)
{
    GL_Program_t *clone = malloc(sizeof(GL_Program_t));
    if (!clone) {
        LOG_E("can't allocate program");
        return NULL;
    }
#if defined(VERBOSE_DEBUG)
    LOG_D("program created at %p", clone);
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
#if defined(VERBOSE_DEBUG)
    LOG_D("program entries at %p freed", program->entries);
#endif  /* VERBOSE_DEBUG */

    free(program);
#if defined(VERBOSE_DEBUG)
    LOG_D("program %p freed", program);
#endif  /* VERBOSE_DEBUG */
}

void GL_program_copy(GL_Program_t *program, const GL_Program_t *other)
{
    size_t length = arrlenu(other->entries);
    arrsetlen(program->entries, length);
    memcpy(program->entries, other->entries, sizeof(GL_Program_Entry_t) * arrlenu(program->entries));
}

void GL_program_clear(GL_Program_t *program)
{
    arrfree(program->entries);
#if defined(VERBOSE_DEBUG)
    LOG_D("program entries at %p freed", program->entries);
#endif  /* VERBOSE_DEBUG */

    // Add a special `WAIT` instruction to halt the Copper(tm) from reading outsize memory boundaries.
    const GL_Program_Entry_t end_of_data = (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_WAIT, .args = { { .size = SIZE_MAX }, { .size = SIZE_MAX } } };
    arrpush(program->entries, end_of_data);
}

void GL_program_erase(GL_Program_t *program, size_t position, size_t length)
{
    arrdeln(program->entries, position, length);
#if defined(VERBOSE_DEBUG)
    LOG_D("program entries at %p freed", program->entries);
#endif  /* VERBOSE_DEBUG */
}

void GL_program_nop(GL_Program_t *program, int position)
{
    program->entries = _insert(program->entries, position,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_NOP, .args = { { 0 } } });
}

void GL_program_wait(GL_Program_t *program, int position, size_t x, size_t y)
{
    program->entries = _insert(program->entries, position,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_WAIT, .args = { { .size = x }, { .size = y } } });
}

void GL_program_skip(GL_Program_t *program, int position, int delta_x, int delta_y)
{
    program->entries = _insert(program->entries, position,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_SKIP, .args = { { .integer = delta_x }, { .integer = delta_y} } });
}

void GL_program_modulo(GL_Program_t *program, int position, int amount)
{
    program->entries = _insert(program->entries, position,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_MODULO, .args = { { .integer = amount } } });
}

void GL_program_offset(GL_Program_t *program, int position, int amount)
{
    program->entries = _insert(program->entries, position,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_OFFSET, .args = { { .integer = amount } } });
}

void GL_program_color(GL_Program_t *program, int position, GL_Pixel_t index, GL_Color_t color)
{
    program->entries = _insert(program->entries, position,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_COLOR, .args = { { .pixel = index }, { .color = color } } });
}

void GL_program_shift(GL_Program_t *program, int position, GL_Pixel_t from, GL_Pixel_t to)
{
    program->entries = _insert(program->entries, position,
            (GL_Program_Entry_t){ .command = GL_PROGRAM_COMMAND_SHIFT, .args = { { .pixel = from }, { .pixel = to } } });
}
