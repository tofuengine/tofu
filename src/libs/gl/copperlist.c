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

#include <memory.h>

#define LOG_CONTEXT "gl-copperlist"

GL_CopperList_t *GL_copperlist_create(void)
{
    GL_CopperList_t *copperlist = malloc(sizeof(GL_CopperList_t));
    if (!copperlist) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate copperlist");
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist created at %p", copperlist);

    *copperlist = (GL_CopperList_t){ 0 };

    GL_copperlist_reset(copperlist);

    return copperlist;
}

void GL_copperlist_destroy(GL_CopperList_t *copperlist)
{
    if (copperlist->program) {
        arrfree(copperlist->program);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist program at %p freed", copperlist->program);
    }

    free(copperlist);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist %p freed", copperlist);
}

void GL_copperlist_reset(GL_CopperList_t *copperlist)
{
    GL_palette_generate_greyscale(&copperlist->palette, GL_MAX_PALETTE_COLORS);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "loaded greyscale palettes of %d entries", GL_MAX_PALETTE_COLORS);

    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        copperlist->shifting[i] = (GL_Pixel_t)i;
    }

    if (copperlist->program) {
        arrfree(copperlist->program);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist program at %p freed", copperlist->program);
        copperlist->program = NULL;
    }
}

void GL_copperlist_set_palette(GL_CopperList_t *copperlist, const GL_Palette_t *palette)
{
    copperlist->palette = *palette;
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "palette updated");
}

void GL_copperlist_set_shifting(GL_CopperList_t *copperlist, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count)
{
    if (!from) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            copperlist->shifting[i] = (GL_Pixel_t)i;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            copperlist->shifting[from[i]] = to[i];
        }
    }
}

void GL_copperlist_set_program(GL_CopperList_t *copperlist, const GL_CopperList_Entry_t *program, size_t length)
{
    if (copperlist->program) {
        arrfree(copperlist->program);
        copperlist->program = NULL;
//        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist %p freed", display->copperlist);
    }

    if (program) {
        for (size_t i = 0; i < length; ++i) {
            arrpush(copperlist->program, program[i]);
        }
        // Add a special `WAIT` instruction to halt the Copper(tm) from reading outsize memory boundaries.
        arrpush(copperlist->program, (GL_CopperList_Entry_t){ .command = WAIT });
        arrpush(copperlist->program, (GL_CopperList_Entry_t){ .size = SIZE_MAX });
        arrpush(copperlist->program, (GL_CopperList_Entry_t){ .size = SIZE_MAX });
//        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "updated copperlist %p w/ #%d entries", display->copperlist, entries);
    }
}

static void _surface_to_rgba(const GL_Surface_t *surface, const GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS], const GL_Palette_t *palette, GL_Color_t *pixels)
{
    const GL_Color_t *colors = palette->colors;
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

void _surface_to_rgba_program(const GL_Surface_t *surface, GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS], GL_Palette_t *palette, const GL_CopperList_Entry_t *program, GL_Color_t *pixels)
{
    size_t wait_y = 0, wait_x = 0;
    GL_Color_t *colors = palette->colors;
#ifdef __DEBUG_GRAPHICS__
    const int count = palette->size;
#endif
    int modulo = 0;
    size_t offset = 0; // Always in the range `[0, width)`.

    const GL_CopperList_Entry_t *entry = program;
    const GL_Pixel_t *src = surface->data;
    GL_Color_t *dst_sod = pixels;

    const size_t dwidth = (int)surface->width;
    const size_t dheight = (int)surface->height;

    for (size_t y = 0; y < dheight; ++y) {
        GL_Color_t *dst_eod = dst_sod + dwidth;
        GL_Color_t *dst = dst_sod + offset; // Apply the (wrapped) offset separately on this row pointer to check row "restart".

        for (size_t x = 0; x < dwidth; ++x) {
            // Note: there's no length indicator for the copperlist program. That means that the interpreter would run
            // endlessly (and unsafely read outside memory bounds, causing crashes). To avoid this a "wait forever"
            // trailer is added to the program in the `Display_set_copperlist()` function. This somehow mimics the
            // real Copper(tm) behaviour, where a special `WAIT` instruction `$FFFF, $FFFE` is used to mark the end of
            // the copperlist.
#ifdef __COPPER_ONE_COMMAND_PER_PIXEL__
            if (y >= wait_y && x >= wait_x) {
#else
            while (y >= wait_y && x >= wait_x) {
#endif
                switch ((entry++)->command) {
                    case WAIT: {
                        wait_x = (entry++)->size;
                        wait_y = (entry++)->size;
                        break;
                    }
                    case MODULO: {
                        modulo = (entry++)->integer;
                        break;
                    }
                    case OFFSET: {
                        const int amount = (entry++)->integer; // FIXME: we could add the `dwidth` and spare an addition.
                        // The offset is in the range of a scanline, so we modulo it to spare operations. Note that
                        // we are casting to `int` to avoid integer promotion, as this is a macro!
                        offset = (size_t)IMOD(amount, (int)dwidth);
                        break;
                    }
                    case COLOR: {
                        const GL_Pixel_t index = (entry++)->pixel;
                        const GL_Color_t color = (entry++)->color;
                        colors[index] = color;
                        break;
                    }
                    case SHIFT: {
                        const GL_Pixel_t from = (entry++)->pixel;
                        const GL_Pixel_t to = (entry++)->pixel;
                        shifting[from] = to;
                        break;
                    }
                    default: {
                        break;
                    }
                }
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

void GL_copperlist_surface_to_rgba(const GL_Surface_t *surface, const GL_CopperList_t *copperlist, GL_Color_t *pixels)
{
    if (copperlist->program) {
        GL_Palette_t palette = copperlist->palette; // Make a local copy, the copperlist can change it.
        GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS] = { 0 };
        memcpy(shifting, copperlist->shifting, sizeof(GL_Pixel_t) * GL_MAX_PALETTE_COLORS);

        _surface_to_rgba_program(surface, shifting, &palette, copperlist->program, pixels);
    } else {
        _surface_to_rgba(surface, copperlist->shifting, &copperlist->palette, pixels);
    }
}
