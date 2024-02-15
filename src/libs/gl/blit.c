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

#include "blit.h"

#include <core/config.h>
#include <libs/fmath.h>
#include <libs/imath.h>
#include <libs/sincos.h>

#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
static inline void _pixel(const GL_Surface_t *surface, int x, int y, int index)
{
    surface->data[y * surface->width + x]= (GL_Pixel_t)(240 + (index % 16));
}
#endif

// TODO: specifies `const` always? Is pedantic or useful?
// https://dev.to/fenbf/please-declare-your-variables-as-const
void GL_context_blit(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    int skip_x = area.x; // Offset into the (source) surface/texture, updated during clipping.
    int skip_y = area.y;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)area.width,
            .y1 = position.y + (int)area.height
        };

    if (drawing_region.x0 < clipping_region->x0) {
        skip_x += clipping_region->x0 - drawing_region.x0;
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        skip_y += clipping_region->y0 - drawing_region.y0;
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0;
    const int height = drawing_region.y1 - drawing_region.y0;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t sskip = swidth - width;
    const size_t dskip = dwidth - width;

    const GL_Pixel_t *sptr = sdata + skip_y * swidth + skip_x;
    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    for (int i = height; i; --i) {
        for (int j = width; j; --j) {
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
            _pixel(surface, drawing_region.x0 + width - j, drawing_region.y0 + height - i, i + j);
#endif
            const GL_Pixel_t index = shifting[*(sptr++)];
            if (transparent[index]) {
                ++dptr;
            } else {
                *(dptr++) = index;
            }
        }
        sptr += sskip;
        dptr += dskip;
    }
}

// Simple implementation of nearest-neighbour scaling, with x/y flipping according to scaling-factor sign.
// See `http://tech-algorithm.com/articles/nearest-neighbor-image-scaling/` for a reference code.
// To avoid empty pixels we scan the destination area and calculate the source pixel.
//
// http://www.datagenetics.com/blog/december32013/index.html
// file:///C:/Users/mlizza/Downloads/Extensible_Implementation_of_Reliable_Pixel_Art_In.pdf
void GL_context_blit_s(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, float scale_x, float scale_y)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    const size_t drawing_width = (size_t)ITRUNC(area.width * fabsf(scale_x)); // Truncate, or we might "bleed" and pick from outside the source area.
    const size_t drawing_height = (size_t)ITRUNC(area.height * fabsf(scale_y));

    float skip_x = 0; // Offset into the (source) surface/texture, updated during clipping.
    float skip_y = 0;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = position.x,
            .y0 = position.y,
            .x1 = position.x + (int)drawing_width,
            .y1 = position.y + (int)drawing_height,
        };

    if (drawing_region.x0 < clipping_region->x0) {
        skip_x += (float)(clipping_region->x0 - drawing_region.x0);
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        skip_y += (float)(clipping_region->y0 - drawing_region.y0);
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0;
    const int height = drawing_region.y1 - drawing_region.y0;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t dskip = dwidth - width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    // The scaling formula is the following:
    //
    //   x_s = round((x_r + 0.5) / S_x - 0.5) = floor((x_r + 0.5) / S_x)
    //   y_s = round((y_r + 0.5) / S_y - 0.5) = floor((y_r + 0.5) / S_y)
    //
    // Notice that we need to work in the mid-center of the pixels. We can also rewrite the
    // formula in a recurring fashion if we increment and accumulate by `1 / S_x` and `1 / S_y` steps.
    const float ou0 = (skip_x + 0.5f) / scale_x;
    const float ov0 = (skip_y + 0.5f) / scale_y; // `skip_*` is never negative, so we can check the sign!
    const float ou = (float)area.x + (ou0 < 0.0f ? (float)area.width + ou0 : ou0); // Offset to the correct margin, according to flipping.
    const float ov = (float)area.y + (ov0 < 0.0f ? (float)area.height + ov0 : ov0);

    const float du = 1.0f / scale_x; // Retain sign of the scaling to move according to a "vector" along the scaling.
    const float dv = 1.0f / scale_y;

    float v = ov;
    for (int i = height; i; --i) {
        const int y = ITRUNC(v); // Truncate, as we used `ITRUNC()` to calculate the scaled size.
        const GL_Pixel_t *sptr = sdata + y * swidth;

        float u = ou;
        for (int j = width; j; --j) {
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
            _pixel(surface, drawing_region.x0 + width - j, drawing_region.y0 + height - i, (int)u + (int)v);
#endif
            const int x = ITRUNC(u); // Ditto.
            GL_Pixel_t index = shifting[sptr[x]];
            if (transparent[index]) {
                ++dptr;
            } else {
                *(dptr++) = index;
            }
            u += du;
        }

        v += dv;
        dptr += dskip;
    }
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
    _pixel(surface, drawing_region.x0, drawing_region.y0, 7);
    _pixel(surface, drawing_region.x1, drawing_region.y0, 7);
    _pixel(surface, drawing_region.x1, drawing_region.y1, 7);
    _pixel(surface, drawing_region.x0, drawing_region.y1, 7);
#endif
}

void GL_context_blit_sr(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, float scale_x, float scale_y, int rotation, float anchor_x, float anchor_y)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const GL_Pixel_t *shifting = state->shifting;
    const GL_Bool_t *transparent = state->transparent;

    const float sw = (float)area.width;
    const float sh = (float)area.height;
    const float dw = sw * FABS(scale_x);
    const float dh = sh * FABS(scale_y);

    const float sax = (sw - 1.0f) * anchor_x; // Anchor points, relative to the source and destination areas.
    const float say = (sh - 1.0f) * anchor_y;
    const float dax = (dw - 1.0f) * anchor_x;
    const float day = (dh - 1.0f) * anchor_y;

    const float sx = (float)area.x + sax; // Total translation, anchor offset *and* source area origin.
    const float sy = (float)area.y + say;
    const float dx = (float)position.x;
    const float dy = (float)position.y;

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

    // Precompute the "target disc": where we must draw pixels of the rotated sprite (relative to (x, y)).
    // The radius of the disc is the distance between the anchor point and the farthest corner of the
    // sprite rectangle, i.e. the magnitude of a vector with
    //   - the biggest horizontal distance between anchor point and rectangle left or right (as width), and
    //   - the biggest vertical distance between anchor point and rectangle top or bottom (as height).
    // If the anchor point is a corner, it is the full diagonal length.

    // Measure distance between anchor and edge pixel center, so anchor vs 0.5 (start) or sw - 0.5 (end)
    // Note that in the operations below, we work "inside" pixels as much as possible (offset 0.5 from top-left corner)
    const float delta_x = fmaxf(dax, dw - dax) - 0.5f;
    const float delta_y = fmaxf(day, dh - day) - 0.5f;
    const float radius_squared = delta_x * delta_x + delta_y * delta_y;
    const float radius = ceilf(sqrtf(radius_squared)); // Ensure room for every pixel.

    const float aabb_x0 = -radius; // The rotation AABB is a tad exaggerated, we'll optimize the scan by using the (squared) radius.
    const float aabb_y0 = -radius;
    const float aabb_x1 = radius;
    const float aabb_y1 = radius;

    float skip_x = aabb_x0; // Offset into the target surface/texture, updated during clipping.
    float skip_y = aabb_y0;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = ICEILF(aabb_x0 + dx), // To include every fractionally occupied pixel.
            .y0 = ICEILF(aabb_y0 + dy),
            .x1 = ICEILF(aabb_x1 + dx) + 1,
            .y1 = ICEILF(aabb_y1 + dy) + 1
        };

    if (drawing_region.x0 < clipping_region->x0) {
        skip_x += (float)(clipping_region->x0 - drawing_region.x0); // Add clipped part, we'll skip it.
        drawing_region.x0 = clipping_region->x0;
    }
    if (drawing_region.y0 < clipping_region->y0) {
        skip_y += (float)(clipping_region->y0 - drawing_region.y0); // Ditto.
        drawing_region.y0 = clipping_region->y0;
    }
    if (drawing_region.x1 > clipping_region->x1) {
        drawing_region.x1 = clipping_region->x1;
    }
    if (drawing_region.y1 > clipping_region->y1) {
        drawing_region.y1 = clipping_region->y1;
    }

    const int width = drawing_region.x1 - drawing_region.x0;
    const int height = drawing_region.y1 - drawing_region.y0;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    const int sminx = area.x;
    const int sminy = area.y;
    const int smaxx = sminx + (int)area.width;
    const int smaxy = sminy + (int)area.height;

    const float M11 = c / scale_x;  // Since we are doing an *inverse* transformation, we combine rotation and *then* scaling *and* flip (TRSF -> FSRT).
    const float M12 = s / scale_x;  // | fx  0 | | 1/sx    0 | |  c s |
    const float M21 = -s / scale_y; // |       | |           | |      | NOTE: flip sign is already fused in the scale factor!
    const float M22 = c / scale_y;  // |  0 fy | |    0 1/sy | | -s c |

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t dskip = dwidth - width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    for (int i = 0; i < height; ++i) {
        const float ov = skip_y + (float)i; // + 0.5f;
#if defined(TOFU_GRAPHICS_OPTIMIZED_ROTATIONS)
        const float ov_squared = ov * ov;
#endif

        for (int j = 0; j < width; ++j) {
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
            _pixel(surface, drawing_region.x0 + j, drawing_region.y0 + i, 15);
#endif
            const float ou = skip_x + (float)j; // + 0.5f;
#if defined(TOFU_GRAPHICS_OPTIMIZED_ROTATIONS)
            const float ou_squared = ou * ou;
            const float distance_squared = ov_squared + ou_squared;
            if (distance_squared <= radius_squared) {
#endif
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
                _pixel(surface, drawing_region.x0 + j, drawing_region.y0 + i, 11);
#endif

                const float u = (ou * M11 + ov * M12) + sx + 0.5f; // Important: offset half a pixel to center the source texture!
                const float v = (ou * M21 + ov * M22) + sy + 0.5f; // (see variables initialization why we are using sx/sx solely)

                int x = IFLOORF(u); // Round down, to preserve negative values as such (e.g. `-0.3` is `-1`) and avoid mirror effect.
                int y = IFLOORF(v); // (can't truncate, because negatives would be truncated toward zero)

                if (x >= sminx && x < smaxx && y >= sminy && y < smaxy) {
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
                    _pixel(surface, drawing_region.x0 + j, drawing_region.y0 + i, 3);
#endif
                    const GL_Pixel_t *sptr = sdata + y * swidth + x;
                    GL_Pixel_t index = shifting[*sptr];
                    if (!transparent[index]) {
                        *dptr = index;
                    }
                }
#if defined(TOFU_GRAPHICS_OPTIMIZED_ROTATIONS)
            }
#endif

            ++dptr;
        }

        dptr += dskip;
    }
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
    _pixel(surface, dx, dy, 7);
    _pixel(surface, drawing_region.x0    , drawing_region.y0    , 7);
    _pixel(surface, drawing_region.x1 - 1, drawing_region.y0    , 7);
    _pixel(surface, drawing_region.x1 - 1, drawing_region.y1 - 1, 7);
    _pixel(surface, drawing_region.x0    , drawing_region.y1 - 1, 7);
#endif
}
