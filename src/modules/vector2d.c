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

//#define _VERBOSE_DEBUG

static int vector2d_new_v_1o(lua_State *L);
static int vector2d_gc_1o_0(lua_State *L);
static int vector2d_eq_1o_1b(lua_State *L);
static int vector2d_tostring_1o_1s(lua_State *L);
static int vector2d_x_v_v(lua_State *L);
static int vector2d_y_v_v(lua_State *L);
static int vector2d_is_zero_1o_1b(lua_State *L);
static int vector2d_is_almost_zero_1o_1b(lua_State *L);
static int vector2d_is_equal_2oo_1b(lua_State *L);
static int vector2d_is_almost_equal_2oo_1b(lua_State *L);
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
            { "__eq", vector2d_eq_1o_1b },
            { "__tostring", vector2d_tostring_1o_1s },
            // -- getters/setters --
            { "x", vector2d_x_v_v },
            { "y", vector2d_y_v_v },
            // -- accessors --
            { "is_zero", vector2d_is_zero_1o_1b },
            { "is_almost_zero", vector2d_is_almost_zero_1o_1b },
            { "is_equal", vector2d_is_equal_2oo_1b },
            { "is_almost_equal", vector2d_is_almost_equal_2oo_1b },
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

#if defined(_VERBOSE_DEBUG)
    LOG_D("vector %p allocated w/ data `<%.3f, %.3f>`", self, x, y);
#endif  /* defined(_VERBOSE_DEBUG) */

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

#if defined(_VERBOSE_DEBUG)
    LOG_D("vector %p allocated w/ data `<%.3f, %.3f>`", self, x, y);
#endif  /* defined(_VERBOSE_DEBUG) */

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

#if defined(_VERBOSE_DEBUG)
    LOG_D("vector %p allocated w/ data `<%.3f, %.3f>`", self, x, y);
#endif  /* defined(_VERBOSE_DEBUG) */

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

#if defined(_VERBOSE_DEBUG)
    LOG_D("vector %p finalized", self);
#endif  /* defined(_VERBOSE_DEBUG) */

    return 0;
}

static int vector2d_eq_1o_1b(lua_State *L)
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

    lua_pushfstring(L, "<%.5f, %.5f>", self->x, self->y);

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

    const bool is_almost_zero = fabsf(self->x) <= FLT_EPSILON && fabsf(self->y) <= FLT_EPSILON;

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

    const bool is_almost_equal = fabsf(self->x - other->x) <= FLT_EPSILON && fabsf(self->y - other->y) <= FLT_EPSILON;

    lua_pushboolean(L, is_almost_equal);

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

    lua_pushnumber(L, sqrtf(x * x + y * y));

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
    const float magnitude = sqrtf(x * x + y * y);

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

    // x' = x *  c + y * -s
    // y' = x *  s + y *  c
    self->x = self->x * c - self->y * s;
    self->y = self->x * s + self->y * c;

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

    // x' = x *  c + y *  s
    // y' = x * -s + y *  c
    self->x = self->x * c + self->y * s;
    self->y = self->y * c - self->x * s;

    return 0;
}

static int vector2d_rotate90_ccw_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    self->x = -self->y;
    self->y = self->x;

    return 0;
}

static int vector2d_rotate90_cw_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Vector2D_Object_t *self = (Vector2D_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_VECTOR2D);

    self->x = self->y;
    self->y = -self->x;

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

    lua_pushnumber(L, sqrtf(dx * dx + dy * dy));

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

    float magnitude = 0.0f;

    const float magnitude_squared = x * x + y * y;
    if (magnitude_squared > 0.0f) {
        magnitude = sqrtf(magnitude_squared);
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

    const float magnitude_squared = x * x + y * y;

    const float factor_squared = l * l / magnitude_squared;

    if (factor_squared < 1.0f) {
        const float factor = sqrtf(factor_squared);

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

    const float angle_self = atan2f(self->y, self->x);
    const float angle_other = atan2f(other->y, other->x);

    lua_pushnumber(L, angle_other - angle_self);

    return 1;
}
