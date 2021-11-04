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

#include "copperlist.h"

#include <libs/imath.h>
#include <libs/log.h>
#include <libs/stb.h>

#define LOG_CONTEXT "gl-copperlist"

GL_Copperlist_t *GL_copperlist_create(void)
{
    GL_Copperlist_t *copperlist = malloc(sizeof(GL_Copperlist_t));
    if (!copperlist) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate copperlist");
        return NULL;
    }

    *copperlist = (GL_Copperlist_t){ 0 };
#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist created at %p", copperlist);
#endif  /* VERBOSE_DEBUG */

    GL_copperlist_reset(copperlist);

    return copperlist;
}

void GL_copperlist_destroy(GL_Copperlist_t *copperlist)
{
    if (copperlist->entries) {
        arrfree(copperlist->entries);
#ifdef VERBOSE_DEBUG
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist entries at %p freed", copperlist->entries);
#endif  /* VERBOSE_DEBUG */
    }

    free(copperlist);
#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist %p freed", copperlist);
#endif  /* VERBOSE_DEBUG */
}

void GL_copperlist_reset(GL_Copperlist_t *copperlist)
{
    // Palette is not part of the "reset" operation.
    GL_copperlist_set_shifting(copperlist, NULL, NULL, 0);
    GL_copperlist_set_program(copperlist, NULL);
}

void GL_copperlist_set_palette(GL_Copperlist_t *copperlist, const GL_Palette_t *palette)
{
    GL_palette_get_colors(palette, copperlist->state.colors);
#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "palette updated");
#endif  /* VERBOSE_DEBUG */
}

// TODO: change API, accepting a single array with successive from/to pairs.
void GL_copperlist_set_shifting(GL_Copperlist_t *copperlist, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count)
{
    if (!from) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            copperlist->state.shifting[i] = (GL_Pixel_t)i;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            copperlist->state.shifting[from[i]] = to[i];
        }
    }
}

static void _surface_to_rgba(const GL_Surface_t *surface, GL_Color_t *pixels, const Copperlist_State_t *state, GL_Program_Entry_t *entries)
{
    const GL_Color_t *colors = state->colors;
    const GL_Pixel_t *shifting = state->shifting;

#ifdef __DEBUG_GRAPHICS__
    const int count = palette->size;
#endif

    const size_t data_size = surface->data_size;

    const GL_Pixel_t *src = surface->data;
    GL_Color_t *dst = pixels;

    for (size_t i = data_size; i; --i) {
        const GL_Pixel_t index = shifting[*(src++)];
#ifdef __DEBUG_GRAPHICS__
        GL_Color_t color;
        if (index >= count) {
            const int y = (index - 240) * 8;
            color = (GL_Color_t){ 0, 63 + y, 0, 255 };
        } else {
            color = colors[index];
        }
        *(dst++) = color;
#else
        *(dst++) = colors[index];
#endif
    }
}

// TODO: use array of function pointers instead of mega-switch?
// TODO: ditch wait-x? copperlist operations changes only once per scanline?
void _surface_to_rgba_program(const GL_Surface_t *surface, GL_Color_t *pixels, const Copperlist_State_t *state, GL_Program_Entry_t *entries)
{
    Copperlist_State_t aux = *state; // Make a local copy, the copperlist could change it.
    GL_Color_t *colors = aux.colors;
    GL_Pixel_t *shifting = aux.shifting;

    size_t wait_y = 0, wait_x = 0;
#ifdef __DEBUG_GRAPHICS__
    const int count = palette->size;
#endif
    int modulo = 0;
    size_t offset = 0; // Always in the range `[0, width)`.

    const GL_Program_Entry_t *entry = entries;
    const GL_Pixel_t *src = surface->data;
    GL_Color_t *dst_sod = pixels;

    const size_t dwidth = surface->width;
    const size_t dheight = surface->height;

    for (size_t y = 0; y < dheight; ++y) {
        GL_Color_t *dst_eod = dst_sod + dwidth;
        GL_Color_t *dst = dst_sod + offset; // Apply the (wrapped) offset separately on this row pointer to check row "restart".

        for (size_t x = 0; x < dwidth; ++x) {
            // Note: there's no length indicator for the copperlist program. That means that the interpreter would run
            // endlessly (and unsafely read outside memory bounds, causing crashes). To avoid this a "wait forever"
            // trailer is added to the program in the `GL_program_create()` and `GL_program_reset()` functions.
            // This somehow mimics the real Copper(tm) behaviour, where a special `WAIT` instruction `$FFFF, $FFFE`
            // is used to mark the end of the copperlist.
#ifdef __COPPER_ONE_COMMAND_PER_PIXEL__
            if (y >= wait_y && x >= wait_x) {
#else
            while (y >= wait_y && x >= wait_x) {
#endif
                switch (entry->command) {
                    case GL_PROGRAM_COMMAND_NOP: {
                        break;
                    }
                    case GL_PROGRAM_COMMAND_WAIT: {
                        wait_x = entry->args[0].size;
                        wait_y = entry->args[1].size;
                        break;
                    }
                    case GL_PROGRAM_COMMAND_SKIP: {
                        wait_x += entry->args[0].size;
                        wait_y += entry->args[1].size;
                        break;
                    }
                    case GL_PROGRAM_COMMAND_MODULO: {
                        modulo = entry->args[0].integer;
                        break;
                    }
                    case GL_PROGRAM_COMMAND_OFFSET: {
                        // The offset is in the range of a scanline, so we modulo it to spare operations. Note that
                        // we are casting to `int` to avoid integer promotion, as this is a macro!
                        offset = (size_t)IMOD(entry->args[0].integer, (int)dwidth);
                        break;
                    }
                    case GL_PROGRAM_COMMAND_COLOR: {
                        const GL_Pixel_t index = entry->args[0].pixel;
                        const GL_Color_t color = entry->args[1].color;
                        colors[index] = color;
                        break;
                    }
                    case GL_PROGRAM_COMMAND_SHIFT: {
                        const GL_Pixel_t from = entry->args[0].pixel;
                        const GL_Pixel_t to = entry->args[1].pixel;
                        shifting[from] = to;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                ++entry;
            }

            const GL_Pixel_t index = shifting[*(src++)];
#ifdef __DEBUG_GRAPHICS__
            GL_Color_t color;
            if (index >= count) {
                const int y = (index - 240) * 8;
                color = (GL_Color_t){ 0, 63 + y, 0, 255 };
            } else {
                color = colors[index];
            }
            *(dst++) = color;
#else
            *(dst++) = colors[index];
#endif
            if (dst == dst_eod) { // Wrap on end-of-data. Check for equality since we are copy one pixel at time.
                dst = dst_sod;
            }
        }

        src += modulo;
        dst_sod += dwidth;
    }
}

static inline GL_Program_Entry_t *_copy(GL_Program_Entry_t *entries, const GL_Program_t *program)
{
    size_t length = arrlenu(program->entries);
    arrsetlen(entries, length);
    memcpy(entries, program->entries, sizeof(GL_Program_Entry_t) * length);
    return entries;
}

// FIXME: make a copy or track the reference? (also for xform and palettes)
void GL_copperlist_set_program(GL_Copperlist_t *copperlist, const GL_Program_t *program)
{
    if (program) {
#ifdef VERBOSE_DEBUG
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist program at %p copied at entries %p", program, copperlist->entries);
#endif  /* VERBOSE_DEBUG */
        copperlist->entries = _copy(copperlist->entries, program);
    } else {
       arrfree(copperlist->entries);
#ifdef VERBOSE_DEBUG
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist entries at %p freed", copperlist->entries);
#endif  /* VERBOSE_DEBUG */
        copperlist->entries = NULL;
    }

    copperlist->surface_to_rgba = program ? _surface_to_rgba_program : _surface_to_rgba;
}

void GL_copperlist_surface_to_rgba(const GL_Copperlist_t *copperlist, const GL_Surface_t *surface, GL_Color_t *pixels)
{
    copperlist->surface_to_rgba(surface, pixels, &copperlist->state, copperlist->entries);
}
