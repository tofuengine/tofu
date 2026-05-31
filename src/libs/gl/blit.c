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

#include "blit.h"

#include <core/config.h>
#include <libs/fmath.h>
#include <libs/imath.h>
#include <libs/sincos.h>

//#define _BRANCHLESS_BLIT_EXPERIMENTAL
//#define _BRANCHLESS_BLIT_EXPERIMENTAL_XOR

#if defined(_BRANCHLESS_BLIT_EXPERIMENTAL)
#ifdef defined(_BRANCHLESS_BLIT_EXPERIMENTAL_XOR)
    #define _BLIT_BLEND(dindex, sindex, mask) ((dindex) ^ (((dindex) ^ (sindex)) & (mask)))
#else   /* defined(_BRANCHLESS_BLIT_EXPERIMENTAL_XOR) */
    #define _BLIT_BLEND(dindex, sindex, mask) (((dindex) & ~(mask)) | ((sindex) & (mask)))
#endif  /* defined(_BRANCHLESS_BLIT_EXPERIMENTAL_XOR) */
#endif  /* defined(_BRANCHLESS_BLIT_EXPERIMENTAL) */

#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
static inline void _pixel(const GL_Surface_t *surface, int x, int y, int index)
{
    surface->data[y * surface->width + x]= (GL_Pixel_t)(240 + (index % 16));
}
#endif  /* defined(TOFU_GRAPHICS_DEBUG_ENABLED) */

// TODO: specifies `const` always? Is pedantic or useful?
// https://dev.to/fenbf/please-declare-your-variables-as-const
void GL_context_blit(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const uint8_t *state_map = state->palette_state.map;
    const uint8_t bank_mask = state->palette_bank;

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
#endif  /* defined(TOFU_GRAPHICS_DEBUG_ENABLED) */

#if defined(_BRANCHLESS_BLIT_EXPERIMENTAL)
            const GL_Pixel_t pixel = *(sptr++);
            const uint8_t mapped = state_map[pixel];
            const GL_Pixel_t skip = GL_PALETTE_IS_TRANSPARENT(mapped);
            const GL_Pixel_t draw = 1 - skip;
            const GL_Pixel_t mask = (GL_Pixel_t)-draw;
            const GL_Pixel_t sindex = bank_mask | GL_PALETTE_GET_SHIFTING(mapped);
            const GL_Pixel_t dindex = *dptr;
            *(dptr++) = _BLIT_BLEND(dindex, sindex, mask);
#else   /* defined(_BRANCHLESS_BLIT_EXPERIMENTAL) */
            uint8_t mapped = state_map[*(sptr++)];
            if (!GL_PALETTE_IS_TRANSPARENT(mapped)) {
                *dptr = bank_mask | GL_PALETTE_GET_SHIFTING(mapped);
            }
            ++dptr;
#endif  /* defined(_BRANCHLESS_BLIT_EXPERIMENTAL) */
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
// https://www.researchgate.net/publication/333721765_Extensible_Implementation_of_Reliable_Pixel_Art_Interpolation
void GL_context_blit_s(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, float scale_x, float scale_y)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const uint8_t *state_map = state->palette_state.map;
    const uint8_t bank_mask = state->palette_bank;

    const int dx = position.x; // Offset into the (destination) surface, updated during clipping.
    const int dy = position.y;
    const int dw = (int)ITRUNC(area.width * FABS(scale_x)); // Truncate, or we might "bleed" and pick from outside the source area.
    const int dh = (int)ITRUNC(area.height * FABS(scale_y));

    if (dw <= 0 || dh <= 0) { // Nothing to draw! Bail out!
        return;
    }

    float skip_x = 0; // Offset into the (source) surface/texture, updated during clipping.
    float skip_y = 0;

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = dx,
            .y0 = dy,
            .x1 = dx + dw,
            .y1 = dy + dh,
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
    const float inv_scale_x = 1.0f / scale_x;
    const float inv_scale_y = 1.0f / scale_y;

    const float ou0 = (skip_x + 0.5f) * inv_scale_x;
    const float ov0 = (skip_y + 0.5f) * inv_scale_y; // `skip_*` is never negative, so we can check the sign!
    const float ou = (float)area.x + (ou0 < 0.0f ? (float)area.width + ou0 : ou0); // Offset to the correct margin, according to flipping.
    const float ov = (float)area.y + (ov0 < 0.0f ? (float)area.height + ov0 : ov0);

    const float du = inv_scale_x; // Retain sign of the scaling to move according to a "vector" along the scaling.
    const float dv = inv_scale_y;

    float v = ov;
    for (int i = height; i; --i) {
        const int y = ITRUNC(v); // Truncate, as we used `ITRUNC()` to calculate the scaled size.
        const GL_Pixel_t *sptr = sdata + y * swidth; // MULT instead of LUT access, more general-purpose (and not necessarily slower on modern CPUs with good branch prediction and pipelining).

        float u = ou;
        for (int j = width; j; --j) {
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
            _pixel(surface, drawing_region.x0 + width - j, drawing_region.y0 + height - i, (int)u + (int)v);
#endif  /* defined(TOFU_GRAPHICS_DEBUG_ENABLED) */
            const int x = ITRUNC(u); // Ditto.

#if defined(_BRANCHLESS_BLIT_EXPERIMENTAL)
            const GL_Pixel_t pixel = sptr[x];
            const uint8_t mapped = state_map[pixel];
            const GL_Pixel_t skip = GL_PALETTE_IS_TRANSPARENT(mapped);
            const GL_Pixel_t draw = 1 - skip;
            const GL_Pixel_t mask = (GL_Pixel_t)-draw;
            const GL_Pixel_t sindex = bank_mask | GL_PALETTE_GET_SHIFTING(mapped);
            const GL_Pixel_t dindex = *dptr;
            *(dptr++) = _BLIT_BLEND(dindex, sindex, mask);
#else   /* defined(_BRANCHLESS_BLIT_EXPERIMENTAL) */
            uint8_t mapped = state_map[sptr[x]];
            if (!GL_PALETTE_IS_TRANSPARENT(mapped)) {
                *dptr = bank_mask | GL_PALETTE_GET_SHIFTING(mapped);
            }
            ++dptr;
#endif  /* defined(_BRANCHLESS_BLIT_EXPERIMENTAL) */

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
#endif  /* defined(TOFU_GRAPHICS_DEBUG_ENABLED) */
}

#if !defined(TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS)
static inline void _rotate_point_around_pivot_with_offset(GL_PointF_t *point,
                                                          float pivot_x, float pivot_y,
                                                          float offset_x, float offset_y,
                                                          float M[4])
{
    float px = point->x - pivot_x;
    float py = point->y - pivot_y;

    point->x = px * M[0] + py * M[1] + offset_x;
    point->y = px * M[2] + py * M[3] + offset_y;
}

static inline void _rotate_point(GL_PointF_t *point,
                                 float M[4])
{
    float px = point->x;
    float py = point->y;

    point->x = px * M[0] + py * M[1];
    point->y = px * M[2] + py * M[3];
}

static inline float _fminf4(float a, float b, float c, float d)
{
    return fminf(fminf(a, b), fminf(c, d));
}

static inline float _fmaxf4(float a, float b, float c, float d)
{
    return fmaxf(fmaxf(a, b), fmaxf(c, d));
}
#endif  /* !defined(TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS) */

void GL_context_blit_sr(const GL_Context_t *context, GL_Point_t position, const GL_Surface_t *source, GL_Rectangle_t area, float scale_x, float scale_y, int rotation, float anchor_x, float anchor_y)
{
    const GL_Surface_t *surface = context->surface;
    const GL_State_t *state = &context->state.current;
    const GL_Quad_t *clipping_region = &state->clipping_region;
    const uint8_t *state_map = state->palette_state.map;
    const uint8_t bank_mask = state->palette_bank;

    const float sw = (float)area.width;
    const float sh = (float)area.height;
    const float dw = (float)ITRUNC(sw * FABS(scale_x)); // Match `GL_context_blit_s()`.
    const float dh = (float)ITRUNC(sh * FABS(scale_y));

    if ((dw <= 0.0f) || (dh <= 0.0f)) {
        return;
    }

    const float sx = (float)area.x;
    const float sy = (float)area.y;
    const float dx = (float)position.x + 0.5f; // Half-pixel aligned rotation, that's the pivot point indeed!
    const float dy = (float)position.y + 0.5f;

#if !defined(TOFU_GRAPHICS_BRANCHLESS_CALCULATIONS)
    const float sax = sw * anchor_x; // Anchor points in source/destination edge-space.
    const float say = sh * anchor_y;
#endif  /* !defined(TOFU_GRAPHICS_BRANCHLESS_CALCULATIONS) */
    const float dax = dw * anchor_x;
    const float day = dh * anchor_y;

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

    float s, c;
    fsincos(rotation, &s, &c);

#if !defined(TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS)
    // Forward transformation matrix, used to rotate the destination rectangle
    // corners and calculate the bounding box of the drawing region.
    float M[4] = {
            c, -s,
            s,  c
        };

    // Inverse transformation matrix, used to backward-map the destination pixel
    // to the source one. We combine inverse rotation, scaling, and flip
    // (TRSF -> FSRT).
    //
    // | fx  0 | | 1/sx    0 | |  c s |
    // |       | |           | |      | NOTE: flip sign is included in the `scale_*` value!
    // |  0 fy | |    0 1/sy | | -s c |
    float IMS[4] = {
            c / scale_x, s / scale_x,
            -s / scale_y, c / scale_y
        };
#endif  /* !defined(TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS) */

#if defined(TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS)
    // If we consider a centered AABB with half-size vector (or half-extent)
    // `<hx, hy>`, then every point of the rectangle can be represented with
    // the following linear combination:
    //
    //   p = a * <1, 0> + b * <0, 1>
    //
    // where `a` and `b` are in the range `[-hx, hx]` and `[-hy, hy]`,
    // respectively. When we apply the rotation to the point `p`, we can
    // rewrite the formula as
    //
    //   p' = a * <c, s> + b * <-s, c>
    //
    // or, for each component
    //
    //   p'_x = a * c - b * s
    //   p'_y = a * s + b * c
    //
    // The largest possible values for `p'_x` and `p'_y` are obtained when both
    // terms contributes in the same direction. Since `a` and `b` are in the
    // range `[-hx, hx]` and `[-hy, hy]`, respectively, we can rewrite the formula as
    //
    //   p'_x = hx * abs(c) + hy * abs(s)
    //   p'_y = hx * abs(s) + hy * abs(c)
    const float abs_s = FABS(s);
    const float abs_c = FABS(c);

    const float hx = dw * 0.5f; // Half-extent size of the destination rectangle
    const float hy = dh * 0.5f;

    const float cx = hx - dax; // Compute the coordinates of the AABB center, relative to the pivot point.
    const float cy = hy - day;
    const float rcx = dx + cx * c - cy * s; // Rotate the center (around the pivot point) and get the coordinates in destination space.
    const float rcy = dy + cx * s + cy * c;

    const float rhx = hx * abs_c + hy * abs_s; // Not really a rotation, more like geometric derivation through trigonometry. :)
    const float rhy = hx * abs_s + hy * abs_c;

    GL_Quad_t drawing_region = {
            .x0 = IFLOORF(rcx - rhx), // Note: we need to floor/ceil to properly round, we can optimize the actual function 'tough.
            .y0 = IFLOORF(rcy - rhy),
            .x1 = ICEILF(rcx + rhx),
            .y1 = ICEILF(rcy + rhy)
        };
#else   /* defined(TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS) */
    const float oax = dx - dax; // Offset of the rotation anchor point in destination space.
    const float oay = dy - day;
    GL_PointF_t p0 = { .x = oax,      .y = oay };
    GL_PointF_t p1 = { .x = oax + dw, .y = oay };
    GL_PointF_t p2 = { .x = oax + dw, .y = oay + dh };
    GL_PointF_t p3 = { .x = oax,      .y = oay + dh };

    _rotate_point_around_pivot_with_offset(&p0, dx, dy, dx, dy, M);
    _rotate_point_around_pivot_with_offset(&p1, dx, dy, dx, dy, M);
    _rotate_point_around_pivot_with_offset(&p2, dx, dy, dx, dy, M);
    _rotate_point_around_pivot_with_offset(&p3, dx, dy, dx, dy, M);

    GL_Quad_t drawing_region = (GL_Quad_t){
            .x0 = IFLOORF(_fminf4(p0.x, p1.x, p2.x, p3.x)),
            .y0 = IFLOORF(_fminf4(p0.y, p1.y, p2.y, p3.y)),
            .x1 = ICEILF(_fmaxf4(p0.x, p1.x, p2.x, p3.x)),
            .y1 = ICEILF(_fmaxf4(p0.y, p1.y, p2.y, p3.y))
        };
#endif  /* defined(TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS) */

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

    const int width = drawing_region.x1 - drawing_region.x0;
    const int height = drawing_region.y1 - drawing_region.y0;
    if ((width <= 0) || (height <= 0)) { // Nothing to draw! Bail out!(can be negative due to clipping region)
        return;
    }

    // The source pivot offset is calculated from the top/left or bottom/right corner,
    // according to the flipping (negative scaling). The source anchor point delta is
    // calculated from the destination values so it takes into account for the
    // flip as well (i.e. we are scanning top/left to bottom/right, or vice versa).
#if defined(TOFU_GRAPHICS_BRANCHLESS_CALCULATIONS)
    const float spx = (scale_x < 0.0f ? sx + sw : sx) + dax / scale_x;
    const float spy = (scale_y < 0.0f ? sy + sh : sy) + day / scale_y;
#else   /* defined(TOFU_GRAPHICS_BRANCHLESS_CALCULATIONS) */
    const float spx = (scale_x < 0.0f ? sx + sw : sx) + sax * FSIGNUM(scale_x);
    const float spy = (scale_y < 0.0f ? sy + sh : sy) + say * FSIGNUM(scale_y);
#endif  /* defined(TOFU_GRAPHICS_BRANCHLESS_CALCULATIONS) */

    // When calculating the source origin, the pivot is still in destination space,
    // as the rotation is applied to the drawing rectangle top/left corner.
#if !defined(TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS)
    GL_PointF_t source_origin = (GL_PointF_t){
            .x = (float)drawing_region.x0 + 0.5f,
            .y = (float)drawing_region.y0 + 0.5f
        };
    GL_PointF_t row_dx = (GL_PointF_t){
            .x = 1.0f,
            .y = 0.0f
        };
    GL_PointF_t row_dy = (GL_PointF_t){
            .x = 0.0f,
            .y = 1.0f
        };

    _rotate_point_around_pivot_with_offset(&source_origin, dx, dy, spx, spy, IMS);
    _rotate_point(&row_dx, IMS);
    _rotate_point(&row_dy, IMS);

    float row_u = source_origin.x;
    float row_v = source_origin.y;

    const float du_dx = row_dx.x;
    const float dv_dx = row_dx.y;
    const float du_dy = row_dy.x;
    const float dv_dy = row_dy.y;
#else   /* !defined(TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS) */
    const float inv_scale_x = 1.0f / scale_x; // Precalculating the reciprocal is cheaper that doing DIVs.
    const float inv_scale_y = 1.0f / scale_y;

    const float m00 =  c * inv_scale_x; // Inverse rotation and scaling combined. The flip sign is included in the `inv_scale_*` value.
    const float m01 =  s * inv_scale_x;
    const float m10 = -s * inv_scale_y;
    const float m11 =  c * inv_scale_y;

    const float px = (float)drawing_region.x0 + 0.5f - dx;
    const float py = (float)drawing_region.y0 + 0.5f - dy;

    float row_u = px * m00 + py * m01 + spx;
    float row_v = px * m10 + py * m11 + spy;

    const float du_dx = m00; // Meaning: delta of (source space) `u` when moving one pixel in (destination space) `x`.
    const float dv_dx = m10;
    const float du_dy = m01;
    const float dv_dy = m11;
#endif  /* !defined(TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS) */

    const GL_Pixel_t *sdata = source->data;
    GL_Pixel_t *ddata = surface->data;

    const size_t swidth = source->width;
    const size_t dwidth = surface->width;

    const size_t dskip = dwidth - width;

    GL_Pixel_t *dptr = ddata + drawing_region.y0 * dwidth + drawing_region.x0;

    const int sminx = area.x; // This is the boundary of the source area, used to check if the backward-mapped pixel is out of bounds.
    const int sminy = area.y;
    const int smaxx = sminx + (int)area.width;
    const int smaxy = sminy + (int)area.height;

#if defined(TOFU_GRAPHICS_NO_IFLOORF)
    const float fsminx = (float)sminx;
    const float fsminy = (float)sminy;
    const float fsmaxx = (float)smaxx;
    const float fsmaxy = (float)smaxy;
#endif  /* defined(TOFU_GRAPHICS_NO_IFLOORF) */

    // We can the destination area and calculate the source pixel with an
    // incremental backward-mapping. This is basically very similar to texture
    // mapping.
    //
    // The source-space row vector `row_dx` and column vector `row_dy`
    // are calculated by applying the inverse transformation to the unit vectors
    // in destination space. The source origin is calculated by applying the
    // inverse transformation to the top/left corner of the drawing region, with
    // an offset to the rotation anchor point in source space (so we can rotate
    // around it).
    //
    // Then we just need to incrementally add the row/column vectors
    // to move across the area.
    for (int i = height; i; --i) {
        float u = row_u;
        float v = row_v;

        for (int j = width; j; --j) {
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
            _pixel(surface, drawing_region.x0 + width - j, drawing_region.y0 + height - i, 15);
#endif  /* defined(TOFU_GRAPHICS_DEBUG_ENABLED) */

#if defined(TOFU_GRAPHICS_NO_IFLOORF)
            // In the source space we can't have negative zero, so we can use
            // simple casts to `int` to get the floor value. We just need to
            // check if the value is in bounds before, to avoid out-of-bounds
            // access due to the truncation toward zero.
            if (u >= fsminx && u < fsmaxx && v >= fsminy && v < fsmaxy) {
                const int x = (int)u;
                const int y = (int)v;
#else   /* defined(TOFU_GRAPHICS_NO_IFLOORF) */
            int x = IFLOORF(u); // Round down, to preserve negative values as such (e.g. `-0.3` is `-1`) and avoid mirror effect.
            int y = IFLOORF(v); // (can't truncate, because negatives would be truncated toward zero)

            // Note: we could rewrite the four conditions with "unsigned checks". This would require to
            //       calculate `(size_t)(x - sminx)`, and compare with `area.width`. We would save a 
            //       comparison, over a difference. This micro-optimization may not be worth it, as
            //       modern branch predictors make it negligible at the cost or less readable code.
            if (x >= sminx && x < smaxx && y >= sminy && y < smaxy) {
#endif  /* defined(TOFU_GRAPHICS_NO_IFLOORF) */
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
                _pixel(surface, drawing_region.x0 + width - j, drawing_region.y0 + height - i, 3);
#endif  /* defined(TOFU_GRAPHICS_DEBUG_ENABLED) */
                const GL_Pixel_t *sptr = sdata + y * swidth + x;

#if defined(_BRANCHLESS_BLIT_EXPERIMENTAL)
                const GL_Pixel_t pixel = *sptr;
                const uint8_t mapped = state_map[pixel];
                const GL_Pixel_t skip = GL_PALETTE_IS_TRANSPARENT(mapped);
                const GL_Pixel_t draw = 1 - skip;
                const GL_Pixel_t mask = (GL_Pixel_t)-draw;
                const GL_Pixel_t sindex = bank_mask | GL_PALETTE_GET_SHIFTING(mapped);
                const GL_Pixel_t dindex = *dptr;
                *dptr = _BLIT_BLEND(dindex, sindex, mask);
#else   /* defined(_BRANCHLESS_BLIT_EXPERIMENTAL) */
                uint8_t mapped = state_map[*sptr];
                if (!GL_PALETTE_IS_TRANSPARENT(mapped)) {
                    *dptr = bank_mask | GL_PALETTE_GET_SHIFTING(mapped);
                }
#endif  /* defined(_BRANCHLESS_BLIT_EXPERIMENTAL) */
            }

            ++dptr;

            u += du_dx; // Move to the next pixel in the row, according to the rotation and scaling.
            v += dv_dx;
        }

        dptr += dskip;

        row_u += du_dy; // Shift the row vertically (scaling is included in the `row_d*` values).
        row_v += dv_dy;
    }
#if defined(TOFU_GRAPHICS_DEBUG_ENABLED)
    _pixel(surface, position.x, position.y, 11); // !!! WARNING !!! Unclipped draw! Might cause a SEGV!
    _pixel(surface, drawing_region.x0    , drawing_region.y0    , 7);
    _pixel(surface, drawing_region.x1 - 1, drawing_region.y0    , 7);
    _pixel(surface, drawing_region.x1 - 1, drawing_region.y1 - 1, 7);
    _pixel(surface, drawing_region.x0    , drawing_region.y1 - 1, 7);
#endif  /* defined(TOFU_GRAPHICS_DEBUG_ENABLED) */
}
