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

#include "xform.h"

#include <config.h>
#include <libs/imath.h>
#include <libs/sincos.h>

#include <math.h>

// https://www.youtube.com/watch?v=3FVN_Ze7bzw
// http://www.coranac.com/tonc/text/mode7.htm
// https://wiki.superfamicom.org/registers
// https://www.smwcentral.net/?p=viewthread&t=27054
void GL_context_xform(const GL_Context_t *context, const GL_Surface_t *surface, GL_Rectangle_t area, GL_Point_t position, const GL_XForm_t *xform)
{
    const GL_State_t *state = &context->state;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
#ifdef __GL_MODE7_TRANSPARENCY__
    const GL_Bool_t *transparent = state->transparent;
#endif  /* __GL_MODE7_TRANSPARENCY__ */

    const int clamp = xform->clamp;
    const GL_XForm_Table_Entry_t *table = xform->table;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (clipping_region->x1 - clipping_region->x0), // We need to scan the whole destination region.
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

    const int sw = (int)area.width;
    const int sh = (int)area.height;
    const int swm1 = sw - 1;
    const int shm1 = sh - 1;
    const int sminx = area.x;
    const int sminy = area.y;

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
            int sx = (int)xp;
            int sy = (int)yp;

            bool copy = true;
            if (clamp == GL_XFORM_CLAMP_REPEAT) {
                if (surface->is_power_of_two) {
                    sx = sx & swm1;
                    sy = sy & shm1;
                } else {
                    sx = imod(sx, sw);
                    sy = imod(sy, sh);
                }
            } else
            if (clamp == GL_XFORM_CLAMP_EDGE) {
                if (sx < 0) {
                    sx = 0;
                } else
                if (sx > swm1) {
                    sx = swm1;
                }
                if (sy < 0) {
                    sy = 0;
                } else
                if (sy > shm1) {
                    sy = shm1;
                }
            } else
            if (clamp == GL_XFORM_CLAMP_BORDER) {
                if (sx < 0 || sx > swm1 || sy < 0 || sy > shm1) {
                    copy = false;
                }
            } else {
                copy = false;
            }

            if (copy) {
                sx += sminx;
                sy += sminy;

                const GL_Pixel_t *sptr = sdata + sy * swidth + sx;
                GL_Pixel_t index = shifting[*sptr];
                // NOTE: no transparency in Mode-7!
#ifdef __GL_MODE7_TRANSPARENCY__
                if (!transparent[index]) {
                    *dptr = index;
                }
#else
                *dptr = index;
#endif  /* __GL_MODE7_TRANSPARENCY__ */
            }

            ++dptr;

            xp += a;
            yp += c;
        }

        dptr += dskip;
    }
}
