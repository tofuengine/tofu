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

#include "vector2d.h"

#include "internal/udt.h"

#include <core/config.h>
#define _LOG_TAG "vector2d"
#include <libs/log.h>

#include <float.h>
#include <math.h>
#include <stdbool.h>

#define _ABSOLUTE_TOLERANCE 1e-6f
#define _RELATIVE_TOLERANCE 1e-5f
#define _PARALLEL_TOLERANCE 1e-4f

static int vector2d_new_v_1o(lua_State *L);
static int vector2d_gc_1o_0(lua_State *L);
static int vector2d_eq_2oo_1b(lua_State *L);
static int vector2d_tostring_1o_1s(lua_State *L);
static int vector2d_x_v_v(lua_State *L);
static int vector2d_y_v_v(lua_State *L);
static int vector2d_is_zero_1o_1b(lua_State *L);
static int vector2d_is_almost_zero_1o_1b(lua_State *L);
static int vector2d_is_equal_2oo_1b(lua_State *L);
static int vector2d_is_almost_equal_2oo_1b(lua_State *L);
static int vector2d_is_aligned_3ooB_1b(lua_State *L);
static int vector2d_unpack_1o_2nn(lua_State *L);
static int vector2d_magnitude_squared_1o_1n(lua_State *L);
static int vector2d_magnitude_1o_1n(lua_State *L);
static int vector2d_angle_1o_1n(lua_State *L);
static int vector2d_polar_1o_2nn(lua_State *L);
static int vector2d_assign_v_0(lua_State *L);
static int vector2d_cast_5onnNN_0(lua_State *L);
static int vector2d_negate_1o_0(lua_State *L);
static int vector2d_add_v_0(lua_State *L);
static int vector2d_sub_v_0(lua_State *L);
static int vector2d_mul_v_0(lua_State *L);
static int vector2d_div_v_0(lua_State *L);
static int vector2d_smul_2on_0(lua_State *L);
static int vector2d_sdiv_2on_0(lua_State *L);
static int vector2d_fma_3oon_0(lua_State *L);
static int vector2d_lerp_3oon_0(lua_State *L);
static int vector2d_rotate_ccw_2on_0(lua_State *L);
static int vector2d_rotate_cw_2on_0(lua_State *L);
static int vector2d_rotate90_ccw_1o_0(lua_State *L);
static int vector2d_rotate90_cw_1o_0(lua_State *L);
static int vector2d_normalize_2oN_1n(lua_State *L);
static int vector2d_trim_2on_0(lua_State *L);
static int vector2d_dot_2oo_1n(lua_State *L);
static int vector2d_perp_dot_2oo_1n(lua_State *L);
static int vector2d_distance_from_squared_2oo_1n(lua_State *L);
static int vector2d_distance_from_2oo_1n(lua_State *L);
static int vector2d_angle_to_2oo_1n(lua_State *L);
static int vector2d_angle_between_2oo_1n(lua_State *L);

int vector2d_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", vector2d_new_v_1o },
            { "__gc", vector2d_gc_1o_0 },
            // -- metamethods --
            { "__eq", vector2d_eq_2oo_1b },
            { "__tostring", vector2d_tostring_1o_1s },
            // -- getters/setters --
            { "x", vector2d_x_v_v },
            { "y", vector2d_y_v_v },
            // -- accessors --
            { "is_zero", vector2d_is_zero_1o_1b },
            { "is_almost_zero", vector2d_is_almost_zero_1o_1b },
            { "is_equal", vector2d_is_equal_2oo_1b },
            { "is_almost_equal", vector2d_is_almost_equal_2oo_1b },
            { "is_aligned", vector2d_is_aligned_3ooB_1b },
            { "unpack", vector2d_unpack_1o_2nn },
            { "magnitude_squared", vector2d_magnitude_squared_1o_1n },
            { "magnitude", vector2d_magnitude_1o_1n },
            { "angle", vector2d_angle_1o_1n },
            { "polar", vector2d_polar_1o_2nn },
            // -- mutators --
            { "assign", vector2d_assign_v_0 },
            { "copy", vector2d_assign_v_0 }, // alias for `assign`
            { "cast", vector2d_cast_5onnNN_0 },
            { "negate", vector2d_negate_1o_0 },
            { "add", vector2d_add_v_0 },
            { "sub", vector2d_sub_v_0 },
            { "mul", vector2d_mul_v_0 },
            { "div", vector2d_div_v_0 },
            { "smul", vector2d_smul_2on_0 },
            { "sdiv", vector2d_sdiv_2on_0 },
            { "fma", vector2d_fma_3oon_0 },
            { "lerp", vector2d_lerp_3oon_0 },
            { "rotate_ccw", vector2d_rotate_ccw_2on_0 },
            { "rotate_cw", vector2d_rotate_cw_2on_0 },
            { "rotate90_ccw", vector2d_rotate90_ccw_1o_0 },
            { "rotate90_cw", vector2d_rotate90_cw_1o_0 },
            { "rotate", vector2d_rotate_ccw_2on_0 }, // alias for `rotate_ccw`
            { "rotate90", vector2d_rotate90_ccw_1o_0 }, // alias for `rotate90_ccw`
            { "rotate180", vector2d_negate_1o_0 }, // alias for `negate`
            { "perp", vector2d_rotate90_ccw_1o_0 }, // alias for `rotate90_ccw`
            { "normalize", vector2d_normalize_2oN_1n },
            { "trim", vector2d_trim_2on_0 },
            // -- operations --
            { "dot", vector2d_dot_2oo_1n },
            { "perp_dot", vector2d_perp_dot_2oo_1n },
            { "distance_from_squared", vector2d_distance_from_squared_2oo_1n },
            { "distance_from", vector2d_distance_from_2oo_1n },
            { "angle_to", vector2d_angle_to_2oo_1n },
            { "angle_between", vector2d_angle_between_2oo_1n },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { "ABSOLUTE_TOLERANCE", LUA_CT_NUMBER, { .n = _ABSOLUTE_TOLERANCE } },
            { "RELATIVE_TOLERANCE", LUA_CT_NUMBER, { .n = _RELATIVE_TOLERANCE } },
            { "PARALLEL_TOLERANCE", LUA_CT_NUMBER, { .n = _PARALLEL_TOLERANCE } },
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static int vector2d_new_0_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Vector2D_Object_t *self = (Vector2D_Object_t *)udt_newobject(L, sizeof(Vector2D_Object_t), &(Vector2D_Object_t){
            .x = 0.0f,
            .y = 0.0f
        }, OBJECT_TYPE_VECTOR2D);
    LUAX_UNUSED(self);

#if defined(TOFU_CORE_VERBOSE_DEBUG)
    LOG_D("vector %p allocated w/ data `<%.3f, %.3f>`", self, self->x, self->y);
#endif  /* defined(TOFU_CORE_VERBOSE_DEBUG) */

    return 1;
}

static int vector2d_new_1o_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    Vector2D_Object_t *self = (Vector2D_Object_t *)udt_newobject(L, sizeof(Vector2D_Object_t), &(Vector2D_Object_t){
            .x = other->x,
            .y = other->y
        }, OBJECT_TYPE_VECTOR2D);
    LUAX_UNUSED(self);

#if defined(TOFU_CORE_VERBOSE_DEBUG)
    LOG_D("vector %p allocated w/ data `<%.3f, %.3f>`", self, self->x, self->y);
#endif  /* defined(TOFU_CORE_VERBOSE_DEBUG) */

    return 1;
}

static int vector2d_new_2nn_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = LUAX_NUMBER(L, 1);
    float y = LUAX_NUMBER(L, 2);

    Vector2D_Object_t *self = (Vector2D_Object_t *)udt_newobject(L, sizeof(Vector2D_Object_t), &(Vector2D_Object_t){
            .x = x,
            .y = y
        }, OBJECT_TYPE_VECTOR2D);
    LUAX_UNUSED(self);

#if defined(TOFU_CORE_VERBOSE_DEBUG)
    LOG_D("vector %p allocated w/ data `<%.3f, %.3f>`", self, self->x, self->y);
#endif  /* defined(TOFU_CORE_VERBOSE_DEBUG) */

    return 1;
}

static int vector2d_new_v_1o(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(vector2d_new_0_1o, 0)
        LUAX_OVERLOAD_BY_ARITY(vector2d_new_1o_1o, 1)
        LUAX_OVERLOAD_BY_ARITY(vector2d_new_2nn_1o, 2)
    LUAX_OVERLOAD_END
}

static int vector2d_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    LUAX_UNUSED(self);

#if defined(TOFU_CORE_VERBOSE_DEBUG)
    LOG_D("vector %p finalized", self);
#endif  /* defined(TOFU_CORE_VERBOSE_DEBUG) */

    return 0;
}

static int vector2d_eq_2oo_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    const bool is_equal = self == other
        || (self->x == other->x && self->y == other->y);

    lua_pushboolean(L, is_equal);

    return 1;
}

static int vector2d_tostring_1o_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    lua_pushfstring(L, "<%f, %f>", self->x, self->y);

    return 1;
}

static int vector2d_x_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    lua_pushnumber(L, self->x);

    return 1;
}

static int vector2d_x_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float x = LUAX_NUMBER(L, 2);

    self->x = x;

    return 0;
}

static int vector2d_x_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(vector2d_x_1o_1n, 1)
        LUAX_OVERLOAD_BY_ARITY(vector2d_x_2on_0, 2)
    LUAX_OVERLOAD_END
}

static int vector2d_y_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    lua_pushnumber(L, self->y);

    return 1;
}

static int vector2d_y_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float y = LUAX_NUMBER(L, 2);

    self->y = y;

    return 0;
}

static int vector2d_y_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(vector2d_y_1o_1n, 1)
        LUAX_OVERLOAD_BY_ARITY(vector2d_y_2on_0, 2)
    LUAX_OVERLOAD_END
}

static int vector2d_is_zero_1o_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    const bool is_zero = self->x == 0.0f && self->y == 0.0f;

    lua_pushboolean(L, is_zero);

    return 1;
}

static int vector2d_is_almost_zero_1o_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    const float x = self->x;
    const float y = self->y;

#if defined(TOFU_CORE_USE_HYPOTF)
    const float magnitude = hypotf(x, y);
#else   /* defined(TOFU_CORE_USE_HYPOTF) */
    const float magnitude = sqrtf(x * x + y * y);
#endif  /* defined(TOFU_CORE_USE_HYPOTF) */
    const bool is_almost_zero = fabsf(magnitude) <= _ABSOLUTE_TOLERANCE;

    lua_pushboolean(L, is_almost_zero);

    return 1;
}

static int vector2d_is_equal_2oo_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    const bool is_equal = self->x == other->x && self->y == other->y;

    lua_pushboolean(L, is_equal);

    return 1;
}

static int vector2d_is_almost_equal_2oo_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    const float sx = self->x;
    const float sy = self->y;
    const float ox = other->x;
    const float oy = other->y;

    bool is_almost_equal;
    if (sx == ox && sy == oy) {
        is_almost_equal = true;
    } else
    if (!isfinite(sx) || !isfinite(sy) || !isfinite(ox) || !isfinite(oy)) {
        is_almost_equal = false;
    } else {
#if defined(TOFU_CORE_USE_HYPOTF)
        const float difference = hypotf(sx - ox, sy - oy);
        const float magnitude = fmaxf(hypotf(sx, sy), hypotf(ox, oy));
#else   /* defined(TOFU_CORE_USE_HYPOTF) */
        const float difference = sqrtf((sx - ox) * (sx - ox) + (sy - oy) * (sy - oy));
        const float magnitude = fmaxf(sqrtf(sx * sx + sy * sy), sqrtf(ox * ox + oy * oy));
#endif  /* defined(TOFU_CORE_USE_HYPOTF) */
        const float tolerance = fmaxf(_ABSOLUTE_TOLERANCE, _RELATIVE_TOLERANCE * magnitude);

        is_almost_equal = difference <= tolerance;
    }

    lua_pushboolean(L, is_almost_equal);

    return 1;
}

static int vector2d_is_aligned_3ooB_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);
    const bool allow_opposite = LUAX_OPTIONAL_BOOLEAN(L, 3, false);

    const float sx = self->x;
    const float sy = self->y;
    const float ox = other->x;
    const float oy = other->y;

#if defined(TOFU_CORE_USE_HYPOTF)
    const float magnitude_self = hypotf(sx, sy);
    const float magnitude_other = hypotf(ox, oy);
#else   /* defined(TOFU_CORE_USE_HYPOTF) */
    const float magnitude_self = sqrtf(sx * sx + sy * sy);
    const float magnitude_other = sqrtf(ox * ox + oy * oy);
#endif  /* defined(TOFU_CORE_USE_HYPOTF) */

    bool is_aligned;
    if (!isfinite(magnitude_self) || !isfinite(magnitude_other)) {
        is_aligned = false;
    } else
    if (magnitude_self <= _ABSOLUTE_TOLERANCE || magnitude_other <= _ABSOLUTE_TOLERANCE) {
        is_aligned = false;
    } else {
        const float ssx = sx / magnitude_self; // Scale the vectors to unit length to avoid overflow/underflow issues.
        const float ssy = sy / magnitude_self;
        const float sox = ox / magnitude_other;
        const float soy = oy / magnitude_other;

        const float dot = ssx * sox + ssy * soy;
        const float cross = ssx * soy - ssy * sox;

        is_aligned = (allow_opposite || dot > 0.0f)
            && fabsf(cross) <= _PARALLEL_TOLERANCE;
    }

    lua_pushboolean(L, is_aligned);

    return 1;
}

static int vector2d_unpack_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    lua_pushnumber(L, self->x);
    lua_pushnumber(L, self->y);

    return 2;
}

static int vector2d_magnitude_squared_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    const float x = self->x;
    const float y = self->y;

    lua_pushnumber(L, x * x + y * y);

    return 1;
}

static int vector2d_magnitude_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    const float x = self->x;
    const float y = self->y;

#if defined(TOFU_CORE_USE_HYPOTF)
    const float magnitude = hypotf(x, y);
#else   /* defined(TOFU_CORE_USE_HYPOTF) */
    const float magnitude = sqrtf(x * x + y * y);
#endif  /* defined(TOFU_CORE_USE_HYPOTF) */

    lua_pushnumber(L, magnitude);

    return 1;
}

static int vector2d_angle_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    const float x = self->x;
    const float y = self->y;

    lua_pushnumber(L, atan2f(y, x));

    return 1;
}

static int vector2d_polar_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    const float x = self->x;
    const float y = self->y;

    const float angle = atan2f(y, x);
#if defined(TOFU_CORE_USE_HYPOTF)
    const float magnitude = hypotf(x, y);
#else   /* defined(TOFU_CORE_USE_HYPOTF) */
    const float magnitude = sqrtf(x * x + y * y);
#endif  /* defined(TOFU_CORE_USE_HYPOTF) */

    lua_pushnumber(L, angle);
    lua_pushnumber(L, magnitude);

    return 2;
}

static int vector2d_assign_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    self->x = other->x;
    self->y = other->y;

    return 0;
}

static int vector2d_assign_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float x = LUAX_NUMBER(L, 2);
    const float y = LUAX_NUMBER(L, 3);

    self->x = x;
    self->y = y;

    return 0;
}

static int vector2d_assign_v_0(lua_State *L) {
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(vector2d_assign_2oo_0, 2)
        LUAX_OVERLOAD_BY_ARITY(vector2d_assign_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static int vector2d_cast_5onnNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float angle = LUAX_NUMBER(L, 2);
    const float magnitude = LUAX_NUMBER(L, 3);
    const float ox = LUAX_OPTIONAL_NUMBER(L, 4, 0.0f);
    const float oy = LUAX_OPTIONAL_NUMBER(L, 5, 0.0f);

    const float x = cosf(angle) * magnitude + ox;
    const float y = sinf(angle) * magnitude + oy;

    self->x = x;
    self->y = y;

    return 0;
}

static int vector2d_negate_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    self->x = -self->x;
    self->y = -self->y;

    return 0;
}

static int vector2d_add_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    self->x += other->x;
    self->y += other->y;

    return 0;
}

static int vector2d_add_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float x = LUAX_NUMBER(L, 2);
    const float y = LUAX_NUMBER(L, 3);

    self->x += x;
    self->y += y;

    return 0;
}

static int vector2d_add_v_0(lua_State *L) {
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(vector2d_add_2oo_0, 2)
        LUAX_OVERLOAD_BY_ARITY(vector2d_add_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static int vector2d_sub_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    self->x -= other->x;
    self->y -= other->y;

    return 0;
}

static int vector2d_sub_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float x = LUAX_NUMBER(L, 2);
    const float y = LUAX_NUMBER(L, 3);

    self->x -= x;
    self->y -= y;

    return 0;
}

static int vector2d_sub_v_0(lua_State *L) {
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(vector2d_sub_2oo_0, 2)
        LUAX_OVERLOAD_BY_ARITY(vector2d_sub_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static int vector2d_mul_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    self->x *= other->x;
    self->y *= other->y;

    return 0;
}

static int vector2d_mul_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float x = LUAX_NUMBER(L, 2);
    const float y = LUAX_NUMBER(L, 3);

    self->x *= x;
    self->y *= y;

    return 0;
}

static int vector2d_mul_v_0(lua_State *L) {
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(vector2d_mul_2oo_0, 2)
        LUAX_OVERLOAD_BY_ARITY(vector2d_mul_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static int vector2d_div_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    self->x /= other->x;
    self->y /= other->y;

    return 0;
}

static int vector2d_div_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float x = LUAX_NUMBER(L, 2);
    const float y = LUAX_NUMBER(L, 3);

    self->x /= x;
    self->y /= y;

    return 0;
}

static int vector2d_div_v_0(lua_State *L) {
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(vector2d_div_2oo_0, 2)
        LUAX_OVERLOAD_BY_ARITY(vector2d_div_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static int vector2d_smul_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float factor = LUAX_NUMBER(L, 2);

    self->x *= factor;
    self->y *= factor;

    return 0;
}

static int vector2d_sdiv_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float factor = LUAX_NUMBER(L, 2);

    self->x /= factor;
    self->y /= factor;

    return 0;
}

static int vector2d_fma_3oon_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);
    const float t = LUAX_NUMBER(L, 3);

    self->x += other->x * t;
    self->y += other->y * t;

    return 0;
}

static int vector2d_lerp_3oon_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);
    const float t = LUAX_NUMBER(L, 3);

    self->x = self->x * (1.0f - t) + other->x * t;
    self->y = self->y * (1.0f - t) + other->y * t;

    return 0;
}

// The 2D counter-clockwise rotation matrix is the following:
//
//   |  cos(a)  -sin(a) | | x |   | x' |
//   |                  | |   | = |    |
//   |  sin(a)   cos(a) | | y |   | y' |
static int vector2d_rotate_ccw_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float angle = LUAX_NUMBER(L, 2);

    const float c = cosf(angle);
    const float s = sinf(angle);

    const float x = self->x;
    const float y = self->y;

    // x' = x *  c + y * -s
    // y' = x *  s + y *  c
    self->x = x * c - y * s;
    self->y = x * s + y * c;

    return 0;
}

// The 2D clockwise rotation matrix is the following:
//
//   |  cos(a)   sin(a) | | x |   | x' |
//   |                  | |   | = |    |
//   | -sin(a)   cos(a) | | y |   | y' |
static int vector2d_rotate_cw_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float angle = LUAX_NUMBER(L, 2);

    const float c = cosf(angle);
    const float s = sinf(angle);

    const float x = self->x;
    const float y = self->y;

    // x' = x *  c + y *  s
    // y' = x * -s + y *  c
    self->x = x * c + y * s;
    self->y = y * c - x * s;

    return 0;
}

static int vector2d_rotate90_ccw_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    const float x = self->x;
    const float y = self->y;

    self->x = -y;
    self->y = x;

    return 0;
}

static int vector2d_rotate90_cw_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    const float x = self->x;
    const float y = self->y;

    self->x = y;
    self->y = -x;

    return 0;
}

static int vector2d_dot_2oo_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    const float dot = self->x * other->x + self->y * other->y;

    lua_pushnumber(L, dot);

    return 1;
}

/*
Area of the parallelogram described by the vector, i.e. the DETERMINAND of
the matrix with the vectors as columns (or rows).

It is also (if scaled) the sine of the angle between the vectors. That means
that if NEGATIVE the second vector is CLOCKWISE from the first one, if
POSITIVE the second vector is COUNTER-CLOCKWISE from the first one.

It is also called "perp-dot", that is the dot product of the perpendicular
vector with another vector (i.e. `a:perp():dot(b)`)

NOTE: when on a 2D display, since the `y` component inverts it sign, also
      the rule inverts! That is if NEGATIVE then is COUNTER-CLOCKWISE.

https://en.wikipedia.org/wiki/Exterior_algebra
http://geomalgorithms.com/vector_products.html#2D-Perp-Product
*/
static int vector2d_perp_dot_2oo_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    const float perp_dot = self->x * other->y - self->y * other->x;

    lua_pushnumber(L, perp_dot);

    return 1;
}

static int vector2d_distance_from_squared_2oo_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    const float dx = self->x - other->x;
    const float dy = self->y - other->y;

    lua_pushnumber(L, dx * dx + dy * dy);

    return 1;
}

static int vector2d_distance_from_2oo_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    const float dx = self->x - other->x;
    const float dy = self->y - other->y;

#if defined(TOFU_CORE_USE_HYPOTF)
    const float distance = hypotf(dx, dy);
#else   /* defined(TOFU_CORE_USE_HYPOTF) */
    const float distance = sqrtf(dx * dx + dy * dy);
#endif  /* defined(TOFU_CORE_USE_HYPOTF) */

    lua_pushnumber(L, distance);

    return 1;
}

static int vector2d_normalize_2oN_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float l = LUAX_OPTIONAL_NUMBER(L, 2, 1);

    const float x = self->x;
    const float y = self->y;

#if defined(TOFU_CORE_USE_HYPOTF)
    const float magnitude = hypotf(x, y);
#else   /* defined(TOFU_CORE_USE_HYPOTF) */
    const float magnitude = sqrtf(x * x + y * y);
#endif  /* defined(TOFU_CORE_USE_HYPOTF) */
    if (magnitude > 0.0f) {
        const float factor = l / magnitude;

        self->x *= factor;
        self->y *= factor;
    }

    lua_pushnumber(L, magnitude);

    return 1;
}

static int vector2d_trim_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const float l = LUAX_NUMBER(L, 2);

    const float x = self->x;
    const float y = self->y;

#if defined(TOFU_CORE_USE_HYPOTF)
    const float magnitude = hypotf(x, y);
#else   /* defined(TOFU_CORE_USE_HYPOTF) */
    const float magnitude = sqrtf(x * x + y * y);
#endif  /* defined(TOFU_CORE_USE_HYPOTF) */
    if (magnitude > l) { // If the vector is longer than `l`, trim it down to `l`
        const float factor = l / magnitude;

        self->x *= factor;
        self->y *= factor;
    }

    return 0;
}

static int vector2d_angle_to_2oo_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    const float angle = atan2f(other->y - self->y, other->x - self->x);

    lua_pushnumber(L, angle);

    return 1;
}

static int vector2d_angle_between_2oo_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Vector2D_Object_t *self = (const Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);
    const Vector2D_Object_t *other = (const Vector2D_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_VECTOR2D);

    // The previous implementation was the following
    //
    //   const float angle_self = atan2f(self->y, self->x);
    //   const float angle_other = atan2f(other->y, other->x);
    //   const float angle = angle_other - angle_self;
    //
    // which was resulting a `±2π` angle difference.
    //
    // We can obtain a shorter angle difference by using the `atan2f` function
    // with the cross and dot products of the two vectors.

    const float sx = self->x;
    const float sy = self->y;
    const float ox = other->x;
    const float oy = other->y;

    const float dot = sx * ox + sy * oy;
    const float cross = sx * oy - sy * ox;

    const float angle = atan2f(cross, dot);

    lua_pushnumber(L, angle);

    return 1;
}
