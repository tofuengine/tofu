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

#include "../log.h"

#include <stdlib.h>
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif
#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define PIXEL(c, x, y)  ((GL_Color_t *)(c)->vram_rows[(y)] + (x))

#ifdef __DEBUG_GRAPHICS__
static void pixel(const GL_Context_t *context, int x, int y, int index)
{
    int v = (index % 16) * 8;
    *PIXEL(context, x, y) = (GL_Color_t){ 0, 63 + v, 0, 255 };
}
#endif

bool GL_context_initialize(GL_Context_t *context, size_t width, size_t height)
{
    void *vram = malloc(width * height * sizeof(GL_Color_t));
    if (!vram) {
        Log_write(LOG_LEVELS_ERROR, "<GL> can't allocate VRAM buffer");
        return false;
    }

    void **vram_rows = malloc(height * sizeof(void *));
    if (!vram_rows) {
        Log_write(LOG_LEVELS_ERROR, "<GL> can't allocate VRAM LUT");
        free(vram);
        return false;
    }
    for (size_t i = 0; i < height; ++i) {
        vram_rows[i] = (GL_Color_t *)vram + (width * i);
    }

    Log_write(LOG_LEVELS_DEBUG, "<GL> VRAM allocated at #%p (%dx%d)", vram, width, height);

    *context = (GL_Context_t){
            .width = width,
            .height = height,
            .stride = width * sizeof(GL_Color_t),
            .vram = vram,
            .vram_rows = vram_rows,
            .vram_size = width * height,
            .clipping_region = (GL_Quad_t){ .x0 = 0, .y0 = 0, .x1 = width - 1, .y1 = height - 1 }
        };

    context->background = 0;
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        context->shifting[i] = i;
        context->transparent[i] = GL_BOOL_FALSE;
    }
    context->transparent[0] = GL_BOOL_TRUE;

    GL_palette_greyscale(&context->palette, GL_MAX_PALETTE_COLORS);
    Log_write(LOG_LEVELS_DEBUG, "<GL> calculating greyscale palette of #%d entries", GL_MAX_PALETTE_COLORS);

    return true;
}

void GL_context_terminate(GL_Context_t *context)
{
    free(context->vram);
    free(context->vram_rows);
    Log_write(LOG_LEVELS_DEBUG, "<GL> context deallocated");

    *context = (GL_Context_t){};
}

void GL_context_push(const GL_Context_t *context)
{
}

void GL_context_pop(const GL_Context_t *context)
{
}

void GL_context_clear(const GL_Context_t *context)
{
    const GL_Color_t color = context->palette.colors[context->background];
    GL_Color_t *dst = (GL_Color_t *)context->vram;
    for (size_t i = context->vram_size; i > 0; --i) {
        *(dst++) = color;
    }
}

void GL_context_screenshot(const GL_Context_t *context, const char *pathfile)
{
    int result = stbi_write_png(pathfile, context->width, context->height, sizeof(GL_Color_t), context->vram, context->stride);
    if (!result) {
        Log_write(LOG_LEVELS_WARNING, "<GL> can't save screenshot to '%s'", pathfile);
    }
}

// TODO: specifies `const` always? Is pedantic or useful?
// TODO: define a `BlitInfo` and `BlitFunc` types to generalize?
// https://dev.to/fenbf/please-declare-your-variables-as-const
void GL_context_blit(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position)
{
    const GL_Quad_t clipping_region = context->clipping_region;
    const GL_Pixel_t *shifting = context->shifting;
    const GL_Bool_t *transparent = context->transparent;
    const GL_Color_t *colors = context->palette.colors;

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

    const GL_Pixel_t *src = (const GL_Pixel_t *)surface->data_rows[area.y + skip_y] + (area.x + skip_x);
    GL_Color_t *dst = (GL_Color_t *)context->vram_rows[drawing_region.y0] + drawing_region.x0;

    const int src_skip = surface->width - width;
    const int dst_skip = context->width - width;

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
                *(dst++) = colors[index];
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
    const GL_Quad_t clipping_region = context->clipping_region;
    const GL_Pixel_t *shifting = context->shifting;
    const GL_Bool_t *transparent = context->transparent;
    const GL_Color_t *colors = context->palette.colors;

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

    GL_Color_t *dst = (GL_Color_t *)context->vram_rows[drawing_region.y0] + drawing_region.x0;

    const size_t skip = context->width - width;

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
        const GL_Pixel_t *src = (const GL_Pixel_t *)surface->data_rows[(int)v];

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
                *(dst++) = colors[index];
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
void GL_context_blit_sr(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position, float scale_x, float scale_y, float angle, float anchor_x, float anchor_y)
{
    const GL_Quad_t clipping_region = context->clipping_region;
    const GL_Pixel_t *shifting = context->shifting;
    const GL_Bool_t *transparent = context->transparent;
    const GL_Color_t *colors = context->palette.colors;

    // FIXME: there's a rounding error here, that generates a spurious row/column.

    const float w = (float)area.width;
    const float h = (float)area.height;
    const float sw = (float)w * scale_x;
    const float sh = (float)h * scale_y;

    const float stx = w * anchor_x; // Anchor points, relative to the source and destination areas.
    const float sty = h * anchor_y;
    const float dtx = sw * anchor_x;
    const float dty = sh * anchor_y;

    const float ox = area.x;
    const float oy = area.y;
    const float px = position.x;
    const float py = position.y;

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
    const float ax = -dtx;
    const float ay = -dty;
    const float bx = sw - dtx;
    const float by = sh - dty;

    const float x0 = c * ax - s * ay;
    const float y0 = s * ax + c * ay;

    const float x1 = c * bx - s * ay;
    const float y1 = s * bx + c * ay;

    const float x2 = c * bx - s * by;
    const float y2 = s * bx + c * by;

    const float x3 = c * ax - s * by;
    const float y3 = s * ax + c * by;

    const float aabb_x0 = fmin(fmin(fmin(x0, x1), x2), x3);
    const float aabb_y0 = fmin(fmin(fmin(y0, y1), y2), y3);
    const float aabb_x1 = fmax(fmax(fmax(x0, x1), x2), x3);
    const float aabb_y1 = fmax(fmax(fmax(y0, y1), y2), y3);
/*
    float fh = sw * s + sh * c;
    float fw = sh * s + sw * c;
    fh = fh;
    fw = fw;
*/
    // Clip both destination and target rectangles. Round for better result.
    const int dminx = (int)fmax(aabb_x0 + 0.5f + px, (float)clipping_region.x0);
    const int dminy = (int)fmax(aabb_y0 + 0.5f + py, (float)clipping_region.y0);
    const int dmaxx = (int)fmin(aabb_x1 + 0.5f + px, (float)clipping_region.x1);
    const int dmaxy = (int)fmin(aabb_y1 + 0.5f + py, (float)clipping_region.y1);

    const int sminx = area.x;
    const int sminy = area.y;
    const int smaxx = area.x + area.width - 1;
    const int smaxy = area.y + area.height - 1;

#define __ROTATE_AND_SCALE__
#ifdef __ROTATE_AND_SCALE__
    const float M11 = c / scale_x; // Combine (inverse) rotation and *then* scaling matrices.
    const float M12 = s / scale_x;  // | 1/sx    0 | |  c s |
    const float M21 = -s / scale_y; // |           | |      |
    const float M22 = c / scale_y;  // |    0 1/sy | | -s c |
#else
    const float M11 = c / scale_x; // Combine scaling down and (inverse) rotate into a single matrix.
    const float M12 = s / scale_y;  // |  c s |   | 1/sx   0  |
    const float M21 = -s / scale_x; // |      | x |           |
    const float M22 = c / scale_y;  // | -s c |   |   0  1/sy |
#endif
    const float tlx = (dminx - px); // Transform the top-left corner of the to-be-drawn rectangle to texture space.
    const float tly = (dminy - py);
    float ou = (tlx * M11 + tly * M12) + stx + ox; // Offset to the source texture quad.
    float ov = (tlx * M21 + tly * M22) + sty + oy;

    for (int y = dminy; y <= dmaxy; ++y) {
        float u = ou;
        float v = ov;

        for (int x = dminx; x <= dmaxx; ++x) {
#ifdef __DEBUG_GRAPHICS__
            pixel(context, x, y, 15);
#endif
//#define __NAIVE_ROTATION__
#ifdef __NAIVE_ROTATION__
            float tx = (float)x - px; // Move relative to center
            float ty = (float)y - py;
            float rx = tx * c + ty * s; // Rotate the point.
            float ry = ty * c - tx * s;
            float sx = rx / scale_x; // Scale into the texture.
            float sy = ry / scale_y;
            int xx = (int)(sx + stx + ox);
            int xy = (int)(sy + sty + oy);
#else
            int xx = (int)u;
            int xy = (int)v;
#endif

            if (xx >= sminx && xy >= sminy && xx <= smaxx && xy <= smaxy) {
#ifdef __DEBUG_GRAPHICS__
                pixel(context, x, y, xx + xy);
#endif
                const GL_Pixel_t *src = (const GL_Pixel_t *)surface->data_rows[xy] + xx;
                GL_Pixel_t index = shifting[*src];

                if (!transparent[index]) {
                    GL_Color_t *dst = (GL_Color_t *)context->vram_rows[y] + x;
                    *dst = colors[index];
                }
            }

            u += M11;
            v += M21;
        }

        ou += M12;
        ov += M22;
    }
#ifdef __DEBUG_GRAPHICS__
    pixel(context, dminx, dminy, 7);
    pixel(context, dmaxx, dminy, 7);
    pixel(context, dmaxx, dmaxy, 7);
    pixel(context, dminx, dmaxy, 7);
#endif
}

void GL_context_palette(GL_Context_t *context, const GL_Palette_t *palette)
{
    context->palette = *palette;
    Log_write(LOG_LEVELS_DEBUG, "<GL> palette updated");
}

void GL_context_shifting(GL_Context_t *context, const size_t *from, const size_t *to, size_t count)
{
    if (!from) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            context->shifting[i] = i;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            context->shifting[from[i]] = to[i];
        }
    }
}

void GL_context_transparent(GL_Context_t *context, const GL_Pixel_t *indexes, const GL_Bool_t *transparent, size_t count)
{
    if (!indexes) {
        for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
            context->transparent[i] = GL_BOOL_FALSE;
        }
        context->transparent[0] = GL_BOOL_TRUE;
    } else {
        for (size_t i = 0; i < count; ++i) {
            context->transparent[indexes[i]] = transparent[i];
        }
    }
}

void GL_context_clipping(GL_Context_t *context, const GL_Quad_t *clipping_region)
{
    if (!clipping_region) {
        context->clipping_region = (GL_Quad_t){
                .x0 = 0,
                .y0 = 0,
                .x1 = context->width - 1,
                .y1 = context->height - 1
            };
    } else {
        context->clipping_region = *clipping_region;
    }
}

void GL_context_background(GL_Context_t *context, const GL_Pixel_t index)
{
    if (index >= context->palette.count) {
        Log_write(LOG_LEVELS_WARNING, "<GL> color index #%d not available in current palette", index);
        return;
    }
    context->background = index;
}

void GL_context_color(GL_Context_t *context, GL_Pixel_t index)
{
    if (index >= context->palette.count) {
        Log_write(LOG_LEVELS_WARNING, "<GL> color index #%d not available in current palette", index);
        return;
    }
    context->color = index;
}

void GL_context_pattern(GL_Context_t *context, uint32_t mask)
{
    context->mask = mask;
}

void GL_context_fill(const GL_Context_t *context, GL_Point_t seed, GL_Pixel_t index)
{
    const GL_Quad_t clipping_region = context->clipping_region;

    if (seed.x < clipping_region.x0 || seed.x > clipping_region.x1
        || seed.y < clipping_region.y0 || seed.y > clipping_region.y1) {
        return;
    }

    GL_Color_t match = *PIXEL(context, seed.x, seed.y);
    GL_Color_t replacement = context->palette.colors[index];

    GL_Point_t *stack = NULL;
    arrpush(stack, seed);

    while (arrlen(stack) > 0) {
        GL_Point_t position = arrpop(stack);

        int x = position.x;
        int y = position.y;

        while (x >= clipping_region.x0 && !memcmp(PIXEL(context, x, y), &match, sizeof(GL_Color_t))) {
            --x;
        }
        ++x;

        bool above = false;
        bool below = false;

        while (x <= clipping_region.x1 && !memcmp(PIXEL(context, x, y), &match, sizeof(GL_Color_t))) {
            *PIXEL(context, x, y) = replacement;

            if (!above && y >= clipping_region.y0 && !memcmp(PIXEL(context, x, y - 1), &match, sizeof(GL_Color_t))) {
                GL_Point_t p = (GL_Point_t){ .x = x, .y = y - 1 };
                arrpush(stack, p);
                above = true;
            } else
            if (above && y >= clipping_region.y0 && memcmp(PIXEL(context, x, y - 1), &match, sizeof(GL_Color_t))) {
                above = false;
            }

            if (!below && y < clipping_region.y1 && !memcmp(PIXEL(context, x, y + 1), &match, sizeof(GL_Color_t))) {
                GL_Point_t p = (GL_Point_t){ .x = x, .y = y + 1 };
                arrpush(stack, p);
                below = true;
            } else
            if (below && y < clipping_region.y1 && memcmp(PIXEL(context, x, y + 1), &match, sizeof(GL_Color_t))) {
                above = false;
            }

            ++x;
        }
    }

    arrfree(stack);
}
