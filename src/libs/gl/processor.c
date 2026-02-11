/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2026 Marco Lizza
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

#include "processor.h"

#include <core/config.h>
#include <libs/imath.h>
#define _LOG_TAG "gl-processor"
#include <libs/log.h>
#include <libs/stb.h>

GL_Processor_t *GL_processor_create(void)
{
    GL_Processor_t *processor = malloc(sizeof(GL_Processor_t));
    if (!processor) {
        LOG_E("can't allocate processor");
        return NULL;
    }

    *processor = (GL_Processor_t){ 0 };
#if defined(TOFU_CORE_VERBOSE_DEBUG)
    LOG_D("processor created at %p", processor);
#endif  /* TOFU_CORE_VERBOSE_DEBUG */

#if defined(TOFU_GRAPHICS_DEFAULT_PALETTE_IS_QUANTIZED)
    LOG_W("setting default to %d color(s) quantized palette", GL_PALETTE_MAX_COLORS);
    #if GL_PALETTE_MAX_COLORS == 256
    GL_palette_set_quantized(processor->state.palette, 3, 3, 2);
    #elif GL_PALETTE_MAX_COLORS == 128
    GL_palette_set_quantized(processor->state.palette, 2, 3, 2);
    #elif GL_PALETTE_MAX_COLORS == 64
    GL_palette_set_quantized(processor->state.palette, 2, 2, 2);
    #elif GL_PALETTE_MAX_COLORS == 32
    GL_palette_set_quantized(processor->state.palette, 2, 2, 1);
    #elif GL_PALETTE_MAX_COLORS == 16
    GL_palette_set_quantized(processor->state.palette, 1, 2, 1);
    #elif GL_PALETTE_MAX_COLORS == 8
    GL_palette_set_quantized(processor->state.palette, 1, 1, 1);
    #else
        #error "Too few palette entries"
    #endif
#else
    LOG_W("setting default to %d color(s) greyscale palette", GL_PALETTE_MAX_COLORS);
    GL_palette_set_greyscale(processor->state.palette, GL_PALETTE_MAX_COLORS);
#endif

    GL_processor_reset(processor);

    return processor;
}

void GL_processor_destroy(GL_Processor_t *processor)
{
#if defined(TOFU_CORE_VERBOSE_DEBUG)
    LOG_D("destroying processor %p", processor);
#endif  /* TOFU_CORE_VERBOSE_DEBUG */

    if (processor->state.program) {
        GL_program_destroy(processor->state.program);
#if defined(TOFU_CORE_VERBOSE_DEBUG)
        LOG_D("processor program %p destroyed", processor->state.program);
#endif  /* TOFU_CORE_VERBOSE_DEBUG */
    }

    free(processor);
#if defined(TOFU_CORE_VERBOSE_DEBUG)
    LOG_D("processor freed");
#endif  /* TOFU_CORE_VERBOSE_DEBUG */
}

void GL_processor_reset(GL_Processor_t *processor)
{
    // Palette is not part of the "reset" operation.
    GL_processor_set_shifting(processor, NULL, NULL, 0);
    GL_processor_set_program(processor, NULL);
}

const GL_Color_t *GL_processor_get_palette(const GL_Processor_t *processor)
{
    return processor->state.palette;
}

void GL_processor_set_palette(GL_Processor_t *processor, const GL_Color_t *palette)
{
    GL_palette_copy(processor->state.palette, palette);
#if defined(TOFU_CORE_VERBOSE_DEBUG)
    LOG_D("palette copied");
#endif  /* TOFU_CORE_VERBOSE_DEBUG */
}

// TODO: change API, accepting a single array with successive from/to pairs.
void GL_processor_set_shifting(GL_Processor_t *processor, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count)
{
    if (!from) {
        for (size_t i = 0; i < GL_PALETTE_MAX_COLORS; ++i) {
            processor->state.shifting[i] = (GL_Pixel_t)i;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            processor->state.shifting[from[i]] = to[i];
        }
    }
}

static void _surface_to_pixels(const GL_Processor_State_t *state, const GL_Surface_t *surface, GL_Color_t *pixels)
{
    const GL_Color_t *palette = state->palette;
    const GL_Pixel_t *shifting = state->shifting;

#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
    const int count = processor->palette->size;
#endif

    const size_t data_size = surface->data_size;

    const GL_Pixel_t *src = surface->data;
    GL_Color_t *dst = pixels;

    for (size_t i = data_size; i; --i) {
        const GL_Pixel_t index = shifting[*(src++)];
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
        GL_Color_t color;
        if (index >= count) {
            const int y = (index - 240) * 8;
            color = (GL_Color_t){ 0, 63 + y, 0, 255 };
        } else {
            color = palette[index];
        }
        *(dst++) = color;
#else
        *(dst++) = palette[index];
#endif
    }
}

// TODO: use array of function pointers instead of mega-switch?
// TODO: ditch wait-x? processor operations changes only once per scanline?
void _surface_to_pixels_program(const GL_Processor_State_t *state, const GL_Surface_t *surface, GL_Color_t *pixels)
{
    GL_Color_t palette[GL_PALETTE_MAX_COLORS];
    GL_Pixel_t shifting[GL_PALETTE_MAX_COLORS];
    memcpy(palette, state->palette, sizeof(GL_Color_t) * GL_PALETTE_MAX_COLORS); // Make a local copy, the processor could change it.
    memcpy(shifting, state->shifting, sizeof(GL_Pixel_t) * GL_PALETTE_MAX_COLORS);

    size_t wait_index = 0;
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
    const int count = processor->palette->size;
#endif
    int src_modulo = 0;
    size_t dst_offset = 0; // Always in the range `[0, width)`.

    const GL_Program_Entry_t *entry = state->program->entries;
    const GL_Pixel_t *src = surface->data;
    GL_Color_t *dst_sod = pixels;

    const size_t dwidth = surface->width;
    const size_t dheight = surface->height;

    // The raster index is the absolute pixel index in the destination surface. It is incremented for each
    // pixel drawn, and used to check against the current `WAIT` target (i.e. `wait_index`).
    //
    // Prior transferring a pixel to the destination, all the processor commands are executed until the next `WAIT`
    // target is reached (or exceeded).
    //
    // Note: there's no explicit length indicator for the processor program. That means that the inner wait-loop
    // would run endlessly (and unsafely read outside memory bounds, causing crashes). To avoid this, a "wait forever"
    // trailer is added to the program by the `GL_program_create()` and `GL_program_reset()` functions.
    // This somehow mimics the real Copper(tm) behaviour, where a special `WAIT` instruction `$FFFF, $FFFE`
    // is used to mark the end of the processor.
    size_t raster_index = 0;
    for (size_t h = dheight; h; --h) {
        GL_Color_t *dst_eod = dst_sod + dwidth;
        GL_Color_t *dst = dst_sod + dst_offset; // Apply the (wrapped) offset separately on this row pointer to check row "restart".

        for (size_t w = dwidth; w; --w) {
#if defined(TOFU_GRAPHICS_PROCESSOR_ONE_COMMAND_PER_PIXEL)
            if (raster_index >= wait) {
#else   /* TOFU_GRAPHICS_PROCESSOR_ONE_COMMAND_PER_PIXEL */
            while (raster_index >= wait_index) {
#endif  /* TOFU_GRAPHICS_PROCESSOR_ONE_COMMAND_PER_PIXEL */
                switch (entry->command) {
                    case GL_PROGRAM_COMMAND_NOP: {
                        break;
                    }
                    case GL_PROGRAM_COMMAND_WAIT: {
                        size_t x = entry->args[0].size;
                        size_t y = entry->args[1].size;
                        size_t index = y * dwidth + x;
#if defined(TOFU_GRAPHICS_PROCESSOR_DEFENSIVE_CHECKS)
                        // This is not really an issue, as the processor would work anyway,
                        // but it may be a mistyped `WAIT` command.
                        if (index < wait_index) {
                            LOG_W("WAIT command can't jump backwards (at <%d, %d>)", x, y);
                        }
#endif  /* TOFU_GRAPHICS_PROCESSOR_DEFENSIVE_CHECKS */
                        wait_index = index;
                        break;
                    }
                    case GL_PROGRAM_COMMAND_SKIP: {
                        int dx = entry->args[0].integer;
                        int dy = entry->args[1].integer;
                        size_t index = wait_index + (dy * dwidth + dx);
#if defined(TOFU_GRAPHICS_PROCESSOR_DEFENSIVE_CHECKS)
                        // Ditto (see above).
                        if (index < wait_index) {
                            LOG_W("WAIT command can't jump backwards <%d, %d>)", dx, dy);
                        }
#endif  /* TOFU_GRAPHICS_PROCESSOR_DEFENSIVE_CHECKS */
                        wait_index = index;
                        break;
                    }
                    case GL_PROGRAM_COMMAND_MODULO: {
                        src_modulo = entry->args[0].integer;
                        break;
                    }
                    case GL_PROGRAM_COMMAND_OFFSET: {
                        // The offset is in the range of a scanline, so we modulo it to spare operations. Note that
                        // we are casting to `int` to avoid integer promotion, as this is a macro!
                        int offset = IMOD(entry->args[0].integer, (int)dwidth);
                        int delta = offset - (int)dst_offset;
                        dst_offset = (size_t)offset;
                        dst += delta; // Update the current destination pointer accordingly.
                        break;
                    }
                    case GL_PROGRAM_COMMAND_COLOR: {
                        const GL_Pixel_t index = entry->args[0].pixel;
                        const GL_Color_t color = entry->args[1].color;
                        palette[index] = color;
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
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
            GL_Color_t color;
            if (index >= count) {
                const int v = (index - 240) * 8;
                color = (GL_Color_t){ 0, 63 + v, 0, 255 };
            } else {
                color = palette[index];
            }
            *(dst++) = color;
#else
            *(dst++) = palette[index];
#endif
            if (dst == dst_eod) { // Wrap on end-of-data. Check for equality since we are copy one pixel at time.
                dst = dst_sod;
            }

            ++raster_index;
        }

        src += src_modulo;
        dst_sod += dwidth;
    }
}

// FIXME: make a copy or track the reference? (also for xform and palettes)
void GL_processor_set_program(GL_Processor_t *processor, const GL_Program_t *program)
{
#if defined(TOFU_GRAPHICS_PROCESSOR_DEFENSIVE_CHECKS)
        bool is_valid = !program || GL_program_validate(program);
        if (!is_valid) {
            LOG_E("attempt to set invalid processor program %p", program);
            return;
        }
#endif  /* TOFU_GRAPHICS_PROCESSOR_DEFENSIVE_CHECKS */

    if (processor->state.program) { // Deallocate current program, if present.
        GL_program_destroy(processor->state.program);
#if defined(TOFU_CORE_VERBOSE_DEBUG)
        LOG_D("processor program %p destroyed", processor->program);
#endif  /* TOFU_CORE_VERBOSE_DEBUG */
        processor->state.program = NULL;
    }

    if (program) {
        processor->state.program = GL_program_clone(program);
#if defined(TOFU_CORE_VERBOSE_DEBUG)
        LOG_D("processor program at %p copied at %p", program, processor->program);
#endif  /* TOFU_CORE_VERBOSE_DEBUG */
    }
    processor->surface_to_pixels = program ? _surface_to_pixels_program : _surface_to_pixels;
}

void GL_processor_surface_to_pixels(const GL_Processor_t *processor, const GL_Surface_t *surface, GL_Color_t *pixels)
{
    processor->surface_to_pixels(&processor->state, surface, pixels);
}
