/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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

#include "blit.h"

#include <config.h>
#include <libs/imath.h>
#include <libs/sincos.h>

#include <math.h>

#ifdef __DEBUG_GRAPHICS__
static inline void pixel(const GL_Context_t *context, int x, int y, int index)
{
    GL_Surface_t *surface = context->state->surface;
    surface->data[y * surface->width + x]= 240 + (index % 16);
}
#endif

// TODO: specifies `const` always? Is pedantic or useful?
// TODO: define a `BlitInfo` and `BlitFunc` types to generalize?
// https://dev.to/fenbf/please-declare-your-variables-as-const
void GL_context_blit(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
#ifdef __GL_MASK_SUPPORT__
    const GL_Mask_t *mask = &state->mask;
#endif

    GL_Quad_t drawing_region = (GL_Quad_t){ // FIXME: remove `GL_Quad_t` usage!
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)area.width - 1,
            .y1 = position.y + (int)area.height - 1
        };

    int skip_x = 0; // Offset into the (source) surface/texture, update during clipping.
    int skip_y = 0;

    if (drawing_region.x0 < clipping_region->x0) {
        skip_x = clipping_region->x0 - drawing_region.x0;
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        skip_y = clipping_region->y0 - drawing_region.y0;
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    const GL_Pixel_t *sdata = surface->data;
    GL_Pixel_t *ddata = context->surface->data;

    const int swidth = (int)surface->width;
    const int dwidth = (int)context->surface->width;

    const GL_Pixel_t *sptr = sdata + (area.y + skip_y) * swidth + (area.x + skip_x);
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const int sskip = swidth - width;
    const int dskip = dwidth - width;
#ifdef __GL_MASK_SUPPORT__
    if (mask->stencil) {
        const GL_Surface_t *stencil = mask->stencil;
        const GL_Pixel_t threshold = mask->threshold;

        const GL_Pixel_t *mptr = stencil->data_rows[area.y + skip_y] + (area.x + skip_x);

        const int mskip = stencil->width - width;

        for (int i = height; i; --i) {
            for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
                pixel(context, drawing_region.x0 + width - j, drawing_region.y0 + height - i, (int)i + (int)j);
#endif
                GL_Pixel_t index = shifting[*(sptr++)];
                GL_Pixel_t mask = *(mptr++);
                if (transparent[index] || (mask < threshold)) {
                    dptr++;
                } else {
                    *(dptr++) = index;
                }
            }
            sptr += sskip;
            mptr += mskip;
            dptr += dskip;
        }
    } else {
#endif
        for (int i = height; i; --i) {
            for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
                pixel(context, drawing_region.x0 + width - j, drawing_region.y0 + height - i, (int)i + (int)j);
#endif
                GL_Pixel_t index = shifting[*(sptr++)];
                if (transparent[index]) {
                    dptr++;
                } else {
                    *(dptr++) = index;
                }
            }
            sptr += sskip;
            dptr += dskip;
        }
#ifdef __GL_MASK_SUPPORT__
    }
#endif
}

static inline int _iroundf(float x)
{
    return (int)floorf(x + 0.5f);
}

#if 0
static inline int _ifloorf(float x)
{
    return (int)floorf(x);
}
#endif

// Simple implementation of nearest-neighbour scaling, with x/y flipping according to scaling-factor sign.
// See `http://tech-algorithm.com/articles/nearest-neighbor-image-scaling/` for a reference code.
// To avoid empty pixels we scan the destination area and calculate the source pixel.
void GL_context_blit_s(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position, float scale_x, float scale_y)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
#ifdef __GL_MASK_SUPPORT__
    const GL_Mask_t *mask = &state->mask;
#endif

    const bool flip_x = scale_x < 0.0f;
    const bool flip_y = scale_y < 0.0f;

    const int drawing_width = _iroundf(area.width * fabsf(scale_x));
    const int drawing_height = _iroundf(area.height * fabsf(scale_y));

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + drawing_width - 1,
            .y1 = position.y + drawing_height - 1,
        };

    float skip_x = 0.0f; // Offset into the (source) surface/texture, update during clipping.
    float skip_y = 0.0f;

    if (drawing_region.x0 < clipping_region->x0) {
        skip_x = (float)(clipping_region->x0 - drawing_region.x0) / fabs(scale_x);
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        skip_y = (float)(clipping_region->y0 - drawing_region.y0) / fabs(scale_y);
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    const GL_Pixel_t *sdata = surface->data;
    GL_Pixel_t *ddata = context->surface->data;

    const int swidth = (int)surface->width;
    const int dwidth = (int)context->surface->width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const int dskip = dwidth - width;

    const float du = 1.0f / scale_x; // Texture coordinates deltas (signed).
    const float dv = 1.0f / scale_y;

    float ou = flip_x // Compute origin, correcting to rightmost/bottom margin when flipping.
        ? (float)area.x + (float)area.width - fabs(du)
        : (float)area.x + skip_x;
    float ov = flip_y
        ? (float)area.y + (float)area.height - fabs(dv)
        : (float)area.y + skip_y;

    // NOTE: we can also apply an integer-based DDA method, using remainders.

#ifdef __GL_MASK_SUPPORT__
    if (mask->stencil) {
        const GL_Surface_t *stencil = mask->stencil;
        const GL_Pixel_t threshold = mask->threshold;

        float v = ov;
        for (int i = height; i; --i) {
            const GL_Pixel_t *sptr = surface->data_rows[(int)v];
            const GL_Pixel_t *mptr = stencil->data_rows[(int)v];

            float u = ou;
            for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
                pixel(context, drawing_region.x0 + width - j, drawing_region.y0 + height - i, (int)u + (int)v);
#endif
                GL_Pixel_t index = shifting[sptr[(int)u]];
                GL_Pixel_t mask = mptr[(int)u];
                if (transparent[index] || (mask < threshold)) {
                    dptr++;
                } else {
                    *(dptr++) = index;
                }
                u += du;
            }

            v += dv;
            dptr += dskip;
        }
    } else {
#endif
        float v = ov;
        for (int i = height; i; --i) {
            const GL_Pixel_t *sptr = sdata + (int)v * swidth;

            float u = ou;
            for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
                pixel(context, drawing_region.x0 + width - j, drawing_region.y0 + height - i, (int)u + (int)v);
#endif
                GL_Pixel_t index = shifting[sptr[(int)u]];
                if (transparent[index]) {
                    dptr++;
                } else {
                    *(dptr++) = index;
                }
                u += du;
            }

            v += dv;
            dptr += dskip;
        }
#ifdef __GL_MASK_SUPPORT__
    }
#endif
}

// https://web.archive.org/web/20190305223938/http://www.drdobbs.com/architecture-and-design/fast-bitmap-rotation-and-scaling/184416337
// https://www.flipcode.com/archives/The_Art_of_Demomaking-Issue_10_Roto-Zooming.shtml
//
// FIXME: one row/column is lost due to rounding errors when angle is multiple of 128.
void GL_context_blit_sr(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position, float scale_x, float scale_y, int rotation, float anchor_x, float anchor_y)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;
#ifdef __GL_MASK_SUPPORT__
    const GL_Mask_t *mask = &state->mask;
#endif

    const bool flip_x = scale_x < 0.0f;
    const bool flip_y = scale_y < 0.0f;

    const float asx = fabs(scale_x);
    const float asy = fabs(scale_y);

    const float sw = (float)area.width;
    const float sh = (float)area.height;
    const float dw = sw * asx;
    const float dh = sh * asy;

    const float sax = (sw - 1.0f) * anchor_x; // Anchor points, relative to the source and destination areas.
    const float say = (sh - 1.0f) * anchor_y;
    const float dax = (dw - 1.0f) * anchor_x;
    const float day = (dh - 1.0f) * anchor_y;

    const float sx = area.x;
    const float sy = area.y;
    const float dx = position.x;
    const float dy = position.y;

    float s, c;
    fsincos(rotation, &s, &c);

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
    const float aabb_x1 = dw - 1.0f - dax;
    const float aabb_y1 = dh - 1.0f - day;

    const float x0 = c * aabb_x0 - s * aabb_y0;
    const float y0 = s * aabb_x0 + c * aabb_y0;

    const float x1 = c * aabb_x1 - s * aabb_y0;
    const float y1 = s * aabb_x1 + c * aabb_y0;

    const float x2 = c * aabb_x1 - s * aabb_y1;
    const float y2 = s * aabb_x1 + c * aabb_y1;

    const float x3 = c * aabb_x0 - s * aabb_y1;
    const float y3 = s * aabb_x0 + c * aabb_y1;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = _iroundf(fmin(fmin(fmin(x0, x1), x2), x3) + dx),
            .y0 = _iroundf(fmin(fmin(fmin(y0, y1), y2), y3) + dy),
            .x1 = _iroundf(fmax(fmax(fmax(x0, x1), x2), x3) + dx),
            .y1 = _iroundf(fmax(fmax(fmax(y0, y1), y2), y3) + dy)
        };

    if (drawing_region.x0 < clipping_region->x0) {
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    const int sminx = area.x;
    const int sminy = area.y;
    const int smaxx = area.x + (int)area.width - 1;
    const int smaxy = area.y + (int)area.height - 1;

    const float M11 = c / asx;  // Since we are doing an *inverse* transformation, we combine rotation and *then* scaling (TRS -> SRT).
    const float M12 = s / asx;  // | 1/sx    0 | |  c s |
    const float M21 = -s / asy; // |           | |      |
    const float M22 = c / asy;  // |    0 1/sy | | -s c |

    const float tlx = (float)drawing_region.x0 - dx; // Transform the top-left corner of the to-be-drawn rectangle to texture space.
    const float tly = (float)drawing_region.y0 - dy; // (could differ from AABB x0 due to clipping, we need to compute it again)
    float ou = (tlx * M11 + tly * M12) + sax + sx; // Offset to the source texture quad.
    float ov = (tlx * M21 + tly * M22) + say + sy;

    const GL_Pixel_t *sdata = surface->data;
    GL_Pixel_t *ddata = context->surface->data;

    const int swidth = (int)surface->width;
    const int dwidth = (int)context->surface->width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const int dskip = dwidth - width;

#if 0
    int (* const round)(float) = (rotation % 512 == 0) ? _ifloorf : _iroundf; // No rounding when no rotation is specified.
#endif

#ifdef __GL_MASK_SUPPORT__
    if (mask->stencil) {
        const GL_Surface_t *stencil = mask->stencil;
        const GL_Pixel_t threshold = mask->threshold;

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
                    pixel(context, drawing_region.x0 + width - j, drawing_region.y0 + height - i, i + j);
#endif
                    const GL_Pixel_t *sptr = surface->data_rows[y] + x;
                    const GL_Pixel_t *mptr = stencil->data_rows[y] + x;
                    GL_Pixel_t index = shifting[*sptr];
                    GL_Pixel_t mask = *mptr;
                    if (transparent[index] || (mask < threshold)) {
                        *dptr = index;
                    }
                }

                ++dptr;

                u += M11;
                v += M21;
            }

            dptr += dskip;

            ou += M12;
            ov += M22;
        }
    } else {
#endif
        for (int i = height; i; --i) {
            float u = ou;
            float v = ov;

            for (int j = width; j; --j) {
#ifdef __DEBUG_GRAPHICS__
                pixel(context, drawing_region.x0 + width - j, drawing_region.y0 + height - i, 15);
#endif
#if 0
                int x = round(u); // Round down, to preserve negative values as such (e.g. `-0.3` is `-1`) and avoid mirror effect.
                int y = round(v);
#else
                int x = _iroundf(u); // Round down, to preserve negative values as such (e.g. `-0.3` is `-1`) and avoid mirror effect.
                int y = _iroundf(v);
#endif

                if (x >= sminx && x <= smaxx && y >= sminy && y <= smaxy) {
#ifdef __DEBUG_GRAPHICS__
                    pixel(context, drawing_region.x0 + width - j, drawing_region.y0 + height - i, i + j);
#endif
                    if (flip_x) {
                        x = smaxx - (x - sminx);
                    }
                    if (flip_y) {
                        y = smaxy - (y - sminy);
                    }

                    const GL_Pixel_t *sptr = sdata + y * swidth + x;
                    GL_Pixel_t index = shifting[*sptr];
                    if (!transparent[index]) {
                        *dptr = index;
                    }
                }

                ++dptr;

                u += M11;
                v += M21;
            }

            dptr += dskip;

            ou += M12;
            ov += M22;
        }
#ifdef __GL_MASK_SUPPORT__
    }
#endif
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
void GL_context_blit_x(const GL_Context_t *context, const GL_Surface_t *surface, GL_Point_t position, const GL_XForm_t *xform)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    const int clamp = xform->clamp;
    const GL_XForm_Table_Entry_t *table = xform->table;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (clipping_region->x1 - clipping_region->x0),
            .y1 = position.y + (clipping_region->y1 - clipping_region->y0)
        };

    if (drawing_region.x0 < clipping_region->x0) {
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0 + 1;
    const int height = drawing_region.y1 - drawing_region.y0 + 1;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!
        return;
    }

    const int sw = (int)surface->width;
    const int sh = (int)surface->height;
    const int sminx = 0;
    const int sminy = 0;
    const int smaxx = sw - 1;
    const int smaxy = sh - 1;

    const GL_Pixel_t *sdata = surface->data;
    GL_Pixel_t *ddata = context->surface->data;

    const int swidth = (int)surface->width;
    const int dwidth = (int)context->surface->width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const int dskip = dwidth - width;

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
    const float *registers = xform->registers;
    float h = registers[GL_XFORM_REGISTER_H]; float v = registers[GL_XFORM_REGISTER_V];
    float a = registers[GL_XFORM_REGISTER_A]; float b = registers[GL_XFORM_REGISTER_B];
    float c = registers[GL_XFORM_REGISTER_C]; float d = registers[GL_XFORM_REGISTER_D];
    float x0 = registers[GL_XFORM_REGISTER_X]; float y0 = registers[GL_XFORM_REGISTER_Y];

    for (int i = 0; i < height; ++i) {
        if (table && i == table->scan_line) {
            for (size_t k = 0; k < table->count; ++k) {
                const GL_XForm_Registers_t id = table->operations[k].id;
                const float value = table->operations[k].value;
                switch (id) {
                    case GL_XFORM_REGISTER_H: { h = value; } break;
                    case GL_XFORM_REGISTER_V: { v = value; } break;
                    case GL_XFORM_REGISTER_A: { a = value; } break;
                    case GL_XFORM_REGISTER_B: { b = value; } break;
                    case GL_XFORM_REGISTER_C: { c = value; } break;
                    case GL_XFORM_REGISTER_D: { d = value; } break;
                    case GL_XFORM_REGISTER_X: { x0 = value; } break;
                    case GL_XFORM_REGISTER_Y: { y0 = value; } break;
                    default: { } break;
                }
            }
            ++table;
#ifdef __DETACH_XFORM_TABLE__
            if (table->scan_line == -1) { // End-of-data reached, detach pointer for faster loop.
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
            int sx = _iroundf(xp); // Round to avoid artifacts.
            int sy = _iroundf(yp);

            if (clamp == GL_XFORM_CLAMP_REPEAT) {
                sx = imod(sx, sw); // TODO: optimize the amount of calls.
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
                const GL_Pixel_t *sptr = sdata + sy * swidth + sx;
                GL_Pixel_t index = shifting[*sptr];
                if (!transparent[index]) {
                    *dptr = index;
                }
            }

            ++dptr;

            xp += a;
            yp += c;
        }

        dptr += dskip;
    }
}
