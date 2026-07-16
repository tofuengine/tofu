# Roto-scaler optimization notes

### Recap

The current roto-scaler is built around a conservative destination AABB, a half-pixel aligned pivot, and an incremental inverse mapping from destination pixels back into source pixels. The main performance work so far has been to keep the expensive transformation math outside the per-pixel loop, preserve the sampling invariants, and avoid optimizations that save a small operation while making the code less predictable or less portable.

The most important optimizations are the half-extent AABB computation, the precomputed inverse transformation increments, fixed-point accumulation in the scaler and roto-scaler hot loops, and the sheet-level fast path for zero rotation. When fixed-point math is disabled, the optional `TOFU_GRAPHICS_NO_IFLOORF` float fallback still avoids per-pixel `IFLOORF()` calls by checking source bounds before casting to integer coordinates.

### Initial rationale

The investigation started from a much more direct roto-scaler implementation. That version was non-incremental: each candidate destination pixel was evaluated independently, and the transformation math was effectively recomputed for every pixel. It also used a radius-based idea to build the drawing region. The scaled sprite was enclosed in a circle around the pivot, with the radius derived from the sprite extents through a square-root calculation. The resulting circle was then used to derive a broad candidate area for the rotated sprite.

That approach was simple to reason about, but it was too coarse and too expensive for a hot blitting path. A circle around a rectangle is conservative, but it includes many pixels that can never belong to the rotated sprite, especially for wide or tall sprites. Those extra pixels still had to be tested. The same distance/radius model was also used to classify candidate pixels, which meant the algorithm was doing work that was geometrically related to the enclosing circle rather than directly to the transformed source rectangle.

The key realization was that the roto-scaler does not need a circular approximation. It needs the exact conservative AABB of the rotated destination rectangle, then a reliable way to map each destination pixel in that AABB back into source space. Once the problem is expressed that way, square-root distance tests disappear from the hot path, the AABB can be computed directly from the rectangle geometry, and the per-pixel mapping can be reduced to incremental additions.

### Optimization path

1. Replace the circle/radius candidate model with destination-to-source sampling.

The roto-scaler scans the destination AABB and backward-maps each destination pixel into source space. This avoids holes that would appear with a forward-mapped source-to-destination approach. The transformation is represented as an inverse rotation and inverse scale, including the sign of `scale_x` and `scale_y` so negative scale still behaves as a flip.

The per-pixel loop does not recompute the full transformation from scratch. It starts each row from `row_u` and `row_v`, then advances with `du_dx` and `dv_dx` for each pixel. At the end of a row it advances the row origin with `du_dy` and `dv_dy`. This keeps the inner loop to incremental additions, source bounds checks, one source fetch, the palette-state lookup, the transparency branch, and the optional destination write.

2. Align the rotation pivot to pixel centers.

The destination pivot is computed as `position + 0.5f`. This makes rotation happen around a pixel center instead of an integer grid corner, matching the half-pixel sampling model used elsewhere in the blitters. This avoids systematic half-pixel drift and makes the destination pixel corresponding to the pivot visually stable.

The resulting semantic is that `GL_context_sprite_sr()` treats `position` as the destination pivot/anchor point. Non-rotated APIs such as `GL_context_sprite()` and `GL_context_sprite_s()` keep the simpler top-left destination semantic.

3. Keep scaled dimensions compatible with the non-rotated scaler.

The roto-scaler computes `dw` and `dh` with `ITRUNC(area.width * FABS(scale_x))` and `ITRUNC(area.height * FABS(scale_y))`, matching `GL_context_sprite_s()`. Truncation is intentional: expanding the scaled size by rounding up can make nearest-neighbor sampling reach outside the intended source rectangle and cause edge bleeding.

The sign of the scale is not used to move the pivot. The pivot location is based on the scaled absolute size and the anchor. The scale sign only affects source traversal and therefore represents flipping, not a different anchor placement rule.

4. Replace four-corner AABB rotation with rotated center plus projected half-extents.

The original conservative AABB can be obtained by rotating all four destination rectangle corners and then taking min/max. That is correct, but it performs redundant work every call.

The optimized path computes the destination rectangle center relative to the pivot, rotates only that center, and then computes the rotated AABB half-extents as:

```c
rhx = hx * abs(c) + hy * abs(s);
rhy = hx * abs(s) + hy * abs(c);
```

This is not literally rotating the half-extent vector. It is projecting the two rotated basis vectors onto the destination X and Y axes and taking the maximum possible contribution of each source-side extent. The absolute values are required because either side of the rectangle can contribute to the positive or negative bound depending on the rotation quadrant.

This optimization is valid even when the pivot is not at the center of the source or destination rectangle. The pivot affects where the rotated center lands. The half-extents only describe the size of the rotated rectangle around that rotated center, so they are independent from the pivot location.

5. Keep `floor` and `ceil` for the destination drawing region.

The drawing region must be conservative. Its left/top bounds need floor semantics and its right/bottom bounds need ceil semantics. A plain cast would truncate toward zero and could shrink the AABB, especially when the rotated rectangle crosses negative destination coordinates.

These calls are not a meaningful per-frame hotspot in the optimized AABB path because they happen four times per roto-scaled blit, not per pixel. They can use inline `ifloor()` and `iceil()` helpers in release builds, but removing the rounding semantics would be incorrect.

6. Precompute inverse scale reciprocals and matrix coefficients.

The optimized path computes `inv_scale_x` and `inv_scale_y` once, then derives the inverse transformation coefficients:

```c
m00 =  c * inv_scale_x;
m01 =  s * inv_scale_x;
m10 = -s * inv_scale_y;
m11 =  c * inv_scale_y;
```

This replaces repeated divisions with multiplications and makes the row and column increments explicit. Division is still more expensive and less pipeline-friendly than multiplication on many targets, and computing the reciprocal once is a simple, readable transformation.

7. Use fixed-point accumulators and truncate only after fixed-space bounds checks.

With `TOFU_GRAPHICS_USE_FIXED_MATH`, transformation setup remains in floating point. The source origins and four inverse-mapping increments are converted to 16.16 fixed point once, immediately before iteration. Both pixel loops then advance source coordinates with integer additions. The non-rotated scaler also converts its X origin once before the row loop rather than once per row.

The conversion is deliberately after the AABB-specific setup. As a result, fixed-point sampling works with both the projected-half-extent AABB and the four-corner reference AABB; choosing an AABB algorithm does not change the accumulator type.

The roto-scaler checks the fixed coordinates against fixed versions of the source bounds before extracting integer pixel indices:

```c
if (u >= fsminx && u < fsmaxx && v >= fsminy && v < fsmaxy) {
    const int x = FPMATH_ITRUNC(u);
    const int y = FPMATH_ITRUNC(v);
    ...
}
```

The previous unconditional `IFLOORF()` was necessary because inverse mapping produces negative coordinates around the rotated sprite and C truncation toward zero is not floor there: `-0.3f` becomes `0`, not `-1`. The new order establishes a stronger invariant. A valid source rectangle is non-negative, and every fixed-point coordinate that is negative or otherwise out of range is rejected. Every coordinate reaching `FPMATH_ITRUNC()` is therefore non-negative. For a non-negative fixed value, truncation toward zero and floor select the same integer, so no `FPMATH_IFLOOR()` is needed in the hot loop.

Rounding during `FPMATH_FROM_FLOAT()` does not supply this proof. It rounds to the nearest representable fixed-point quantum, not to the nearest source pixel. The equivalence with floor comes only from the bounds-before-truncation invariant. Quantizing origins and increments can still change a sample extremely close to an integer boundary and can accumulate drift over long rows; that is the explicit precision/performance tradeoff of the fixed path.

When fixed-point math is disabled, `TOFU_GRAPHICS_NO_IFLOORF` applies the same ordering in float space and uses casts for accepted coordinates. Without either optimization, the float reference path retains `IFLOORF()` followed by integer bounds checks.

8. Add sheet-level zero-rotation fast paths.

The lower-level GL blitters intentionally keep simple semantics: `GL_context_sprite()` and `GL_context_sprite_s()` use top-left destination coordinates, while `GL_context_sprite_sr()` uses pivot destination coordinates. The sheet layer is the right place to adapt between these semantics.

When `TOFU_GRAPHICS_BLIT_FAST_PATH` is enabled, `GL_sheet_blit_sr()` detects zero rotation and converts the pivot/anchor destination into the top-left coordinate required by the non-rotated blitters. It then calls `GL_context_sprite()` for exact unscaled positive scale, or `GL_context_sprite_s()` for scaled or flipped cases.

The raw blit fast path checks `scale_x == 1.0f && scale_y == 1.0f`, not `FABS(scale) == 1.0f`. A scale of `-1.0f` still means flipped rendering and cannot be replaced by `GL_context_sprite()`.

9. Keep palette-bank handling separate from palette-state mapping.

The palette bank remains separate from `palette_state.map`. Folding the bank into the map could remove an OR in the inner loop, but it would make bank switching require recalculating palette state. The current layout preserves cheap bank changes, which is more valuable for the engine design.

10. Keep the simple transparency branch in the hot path.

The branchless blit experiment remains disabled. The ordinary branch is readable, avoids extra mask/blend operations, and can be predicted well when sprite transparency has spatial coherence. The current branch also increments the destination pointer outside the branch, reducing duplicated pointer update logic without changing behavior.

### Considered alternatives and deferred work

Precomputing source row pointers with a LUT was rejected for the general roto-scaler. Replacing `sdata + y * swidth + x` with `row_lut[y] + x` trades one integer multiply for an additional dependent memory load. In this loop memory bandwidth, cache pressure, and dependency chains are usually more important than one integer multiply. A row-pointer cache for repeated `y` values might help in special cases, but it adds a branch in the inner loop and is not generally reliable under rotation.

Replacing the `GL_context_sprite_s()` float accumulators was originally deferred, but the shared fixed-point implementation now makes the change small enough to justify. Its source origins still use the existing half-pixel formula and negative-scale margin selection; only the recurring `u` and `v` representation changes. The fixed X origin is computed once outside the row loop, and integer extraction replaces the per-pixel float conversion.

Precomputing X indices for `GL_context_sprite_s()` could be useful only if profiling proves the scaled non-rotated path is hot. It would compute the source X coordinate once per destination column and reuse it for every row. The tradeoff is extra storage management, and the project should avoid adding VLAs or per-call heap allocation in this path just for a speculative win.

Replacing the four bounds comparisons with unsigned range checks was considered. It would reduce the source bounds test to expressions such as `(size_t)(x - sminx) < area.width`, but the readability cost is high and the practical gain is uncertain. With the `TOFU_GRAPHICS_NO_IFLOORF` path the current check is already expressed naturally in float space before conversion.

Using only two original corners for AABB rotation was rejected as a general solution. Depending on the rotation quadrant, any of the four corners can determine an X or Y min/max. The half-extent projection formula gives the desired reduction without relying on quadrant-specific corner choices.

Replacing `rotation % SINCOS_PERIOD == 0` with a manual period-specific trick was not considered worth it. The compiler can usually optimize constant-period arithmetic when profitable, and this check is outside the pixel loop.

Trying to remove `IFLOORF()` and `ICEILF()` from the AABB computation was rejected. The conservative drawing region depends on floor/ceil semantics. The cost is per blit call, not per pixel, so the correct optimization is to make those helpers cheap in release builds, not to change the rounding rule.

### Current compile-time controls

`TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS` enables the half-extent AABB path. This is the preferred path because it avoids rotating four corners while preserving a conservative bounding box.

`TOFU_GRAPHICS_USE_FIXED_MATH` converts the scaler and roto-scaler source origins and increments to the project's fixed-point representation before their pixel loops. The setup geometry stays in floating point. The default 16.16 format has a whole-number range of `[-32768, 32767]`; source bounds, origins, increments, and all accumulated values must remain representable. Changing `FIX32_FRACTIONAL_BITS` changes both that range and the sampling quantization.

`TOFU_GRAPHICS_NO_IFLOORF` enables the float-bounds-then-cast fallback in `GL_context_sprite_sr()` when fixed-point math is disabled. It has no effect on the fixed hot loop, which always checks fixed-space bounds before truncation.

`TOFU_GRAPHICS_BLIT_FAST_PATH` enables sheet-level dispatch to cheaper blitters when rotation or scaling is unnecessary. This belongs in the sheet layer because that layer can adapt pivot/anchor semantics to top-left blit semantics.

`TOFU_GRAPHICS_BRANCHLESS_CALCULATIONS` remains disabled. Its purpose is to reduce branches in some setup calculations, but the code comments note potential accuracy issues near zero scale. The current implementation favors the explicit path.

Release builds define `IMATH_FAST_OPERATIONS` and `IMATH_INLINE_FAST_OPERATIONS` from the Makefile. That makes `IFLOORF()` and `ICEILF()` use the project's inline `ifloor()` and `iceil()` helpers instead of forcing out-of-line calls. This mostly matters for code paths that still need floor/ceil semantics, such as AABB construction.

### Correctness constraints kept during optimization

The destination AABB must be conservative. Any optimization that can shrink the box risks skipping pixels along rotated edges.

The inner source lookup must reject negative source coordinates before integer truncation. The fixed path always checks fixed-space bounds first; the `TOFU_GRAPHICS_NO_IFLOORF` float fallback does the equivalent check in float space. The reference float path instead preserves floor semantics before its integer bounds check.

Fixed-point conversion is a quantization step, not a mathematically exact replacement for float accumulation. The default 16 fractional bits limit the initial error of each converted value to half a fixed quantum, but increment error accumulates across a row or column. Pixel comparisons should allow differences only at sampling boundaries, and profiling must confirm that this tradeoff is worthwhile on each target.

Every fixed source bound, origin, increment, and accumulated coordinate must fit the configured `fix32_t` representation. The current path assumes normal engine surface sizes within the default 16.16 range; widening the supported coordinate range requires reducing fractional bits, changing representation, or adding a float fallback.

The scaled destination size must match the non-rotated scaler. `GL_context_sprite_sr()` and `GL_context_sprite_s()` both use truncated absolute scaled size, avoiding source edge bleeding and keeping zero-rotation behavior as close as possible.

The pivot placement must not use the scale sign. Negative scale is a flip of source traversal. It should not reinterpret the anchor or move the pivot to a different destination location.

The raw blit fast path must not treat `-1.0f` scale as unscaled. A negative unit scale still needs the scaled blitter because it flips the sampled image.

### Remaining profiling targets

The current likely hotspots are the source bounds check, source pixel fetch, palette-state lookup, transparency branch, and destination write. The source address multiply is unlikely to dominate compared with memory access and branch behavior.

If future profiling shows `GL_context_sprite_sr()` still dominates frame time, the next useful checks are generated assembly for the fixed inner loop, branch behavior for transparency-heavy sprites, and fixed-versus-float timing on each target. The engine is portable, so an integer loop should not be assumed faster merely because it avoids floating-point conversion.

The fixed and float paths should also be rendered side by side across rotations, fractional and negative scales, clipping, pivots, and odd/even sprite sizes. Differences should be confined to coordinates that quantize across a nearest-neighbor sampling boundary; larger or structured differences indicate an origin, sign, bounds, or accumulator bug.

For `GL_context_sprite_s()`, the best future optimization would be a measured specialized fast path for common scale patterns. Precomputed X indices remain an option only if profiling justifies their storage and setup costs.
