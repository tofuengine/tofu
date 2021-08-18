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

GL_Copperlist_t *GL_copperlist_create(void)
{
    GL_Copperlist_t *copperlist = malloc(sizeof(GL_Copperlist_t));
    if (!copperlist) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate copperlist");
        return NULL;
    }

    GL_Palette_t *palette = GL_palette_create();
    if (!palette) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create palette");
        free(copperlist);
        return NULL;
    }

    *copperlist = (GL_Copperlist_t){
            .palette = palette,
            .shifting = { 0 },
            .program  = NULL
        };
#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist created at %p", copperlist);
#endif  /* VERBOSE_DEBUG */

    GL_copperlist_reset(copperlist);

    return copperlist;
}

void GL_copperlist_destroy(GL_Copperlist_t *copperlist)
{
    if (copperlist->program) {
        GL_program_destroy(copperlist->program);
#ifdef VERBOSE_DEBUG
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist program at %p destroyed", copperlist->program);
#endif  /* VERBOSE_DEBUG */
    }

    GL_palette_destroy(copperlist->palette);
#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist palette %p freed", copperlist->palette);
#endif  /* VERBOSE_DEBUG */

    free(copperlist);
#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist %p freed", copperlist);
#endif  /* VERBOSE_DEBUG */
}

void GL_copperlist_reset(GL_Copperlist_t *copperlist)
{
    GL_copperlist_set_palette(copperlist, NULL);
    GL_copperlist_set_shifting(copperlist, NULL, NULL, 0);
    GL_copperlist_set_program(copperlist, NULL);
}

void GL_copperlist_set_palette(GL_Copperlist_t *copperlist, const GL_Palette_t *palette)
{
    if (palette) {
        GL_palette_copy(copperlist->palette, palette);
#ifdef VERBOSE_DEBUG
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "palette updated");
#endif  /* VERBOSE_DEBUG */
    } else {
        GL_palette_set_greyscale(copperlist->palette, GL_MAX_PALETTE_COLORS);
#ifdef VERBOSE_DEBUG
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "loaded greyscale palettes of %d entries", GL_MAX_PALETTE_COLORS);
#endif  /* VERBOSE_DEBUG */
    }
}

void GL_copperlist_set_shifting(GL_Copperlist_t *copperlist, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count)
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

void GL_copperlist_set_program(GL_Copperlist_t *copperlist, const GL_Program_t *program) // FIXME: make a copy or track the reference? (also for xform)
{
    if (copperlist->program) {
        GL_program_destroy(copperlist->program);
#ifdef VERBOSE_DEBUG
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist program at %p destroyed", copperlist->program);
#endif  /* VERBOSE_DEBUG */
        copperlist->program = NULL;
    }

    if (program) {
        copperlist->program = GL_program_clone(program);
#ifdef VERBOSE_DEBUG
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist program at %p cloned at %p", program, copperlist->program);
#endif  /* VERBOSE_DEBUG */
    }
}

static void _surface_to_rgba(const GL_Surface_t *surface, const GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS], const GL_Color_t colors[GL_MAX_PALETTE_COLORS], GL_Color_t *pixels)
{
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

void _surface_to_rgba_program(const GL_Surface_t *surface, GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS], GL_Color_t colors[GL_MAX_PALETTE_COLORS], const GL_Program_t *program, GL_Color_t *pixels)
{
    size_t wait_y = 0, wait_x = 0;
#ifdef __DEBUG_GRAPHICS__
    const int count = palette->size;
#endif
    int modulo = 0;
    size_t offset = 0; // Always in the range `[0, width)`.

    const GL_Program_Entry_t *entry = program->entries;
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
            // trailer is added to the program in the `Display_set_copperlist()` function. This somehow mimics the
            // real Copper(tm) behaviour, where a special `WAIT` instruction `$FFFF, $FFFE` is used to mark the end of
            // the copperlist.
#ifdef __COPPER_ONE_COMMAND_PER_PIXEL__
            if (y >= wait_y && x >= wait_x) {
#else
            while (y >= wait_y && x >= wait_x) {
#endif
                switch (entry->command) {
                    case GL_PROGRAM_COMMAND_WAIT: {
                        wait_x = entry->args[0].size;
                        wait_y = entry->args[1].size;
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

void GL_copperlist_surface_to_rgba(const GL_Surface_t *surface, const GL_Copperlist_t *copperlist, GL_Color_t *pixels)
{
    if (copperlist->program) {
        GL_Color_t colors[GL_MAX_PALETTE_COLORS] = { 0 }; // Make a local copy, the copperlist can change it.
        memcpy(colors, copperlist->palette->colors, sizeof(GL_Color_t) * GL_MAX_PALETTE_COLORS);
        GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS] = { 0 };
        memcpy(shifting, copperlist->shifting, sizeof(GL_Pixel_t) * GL_MAX_PALETTE_COLORS);

        _surface_to_rgba_program(surface, shifting, colors, copperlist->program, pixels);
    } else {
        _surface_to_rgba(surface, copperlist->shifting, copperlist->palette->colors, pixels);
    }
}