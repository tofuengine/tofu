/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "context.h"

#include "../config.h"
#include "surface.h"

#include "../core/imath.h"
#include "../log.h"

#include <stdlib.h>
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif
#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define PIXEL(c, x, y)  ((c)->surface.data_rows[(y)] + (x))

#ifdef __DEBUG_GRAPHICS__
static inline void pixel(const GL_Context_t *context, int x, int y, int index)
{
    *PIXEL(context, x, y) = 240 + (index % 16);
}
#endif

static inline GL_State_t initialize_state(size_t width, size_t height)
{
    GL_State_t state = (GL_State_t){
            .clipping_region = (GL_Quad_t){ .x0 = 0, .y0 = 0, .x1 = width - 1, .y1 = height - 1 },
            .background = 0
        };
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        state.shifting[i] = i;
        state.transparent[i] = GL_BOOL_FALSE;
    }
    state.transparent[0] = GL_BOOL_TRUE;
    return state;
}

bool GL_context_create(GL_Context_t *context, size_t width, size_t height)
{
    GL_Surface_t surface;
    GL_surface_create(&surface, width, height);

    *context = (GL_Context_t){
            .surface = surface,
            .state = initialize_state(width, height),
            .stack = NULL
        };

    return true;
}

void GL_context_delete(GL_Context_t *context)
{
    arrfree(context->stack);

    GL_surface_delete(&context->surface);
    Log_write(LOG_LEVELS_DEBUG, "<GL> context deallocated");

    *context = (GL_Context_t){};
}

void GL_context_push(GL_Context_t *context)
{
    arrpush(context->stack, context->state);
    context->state = initialize_state(context->surface.width, context->surface.height);
}

void GL_context_pop(GL_Context_t *context)
{
    if (arrlen(context->stack) == 0) {
        Log_write(LOG_LEVELS_WARNING, "<GL> no states to pop from stack");
        return;
    }
    context->state = arrpop(context->stack);
}

void GL_context_clear(const GL_Context_t *context)
{
    const GL_State_t *state = &context->state;
    const GL_Pixel_t color = state->background;
    GL_Pixel_t *dst = context->surface.data;
    for (size_t i = context->surface.data_size; i; --i) {
        *(dst++) = color;
    }
}

void GL_context_shifting(GL_Context_t *context, const size_t *from, const size_t *to, size_t count)
{
    GL_State_t *state = &context->state;
    if (!from) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            state->shifting[i] = i;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            state->shifting[from[i]] = to[i];
        }
    }
}

void GL_context_transparent(GL_Context_t *context, const GL_Pixel_t *indexes, const GL_Bool_t *transparent, size_t count)
{
    GL_State_t *state = &context->state;
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

void GL_context_clipping(GL_Context_t *context, const GL_Quad_t *clipping_region)
{
    GL_State_t *state = &context->state;
    if (!clipping_region) {
        state->clipping_region = (GL_Quad_t){
                .x0 = 0,
                .y0 = 0,
                .x1 = context->surface.width - 1,
                .y1 = context->surface.height - 1
            };
    } else {
        // TODO: check is clipping region is legit and restrict if necessary.
        state->clipping_region = *clipping_region;
    }
}

void GL_context_background(GL_Context_t *context, const GL_Pixel_t index)
{
    GL_State_t *state = &context->state;
    state->background = index;
}

void GL_context_color(GL_Context_t *context, GL_Pixel_t index)
{
    GL_State_t *state = &context->state;
    state->color = index;
}

void GL_context_pattern(GL_Context_t *context, uint32_t mask)
{
    GL_State_t *state = &context->state;
    state->mask = mask;
}

void GL_context_screenshot(const GL_Context_t *context, const GL_Palette_t *palette, const char *pathfile)
{
    void *vram = malloc(context->surface.width * context->surface.height * sizeof(GL_Color_t));
    if (!vram) {
        Log_write(LOG_LEVELS_WARNING, "<GL> can't create buffer for screenshot");
    }

    GL_context_to_rgba(context, palette, vram);

    int result = stbi_write_png(pathfile, context->surface.width, context->surface.height, sizeof(GL_Color_t), vram, context->surface.width * sizeof(GL_Color_t));
    if (!result) {
        Log_write(LOG_LEVELS_WARNING, "<GL> can't save screenshot to '%s'", pathfile);
    }

    free(vram);
}

void GL_context_to_rgba(const GL_Context_t *context, const GL_Palette_t *palette, void *vram)
{
    const int width = context->surface.width;
    const int height = context->surface.height;
    const GL_Color_t *colors = palette->colors;
#ifdef __DEBUG_GRAPHICS__
    int count = palette->count;
#endif
    const GL_Pixel_t *src = context->surface.data;
    GL_Color_t *dst = (GL_Color_t *)vram;
    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            GL_Pixel_t index = *src++;
#ifdef __DEBUG_GRAPHICS__
            GL_Color_t color;
            if (index >= count) {
                int y = (index - 240) * 8;
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
}

void GL_context_to_surface(const GL_Context_t *context, const GL_Surface_t *surface)
{
    size_t width = context->surface.width;
    size_t height = context->surface.height;

    if (width > surface->width) {
        width = surface->width;
    }
    if (height > surface->height) {
        height = surface->height;
    }

    const GL_Pixel_t *src = surface->data;
    GL_Pixel_t *dst = context->surface.data;

    const int src_skip = surface->width - width;
    const int dst_skip = context->surface.width - width;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
            *(dst++) = *(src++);
        }
        src += src_skip;
        dst += dst_skip;
    }
}

// TODO: specifies `const` always? Is pedantic or useful?
// TODO: define a `BlitInfo` and `BlitFunc` types to generalize?
// https://dev.to/fenbf/please-declare-your-variables-as-const
void GL_context_blit(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + area.width - 1,
            .y1 = position.y + area.height - 1
        };

    int skip_x = 0; // Offset into the (source) surface/texture, update during clipping.
    int skip_y = 0;

    if (drawing_region.x0 < clipping_region.x0) {
        skip_x = clipping_region.x0 - drawing_region.x0;
        drawing_region.x0 = clipping_region.x0;
    }
    if (drawing_region.y0 < clipping_region.y0) {
        skip_y = clipping_region.y0 - drawing_region.y0;
        drawing_region.y0 = clipping_region.y0;
    }
    if (drawing_region.x1 > clipping_region.x1) {
        drawing_region.x1 = clipping_region.x1;
    }
    if (drawing_region.y1 > clipping_region.y1) {
        drawing_region.y1 = clipping_region.y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    const GL_Pixel_t *src = surface->data_rows[area.y + skip_y] + (area.x + skip_x);
    GL_Pixel_t *dst = context->surface.data_rows[drawing_region.y0] + drawing_region.x0;

    const int src_skip = surface->width - width;
    const int dst_skip = context->surface.width - width;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
            pixel(context, drawing_region.x0 + width - j, drawing_region.y0 + height - i, (int)i + (int)j);
#endif
            GL_Pixel_t index = shifting[*(src++)];
#if 1
            if (transparent[index]) {
                dst++;
            } else {
                *(dst++) = index;
            }
#else
            *dst = transparent[index] ? *dst : colors[index];
            dst++;
#endif
        }
        src += src_skip;
        dst += dst_skip;
    }
}

// Simple implementation of nearest-neighbour scaling, with x/y flipping according to scaling-factor sign.
// See `http://tech-algorithm.com/articles/nearest-neighbor-image-scaling/` for a reference code.
// To avoid empty pixels we scan the destination area and calculate the source pixel.
void GL_context_blit_s(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position, float scale_x, float scale_y)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    const int drawing_width = (int)((float)(area.width * fabs(scale_x)) + 0.5f);
    const int drawing_height = (int)((float)(area.height * fabs(scale_y)) + 0.5f);

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + drawing_width - 1,
            .y1 = position.y + drawing_height - 1,
        };

    float skip_x = 0.0f; // Offset into the (source) surface/texture, update during clipping.
    float skip_y = 0.0f;

    if (drawing_region.x0 < clipping_region.x0) {
        skip_x = (float)(clipping_region.x0 - drawing_region.x0) / scale_x;
        drawing_region.x0 = clipping_region.x0;
    }
    if (drawing_region.y0 < clipping_region.y0) {
        skip_y = (float)(clipping_region.y0 - drawing_region.y0) / scale_y;
        drawing_region.y0 = clipping_region.y0;
    }
    if (drawing_region.x1 > clipping_region.x1) {
        drawing_region.x1 = clipping_region.x1;
    }
    if (drawing_region.y1 > clipping_region.y1) {
        drawing_region.y1 = clipping_region.y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    GL_Pixel_t *dst = context->surface.data_rows[drawing_region.y0] + drawing_region.x0;

    const size_t skip = context->surface.width - width;

    const float du = 1.0f / scale_x; // Texture coordinates deltas (signed).
    const float dv = 1.0f / scale_y;

    float ou = (float)area.x + skip_x;
    if (scale_x < 0.0f) {
        ou += (float)area.width + du; // Move to last pixel, scaled, into the texture.
    }
    float ov = (float)area.y + skip_y;
    if (scale_y < 0.0f) {
        ov += (float)area.height + dv;
    }

    float v = ov; // NOTE: we can also apply an integer-based DDA method, using reminders.
    for (int i = height; i; --i) {
        const GL_Pixel_t *src = surface->data_rows[(int)v];

        float u = ou;
        for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
            pixel(context, drawing_region.x0 + width - j, drawing_region.y0 + height - i, (int)u + (int)v);
#endif
            GL_Pixel_t index = shifting[src[(int)u]];
#if 1
            if (transparent[index]) {
                dst++;
            } else {
                *(dst++) = index;
            }
#else
            *dst = transparent[index] ? *dst : colors[index];
            dst++;
#endif
            u += du;
        }

        v += dv;
        dst += skip;
    }
}

// https://web.archive.org/web/20190305223938/http://www.drdobbs.com/architecture-and-design/fast-bitmap-rotation-and-scaling/184416337
// https://www.flipcode.com/archives/The_Art_of_Demomaking-Issue_10_Roto-Zooming.shtml
// TODO: add 90/180/270 rotations?
void GL_context_blit_sr(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position, float scale_x, float scale_y, float angle, float anchor_x, float anchor_y)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    const float w = (float)area.width;
    const float h = (float)area.height;
    const float sw = (float)w * scale_x;
    const float sh = (float)h * scale_y;

    const float sax = w * anchor_x; // Anchor points, relative to the source and destination areas.
    const float say = h * anchor_y;
    const float dax = sw * anchor_x;
    const float day = sh * anchor_y;

    const float sx = area.x;
    const float sy = area.y;
    const float dx = position.x;
    const float dy = position.y;

    const float c = cosf(angle);
    const float s = sinf(angle);

    // The counter-clockwise 2D rotation matrix is
    //
    //      |  c  -s |
    //  R = |        |
    //      |  s   c |
    //
    // In order to calculate the clockwise rotation matrix one can use the
    // similarities `cos(-a) = cos(a)` and `sin(-a) = -sin(a)` and get
    //
    //      |  c   s |
    //  R = |        |
    //      | -s   c |

    // Rotate the four corners of the scaled image to compute the rotate/scaled AABB.
    //
    // Note that we aren *not* adding `dst/dty` on purpose to rotate around the anchor point.
    const float aabb_x0 = -dax;
    const float aabb_y0 = -day;
    const float aabb_x1 = sw - dax;
    const float aabb_y1 = sh - day;

    const float x0 = c * aabb_x0 - s * aabb_y0;
    const float y0 = s * aabb_x0 + c * aabb_y0;

    const float x1 = c * aabb_x1 - s * aabb_y0;
    const float y1 = s * aabb_x1 + c * aabb_y0;

    const float x2 = c * aabb_x1 - s * aabb_y1;
    const float y2 = s * aabb_x1 + c * aabb_y1;

    const float x3 = c * aabb_x0 - s * aabb_y1;
    const float y3 = s * aabb_x0 + c * aabb_y1;

    // Clip both destination and target rectangles.
    // Note, `floorf()` is not needed here, since we have always a positive-valued clipping region.
    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = (int)(fmin(fmin(fmin(x0, x1), x2), x3) + dx),
            .y0 = (int)(fmin(fmin(fmin(y0, y1), y2), y3) + dy),
            .x1 = (int)(fmax(fmax(fmax(x0, x1), x2), x3) + dx),
            .y1 = (int)(fmax(fmax(fmax(y0, y1), y2), y3) + dy)
        };

    if (drawing_region.x0 < clipping_region.x0) {
        drawing_region.x0 = clipping_region.x0;
    }
    if (drawing_region.y0 < clipping_region.y0) {
        drawing_region.y0 = clipping_region.y0;
    }
    if (drawing_region.x1 > clipping_region.x1) {
        drawing_region.x1 = clipping_region.x1;
    }
    if (drawing_region.y1 > clipping_region.y1) {
        drawing_region.y1 = clipping_region.y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    const int sminx = area.x;
    const int sminy = area.y;
    const int smaxx = area.x + area.width - 1;
    const int smaxy = area.y + area.height - 1;

    const float M11 = c / scale_x;  // Since we are doing an *inverse* transformation, we combine rotation and *then* scaling (TRS -> SRT).
    const float M12 = s / scale_x;  // | 1/sx    0 | |  c s |
    const float M21 = -s / scale_y; // |           | |      |
    const float M22 = c / scale_y;  // |    0 1/sy | | -s c |

    const float tlx = (float)drawing_region.x0 - dx; // Transform the top-left corner of the to-be-drawn rectangle to texture space.
    const float tly = (float)drawing_region.y0 - dy; // (could differ from AABB x0 due to clipping, we need to compute it again)
    float ou = (tlx * M11 + tly * M12) + sax + sx; // Offset to the source texture quad.
    float ov = (tlx * M21 + tly * M22) + say + sy;

    GL_Pixel_t *dst = context->surface.data_rows[drawing_region.y0] + drawing_region.x0;

    const int skip = context->surface.width - width;

    for (int i = height; i; --i) {
        float u = ou;
        float v = ov;

        for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
            pixel(context, drawing_region.x0 + width - j, drawing_region.y0 + height - i, 15);
#endif
            int x = (int)floorf(u); // Round down, to preserve negative values as such (e.g. `-0.3` is `-1`) and avoid mirror effect.
            int y = (int)floorf(v);

            if (x >= sminx && x <= smaxx && y >= sminy && y <= smaxy) {
#ifdef __DEBUG_GRAPHICS__
                pixel(context, drawing_region.x0 + width - j, drawing_region.y0 + height - i, (int)i + (int)j);
#endif
                const GL_Pixel_t *src = surface->data_rows[y] + x;
                GL_Pixel_t index = shifting[*src];
                if (!transparent[index]) {
                    *dst = index;
                }
            }

            ++dst;

            u += M11;
            v += M21;
        }

        dst += skip;

        ou += M12;
        ov += M22;
    }
#ifdef __DEBUG_GRAPHICS__
    pixel(context, drawing_region.x0, drawing_region.y0, 7);
    pixel(context, drawing_region.x1, drawing_region.y0, 7);
    pixel(context, drawing_region.x1, drawing_region.y1, 7);
    pixel(context, drawing_region.x0, drawing_region.y1, 7);
#endif
}

// https://www.youtube.com/watch?v=3FVN_Ze7bzw
// http://www.coranac.com/tonc/text/mode7.htm
// https://wiki.superfamicom.org/registers
// https://www.smwcentral.net/?p=viewthread&t=27054
void GL_context_blit_x(const GL_Context_t *context, const GL_Surface_t *surface, GL_Point_t position, GL_XForm_t xform)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    const int clamp = xform.clamp;
    const GL_XForm_Table_Entry_t *table = xform.table;

    GL_Quad_t drawing_region = (GL_Quad_t) {
        .x0 = position.x,
        .y0 = position.y,
        .x1 = position.x + (clipping_region.x1 - clipping_region.x0),
        .y1 = position.y + (clipping_region.y1 - clipping_region.y0)
    };

    if (drawing_region.x0 < clipping_region.x0) {
        drawing_region.x0 = clipping_region.x0;
    }
    if (drawing_region.y0 < clipping_region.y0) {
        drawing_region.y0 = clipping_region.y0;
    }
    if (drawing_region.x1 > clipping_region.x1) {
        drawing_region.x1 = clipping_region.x1;
    }
    if (drawing_region.y1 > clipping_region.y1) {
        drawing_region.y1 = clipping_region.y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    const int sw = surface->width;
    const int sh = surface->height;
    const int sminx = 0;
    const int sminy = 0;
    const int smaxx = sw - 1;
    const int smaxy = sh - 1;

    GL_Pixel_t *dst = context->surface.data_rows[drawing_region.y0] + drawing_region.x0;

    const int skip = context->surface.width - width;

    // The basic Mode7 formula is the following
    //
    // [ X ]   [ A B ]   [ SX + H - CX ]   [ CX ]
    // [   ] = [     ] * [             ] + [    ]
    // [ Y ]   [ C D ]   [ SY + V - CY ]   [ CY ]
    //
    // However, it can be optimized by (re)computing the transformed X/Y pair at each scanline,
    // then moving along the projected matrix line using the 1st matrix column-vector.
    //
    // X[0,y] = A*(H-CX) + B*y + B*(V-CY) + CX
    //        = A*(H-CX) + B*(y+V-CY) + CX
    // Y[0,y] = C*(H-CX) + D*y + D*(V-CY) + CY
    //        = C*(H-CX) + D*(y+V-CY) + CY
    //
    // X[x,y] = X[x-1,y] + A
    // Y[x,y] = Y[x-1,y] + C
    //
    // The current scan-line need to be (re)projected due to the presence of the HDMA modifier.
    //
    // The formula above seems to be incorrect. The H/V displacement should be applied only at last,
    // to get the final position on the texture, that is
    //
    // X = A * (SX - CX) + B * (SY - CY) + CX + H
    // Y = C * (SX - CX) + D * (SY - CY) + CY + V
    GL_XForm_State_t xform_state;
    memcpy(&xform_state, &xform.state, sizeof(GL_XForm_State_t));
    float h = xform_state.h; float v = xform_state.v; float a = xform_state.a; float b = xform_state.b;
    float c = xform_state.c; float d = xform_state.d; float x0 = xform_state.x; float y0 = xform_state.y;

    for (int i = 0; i < height; ++i) {
        if (table && i == table->scan_line) {
            for (int k = 0; k < table->count; ++k) {
                xform_state.registers[table->operations[k].id] = table->operations[k].value;
            }
            h = xform_state.h; v = xform_state.v; a = xform_state.a; b = xform_state.b; // Keep the fast-access variables updated.
            c = xform_state.c; d = xform_state.d; x0 = xform_state.x; y0 = xform_state.y;
            ++table;
#ifdef __DETACH_XFORM_TABLE__
            if (table->y == -1) { // End-of-data reached, detach pointer for faster loop.
                table = NULL;
            }
#endif
        }

        const float xi = 0.0f - x0;
        const float yi = (float)i - y0;

#ifndef __CLIP_OFFSET__
        float xp = (a * xi + b * yi) + x0 + h;
        float yp = (c * xi + d * yi) + y0 + v;
#else
        float xp = (a * xi + b * yi) + x0 + fmodf(h, sw); // Clip to avoid cancellation when H/V are large.
        float yp = (c * xi + d * yi) + y0 + fmodf(v, sh);
#endif

        for (int j = 0; j < width; ++j) {
#ifdef __DEBUG_GRAPHICS__
            pixel(context, drawing_region.x0 + j, drawing_region.y0 + i, i + j);
#endif
            int sx = (int)(xp + 0.5f); // Round to avoid artifacts.
            int sy = (int)(yp + 0.5f);

            if (clamp == GL_XFORM_CLAMP_REPEAT) {
                sx = imod(sx, sw);
                sy = imod(sy, sh);
            } else
            if (clamp == GL_XFORM_CLAMP_EDGE) {
                if (sx < sminx) {
                    sx = sminx;
                } else
                if (sx > smaxx) {
                    sx = smaxx;
                }
                if (sy < sminy) {
                    sy = sminy;
                } else
                if (sy > smaxy) {
                    sy = smaxy;
                }
            }

            if (sx >= sminx && sx <= smaxx && sy >= sminy && sy <= smaxy) {
                const GL_Pixel_t *src = surface->data_rows[sy] + sx;
                GL_Pixel_t index = shifting[*src];
                if (!transparent[index]) {
                    *dst = index;
                }
            }

            ++dst;

            xp += a;
            yp += c;
        }

        dst += skip;
    }
}

// https://lodev.org/cgtutor/floodfill.html
void GL_context_fill(const GL_Context_t *context, GL_Point_t seed, GL_Pixel_t index)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t clipping_region = state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    if (seed.x < clipping_region.x0 || seed.x > clipping_region.x1
        || seed.y < clipping_region.y0 || seed.y > clipping_region.y1) {
        return;
    }

    const GL_Pixel_t match = *context->surface.data_rows[seed.y] + seed.x;
    const GL_Pixel_t replacement = shifting[index];

    GL_Point_t *stack = NULL;
    arrpush(stack, seed);

    const int skip = context->surface.width;

    while (arrlen(stack) > 0) {
        const GL_Point_t position = arrpop(stack);

        int x = position.x;
        int y = position.y;

        GL_Pixel_t *dst = context->surface.data_rows[y] + x;
        while (x >= clipping_region.x0 && *dst == match) {
            --x;
            --dst;
        }
        ++x;
        ++dst;

        bool above = false;
        bool below = false;

        while (x <= clipping_region.x1 && *dst == match) {
            *dst = replacement;

            const GL_Pixel_t pixel_above = *(dst - skip);
            if (!above && y >= clipping_region.y0 && pixel_above == match) {
                const GL_Point_t p = (GL_Point_t){ .x = x, .y = y - 1 };
                arrpush(stack, p);
                above = true;
            } else
            if (above && y >= clipping_region.y0 && pixel_above != match) {
                above = false;
            }

            const GL_Pixel_t pixel_below = *(dst + skip);
            if (!below && y < clipping_region.y1 && pixel_below == match) {
                const GL_Point_t p = (GL_Point_t){ .x = x, .y = y + 1 };
                arrpush(stack, p);
                below = true;
            } else
            if (below && y < clipping_region.y1 && pixel_below != match) {
                above = false;
            }

            ++x;
            ++dst;
        }
    }

    arrfree(stack);
}
