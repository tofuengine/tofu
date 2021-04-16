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

void GL_surface_to_rgba_copperlist(const GL_Surface_t *surface, int bias, GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS],
    GL_Palette_t *palette, const GL_CopperList_Entry_t *copperlist, GL_Color_t *pixels)
{
    size_t wait_y = 0, wait_x = 0;
    GL_Color_t *colors = palette->colors;
#ifdef __DEBUG_GRAPHICS__
    const int count = palette->size;
#endif
    int modulo = 0;
    size_t offset = 0; // Always in the range `[0, width)`.

    const GL_CopperList_Entry_t *entry = copperlist;
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
                    case BIAS: {
                        bias = (entry++)->integer;
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

            const GL_Pixel_t index = shifting[*(src++) + bias];
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
