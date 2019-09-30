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

#define PIXEL(c, x, y)  ((GL_Pixel_t *)(c)->vram_rows[(y)] + (x))

static int iabs(int v)
{
    return v > 0 ? v : -v;
}

#ifdef __DEBUG_GRAPHICS__
static void pixel(const GL_Context_t *context, int x, int y, int index)
{
    *PIXEL(context, x, y) = 240 + (index % 16);
}
#endif

bool GL_context_create(GL_Context_t *context, size_t width, size_t height)
{
    void *vram = malloc(width * height * sizeof(GL_Pixel_t));
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
        vram_rows[i] = (GL_Pixel_t *)vram + (width * i);
    }

    Log_write(LOG_LEVELS_DEBUG, "<GL> VRAM allocated at #%p (%dx%d)", vram, width, height);

    *context = (GL_Context_t){
            .width = width,
            .height = height,
            .stride = width * sizeof(GL_Pixel_t),
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

    return true;
}

void GL_context_delete(GL_Context_t *context)
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
    const GL_Pixel_t color = context->background;
    GL_Pixel_t *dst = (GL_Pixel_t *)context->vram;
    for (size_t i = context->vram_size; i > 0; --i) {
        *(dst++) = color;
    }
}

void GL_context_screenshot(const GL_Context_t *context, const GL_Palette_t *palette, const char *pathfile)
{
    void *vram = malloc(context->width * context->height * sizeof(GL_Color_t));
    if (!vram) {
        Log_write(LOG_LEVELS_WARNING, "<GL> can't create buffer for screenshot");
    }

    GL_context_to_rgba(context, palette, vram);

    int result = stbi_write_png(pathfile, context->width, context->height, sizeof(GL_Color_t), vram, context->width * sizeof(GL_Color_t));
    if (!result) {
        Log_write(LOG_LEVELS_WARNING, "<GL> can't save screenshot to '%s'", pathfile);
    }

    free(vram);
}

void GL_context_to_rgba(const GL_Context_t *context, const GL_Palette_t *palette, void *vram)
{
    const int width = context->width;
    const int height = context->height;
    const GL_Color_t *colors = palette->colors;
    int count = palette->count;
    const GL_Pixel_t *src = (const GL_Pixel_t *)context->vram;
    GL_Color_t *dst = (GL_Color_t *)vram;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
#ifdef __DEBUG_GRAPHICS__
            GL_Pixel_t index = *src++;
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

// TODO: specifies `const` always? Is pedantic or useful?
// TODO: define a `BlitInfo` and `BlitFunc` types to generalize?
// https://dev.to/fenbf/please-declare-your-variables-as-const
void GL_context_blit(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position)
{
    const GL_Quad_t clipping_region = context->clipping_region;
    const GL_Pixel_t *shifting = context->shifting;
    const GL_Bool_t *transparent = context->transparent;

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
    GL_Pixel_t *dst = (GL_Pixel_t *)context->vram_rows[drawing_region.y0] + drawing_region.x0;

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
    const GL_Quad_t clipping_region = context->clipping_region;
    const GL_Pixel_t *shifting = context->shifting;
    const GL_Bool_t *transparent = context->transparent;

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

    GL_Pixel_t *dst = (GL_Pixel_t *)context->vram_rows[drawing_region.y0] + drawing_region.x0;

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
    const GL_Quad_t clipping_region = context->clipping_region;
    const GL_Pixel_t *shifting = context->shifting;
    const GL_Bool_t *transparent = context->transparent;

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

    // Clip both destination and target rectangles.
    // Note, `floorf()` is not needed here, since we have always a positive-valued clipping region.
    const int dminx = (int)fmax(aabb_x0 + px, (float)clipping_region.x0);
    const int dminy = (int)fmax(aabb_y0 + py, (float)clipping_region.y0);
    const int dmaxx = (int)fmin(aabb_x1 + px, (float)clipping_region.x1);
    const int dmaxy = (int)fmin(aabb_y1 + py, (float)clipping_region.y1);

    const int width = dmaxx - dminx + 1;
    const int height = dmaxy - dminy + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    const int sminx = area.x;
    const int sminy = area.y;
    const int smaxx = area.x + area.width - 1;
    const int smaxy = area.y + area.height - 1;

    const float M11 = c / scale_x;  // Combine (inverse) rotation and *then* scaling matrices.
    const float M12 = s / scale_x;  // | 1/sx    0 | |  c s |
    const float M21 = -s / scale_y; // |           | |      |
    const float M22 = c / scale_y;  // |    0 1/sy | | -s c |

    const float tlx = (dminx - px); // Transform the top-left corner of the to-be-drawn rectangle to texture space.
    const float tly = (dminy - py); // (could differ from AABB x0 due to clipping, we need to compute it again)
    float ou = (tlx * M11 + tly * M12) + stx + ox; // Offset to the source texture quad.
    float ov = (tlx * M21 + tly * M22) + sty + oy;

    GL_Pixel_t *dst = (GL_Pixel_t *)context->vram_rows[dminy] + dminx;

    const int skip = context->width - width;

    for (int i = height; i; --i) {
        float u = ou;
        float v = ov;

        for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
            pixel(context, dminx + width - j, dminy + height - i, 15);
#endif
            int x = (int)floorf(u); // Round down, to preserve negative values as such (e.g. `-0.3` is `-1`)
            int y = (int)floorf(v);

            if (x >= sminx && x <= smaxx && y >= sminy && y <= smaxy) {
#ifdef __DEBUG_GRAPHICS__
                pixel(context, dminx + width - j, dminy + height - i, (int)i + (int)j);
#endif
                const GL_Pixel_t *src = (const GL_Pixel_t *)surface->data_rows[y] + x;
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
    pixel(context, dminx, dminy, 7);
    pixel(context, dmaxx, dminy, 7);
    pixel(context, dmaxx, dmaxy, 7);
    pixel(context, dminx, dmaxy, 7);
#endif
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
    context->background = index;
}

void GL_context_color(GL_Context_t *context, GL_Pixel_t index)
{
    context->color = index;
}

void GL_context_pattern(GL_Context_t *context, uint32_t mask)
{
    context->mask = mask;
}

// https://lodev.org/cgtutor/floodfill.html
void GL_context_fill(const GL_Context_t *context, GL_Point_t seed, GL_Pixel_t index)
{
    const GL_Quad_t clipping_region = context->clipping_region;

    if (seed.x < clipping_region.x0 || seed.x > clipping_region.x1
        || seed.y < clipping_region.y0 || seed.y > clipping_region.y1) {
        return;
    }

    GL_Pixel_t match = *PIXEL(context, seed.x, seed.y);
    GL_Pixel_t replacement = index;

    GL_Point_t *stack = NULL;
    arrpush(stack, seed);

    while (arrlen(stack) > 0) {
        GL_Point_t position = arrpop(stack);

        int x = position.x;
        int y = position.y;

        while (x >= clipping_region.x0 && !memcmp(PIXEL(context, x, y), &match, sizeof(GL_Pixel_t))) {
            --x;
        }
        ++x;

        bool above = false;
        bool below = false;

        while (x <= clipping_region.x1 && !memcmp(PIXEL(context, x, y), &match, sizeof(GL_Pixel_t))) {
            *PIXEL(context, x, y) = replacement;

            if (!above && y >= clipping_region.y0 && !memcmp(PIXEL(context, x, y - 1), &match, sizeof(GL_Pixel_t))) {
                GL_Point_t p = (GL_Point_t){ .x = x, .y = y - 1 };
                arrpush(stack, p);
                above = true;
            } else
            if (above && y >= clipping_region.y0 && memcmp(PIXEL(context, x, y - 1), &match, sizeof(GL_Pixel_t))) {
                above = false;
            }

            if (!below && y < clipping_region.y1 && !memcmp(PIXEL(context, x, y + 1), &match, sizeof(GL_Pixel_t))) {
                GL_Point_t p = (GL_Point_t){ .x = x, .y = y + 1 };
                arrpush(stack, p);
                below = true;
            } else
            if (below && y < clipping_region.y1 && memcmp(PIXEL(context, x, y + 1), &match, sizeof(GL_Pixel_t))) {
                above = false;
            }

            ++x;
        }
    }

    arrfree(stack);
}

// https://www.youtube.com/watch?v=3FVN_Ze7bzw
// http://www.coranac.com/tonc/text/mode7.htm
void GL_context_blit_x(const GL_Context_t *context, const GL_Surface_t *surface, GL_Point_t position, GL_Transformation_t transformation) // TODO: add scanline callback or op-table
{
    const GL_Quad_t clipping_region = context->clipping_region;
    const GL_Pixel_t *shifting = context->shifting;
    const GL_Bool_t *transparent = context->transparent;

    const float h = transformation.h;
    const float v = transformation.v;
    const float x0 = transformation.x0;
    const float y0 = transformation.y0;
    const float a = transformation.a;
    const float b = transformation.b;
    const float c = transformation.c;
    const float d = transformation.d;
    const int clamp = transformation.clamp;
    const float elevation = transformation.elevation;
    const float horizon = transformation.horizon;
    const bool perspective = transformation.perspective;

    const float yh = perspective ? horizon : 0.0f;
    const float ya = elevation;

    const int offset_y = perspective ? (int)yh + 1 : 0;

    GL_Quad_t drawing_region = (GL_Quad_t) {
        .x0 = position.x,
        .y0 = position.y + offset_y, // Skip the horizon line.
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

    const int ox = position.x;
    const int oy = position.y;

    const int sw = surface->width;
    const int sh = surface->height;
    const int sminx = 0;
    const int sminy = 0;
    const int smaxx = sw - 1;
    const int smaxy = sh - 1;

    GL_Pixel_t *dst = (GL_Pixel_t *)context->vram_rows[drawing_region.y0] + drawing_region.x0;

    const int skip = context->width - width;

    for (int y = drawing_region.y0; y <= drawing_region.y1; ++y) {
        const float yi = ((float)(y - oy) - yh) + v - y0;

        const float p = perspective
            ? ya / ((float)(y - oy) - yh)
            : 1.0f;

        const float pa = p * a; float pb =  p * b;
        const float pc = p * c; float pd =  p * d;

        for (int x = drawing_region.x0; x <= drawing_region.x1; ++x) {
#ifdef __DEBUG_GRAPHICS__
            pixel(context, x, y, x + y);
#endif
            const float xi = (float)(x - ox) + h - x0;

            const float xp = (pa * xi + pb * yi) + x0;
            const float yp = (pc * xi + pd * yi) + y0;

            int sx = (int)xp;
            int sy = (int)yp;

            if (clamp == GL_CLAMP_MODE_REPEAT) {
                sx = iabs(sx) % sw;
                sy = iabs(sy) % sh;
            } else
            if (clamp == GL_CLAMP_MODE_EDGE) {
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
                const GL_Pixel_t *src = (const GL_Pixel_t *)surface->data_rows[sy] + sx;
                GL_Pixel_t index = shifting[*src];
                if (!transparent[index]) {
                    *dst = index;
                }
            }

            ++dst;
        }

        dst += skip;
    }
}
