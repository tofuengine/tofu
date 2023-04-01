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

#include "processor.h"

#include <libs/imath.h>
#include <libs/log.h>
#include <libs/stb.h>

#define LOG_CONTEXT "gl-processor"

GL_Processor_t *GL_processor_create(void)
{
    GL_Processor_t *processor = malloc(sizeof(GL_Processor_t));
    if (!processor) {
        LOG_E(LOG_CONTEXT, "can't allocate processor");
        return NULL;
    }

    *processor = (GL_Processor_t){ 0 };
#if defined(VERBOSE_DEBUG)
    LOG_D(LOG_CONTEXT, "processor created at %p", processor);
#endif  /* VERBOSE_DEBUG */

#if defined(__PROGRAM_DEFAULT_QUANTIZED_PALETTE__)
    LOG_W(LOG_CONTEXT, "setting default to %d color(s) quantized palette", GL_MAX_PALETTE_COLORS);
  #if GL_MAX_PALETTE_COLORS == 256
    GL_palette_set_quantized(processor->state.palette, 3, 3, 2);
  #elif GL_MAX_PALETTE_COLORS == 128
    GL_palette_set_quantized(processor->state.palette, 2, 3, 2);
  #elif GL_MAX_PALETTE_COLORS == 64
    GL_palette_set_quantized(processor->state.palette, 2, 2, 2);
  #elif GL_MAX_PALETTE_COLORS == 32
    GL_palette_set_quantized(processor->state.palette, 2, 2, 1);
  #elif GL_MAX_PALETTE_COLORS == 16
    GL_palette_set_quantized(processor->state.palette, 1, 2, 1);
  #elif GL_MAX_PALETTE_COLORS == 8
    GL_palette_set_quantized(processor->state.palette, 1, 1, 1);
  #else
    #error "Too few palette entries"
  #endif
#else
    LOG_W(LOG_CONTEXT, "setting default to %d color(s) greyscale palette", GL_MAX_PALETTE_COLORS);
    GL_palette_set_greyscale(processor->state.palette, GL_MAX_PALETTE_COLORS);
#endif

    GL_processor_reset(processor);

    return processor;
}

void GL_processor_destroy(GL_Processor_t *processor)
{
    if (processor->state.program) {
        GL_program_destroy(processor->state.program);
#if defined(VERBOSE_DEBUG)
        LOG_D(LOG_CONTEXT, "processor program %p destroyed", processor->state.program);
#endif  /* VERBOSE_DEBUG */
    }

    free(processor);
#if defined(VERBOSE_DEBUG)
    LOG_D(LOG_CONTEXT, "processor %p freed", processor);
#endif  /* VERBOSE_DEBUG */
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
#if defined(VERBOSE_DEBUG)
    LOG_D(LOG_CONTEXT, "palette copied");
#endif  /* VERBOSE_DEBUG */
}

// TODO: change API, accepting a single array with successive from/to pairs.
void GL_processor_set_shifting(GL_Processor_t *processor, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count)
{
    if (!from) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            processor->state.shifting[i] = (GL_Pixel_t)i;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            processor->state.shifting[from[i]] = to[i];
        }
    }
}

static void _surface_to_rgba(const GL_Processor_State_t *state, const GL_Surface_t *surface, GL_Color_t *pixels)
{
    const GL_Color_t *palette = state->palette;
    const GL_Pixel_t *shifting = state->shifting;

#if defined(__DEBUG_GRAPHICS__)
    const int count = processor->palette->size;
#endif

    const size_t data_size = surface->data_size;

    const GL_Pixel_t *src = surface->data;
    GL_Color_t *dst = pixels;

    for (size_t i = data_size; i; --i) {
        const GL_Pixel_t index = shifting[*(src++)];
#if defined(__DEBUG_GRAPHICS__)
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
void _surface_to_rgba_program(const GL_Processor_State_t *state, const GL_Surface_t *surface, GL_Color_t *pixels)
{
    GL_Color_t palette[GL_MAX_PALETTE_COLORS];
    GL_Pixel_t shifting[GL_MAX_PALETTE_COLORS];
    memcpy(palette, state->palette, sizeof(GL_Color_t) * GL_MAX_PALETTE_COLORS); // Make a local copy, the processor could change it.
    memcpy(shifting, state->shifting, sizeof(GL_Pixel_t) * GL_MAX_PALETTE_COLORS);

    size_t wait = 0;
#if defined(__DEBUG_GRAPHICS__)
    const int count = processor->palette->size;
#endif
    int modulo = 0;
    size_t offset = 0; // Always in the range `[0, width)`.

    const GL_Program_Entry_t *entry = state->program->entries;
    const GL_Pixel_t *src = surface->data;
    GL_Color_t *dst_sod = pixels;

    const size_t dwidth = surface->width;
    const size_t dheight = surface->height;

    size_t i = 0;
    for (size_t h = dheight; h; --h) {
        GL_Color_t *dst_eod = dst_sod + dwidth;
        GL_Color_t *dst = dst_sod + offset; // Apply the (wrapped) offset separately on this row pointer to check row "restart".

        for (size_t w = dwidth; w; --w) {
            // Note: there's no length indicator for the processor program. That means that the interpreter would run
            // endlessly (and unsafely read outside memory bounds, causing crashes). To avoid this a "wait forever"
            // trailer is added to the program in the `GL_program_create()` and `GL_program_reset()` functions.
            // This somehow mimics the real Copper(tm) behaviour, where a special `WAIT` instruction `$FFFF, $FFFE`
            // is used to mark the end of the processor.
#if defined(__PROCESSOR_ONE_COMMAND_PER_PIXEL__)
            if (i >= wait) {
#else
            while (i >= wait) {
#endif
                switch (entry->command) {
                    case GL_PROGRAM_COMMAND_NOP: {
                        break;
                    }
                    case GL_PROGRAM_COMMAND_WAIT: {
                        size_t x = entry->args[0].size;
                        size_t y = entry->args[1].size;
                        wait = y * dwidth + x;
                        break;
                    }
                    case GL_PROGRAM_COMMAND_SKIP: {
                        int dx = entry->args[0].integer;
                        int dy = entry->args[1].integer;
                        wait += dy * dwidth + dx;
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
#if defined(__DEBUG_GRAPHICS__)
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

            ++i;
        }

        src += modulo;
        dst_sod += dwidth;
    }
}

// FIXME: make a copy or track the reference? (also for xform and palettes)
void GL_processor_set_program(GL_Processor_t *processor, const GL_Program_t *program)
{
    if (processor->state.program) { // Deallocate current program, is present.
       GL_program_destroy(processor->state.program);
#if defined(VERBOSE_DEBUG)
        LOG_D(LOG_CONTEXT, "processor program %p destroyed", processor->program);
#endif  /* VERBOSE_DEBUG */
        processor->state.program = NULL;
    }

    if (program) {
        processor->state.program = GL_program_clone(program);
#if defined(VERBOSE_DEBUG)
        LOG_D(LOG_CONTEXT, "processor program at %p copied at %p", program, processor->program);
#endif  /* VERBOSE_DEBUG */
    }
    processor->surface_to_rgba = program ? _surface_to_rgba_program : _surface_to_rgba;
}

void GL_processor_surface_to_rgba(const GL_Processor_t *processor, const GL_Surface_t *surface, GL_Color_t *pixels)
{
    processor->surface_to_rgba(&processor->state, surface, pixels);
}
