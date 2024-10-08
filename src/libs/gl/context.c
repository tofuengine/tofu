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
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "context.h"

#include <core/config.h>
#include <libs/imath.h>
#define _LOG_TAG "gl-context"
#include <libs/log.h>
#include <libs/stb.h>

static void _reset(GL_Context_t *context)
{
    const GL_Surface_t *surface = context->surface;

    GL_State_t state = (GL_State_t){
            .clipping_region = (GL_Quad_t){
                .x0 = 0,
                .y0 = 0,
                .x1 = (int)surface->width,
                .y1 = (int)surface->height
            },
            .shifting = { 0 },
            .transparent = { 0 }
        };

    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        state.shifting[i] = (GL_Pixel_t)i;
        state.transparent[i] = GL_BOOL_FALSE;
    }
    state.transparent[0] = GL_BOOL_TRUE;

    context->state.current = state;
}

GL_Context_t *GL_context_create(const GL_Surface_t *surface)
{
    GL_Context_t *context = malloc(sizeof(GL_Context_t));
    if (!context) {
        LOG_E("can't allocate context");
        return NULL;
    }

    *context = (GL_Context_t){
            .surface = surface
        };

    _reset(context);

    return context;
}

void GL_context_destroy(GL_Context_t *context)
{
    arrfree(context->state.stack);
    LOG_D("context stack at %p freed", context->state.stack);

    free(context);
    LOG_D("context %p freed", context);
}

void GL_context_reset(GL_Context_t *context)
{
    _reset(context);
}

void GL_context_push(GL_Context_t *context)
{
    arrpush(context->state.stack, context->state.current); // Store current state into stack.
}

void GL_context_pop(GL_Context_t *context, size_t levels)
{
    const size_t length = arrlenu(context->state.stack);
    if (length < 1) {
        LOG_W("no states to pop from context");
        return;
    }
    if (levels > length) {
        levels = length;
    }
    for (size_t i = levels; i; --i) {
        context->state.current = arrpop(context->state.stack);
    }
}

void GL_context_set_clipping(GL_Context_t *context, const GL_Rectangle_t *region)
{
    const GL_Surface_t *surface = context->surface;

    GL_State_t *state = &context->state.current;
    if (!region) {
        state->clipping_region = (GL_Quad_t){
                .x0 = 0,
                .y0 = 0,
                .x1 = (int)surface->width,
                .y1 = (int)surface->height
            };
    } else {
        state->clipping_region = (GL_Quad_t){
                .x0 = imax(0, region->x),
                .y0 = imax(0, region->y),
                .x1 = imin((int)surface->width, region->x + (int)region->width),
                .y1 = imin((int)surface->height, region->y + (int)region->height)
            };
    }
}

void GL_context_set_shifting(GL_Context_t *context, const GL_Pixel_t *from, const GL_Pixel_t *to, size_t count)
{
    GL_State_t *state = &context->state.current;
    if (!from) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            state->shifting[i] = (GL_Pixel_t)i;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            state->shifting[from[i]] = to[i];
        }
    }
}

void GL_context_set_transparent(GL_Context_t *context, const GL_Pixel_t *indexes, const GL_Bool_t *transparent, size_t count)
{
    GL_State_t *state = &context->state.current;
    if (!indexes) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            state->transparent[i] = GL_BOOL_FALSE;
        }
        state->transparent[0] = GL_BOOL_TRUE;
    } else {
        for (size_t i = 0; i < count; ++i) {
            state->transparent[indexes[i]] = transparent[i];
        }
    }
}

void GL_context_clear(const GL_Context_t *context, GL_Pixel_t index, bool transparency)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    const int width = clipping_region->x1 - clipping_region->x0;
    const int height = clipping_region->y1 - clipping_region->y0;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }
    // FIXME: remove this early bailing out everywhere? Null for-loop suffices and is better due to lack of branch?

    index = shifting[index];

    if (transparency && transparent[index]) {
        return;
    }

    GL_Pixel_t *ddata = surface->data;

    const size_t dwidth = surface->width;

    const size_t dskip = dwidth - width;

    GL_Pixel_t *dptr = ddata + clipping_region->y0 * dwidth + clipping_region->x0;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            *(dptr++) = index;
        }
        dptr += dskip;
    }
}
