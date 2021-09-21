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

#include "math.h"

#include <config.h>
#include <libs/easing.h>
#include <libs/log.h>
#include <libs/luax.h>
#include <libs/sincos.h>
#include <libs/wave.h>
#include <libs/imath.h>
#include <libs/fmath.h>

#include <perlin-noise/noise1234.h>
#include <perlin-noise/simplexnoise1234.h>

#include <stdint.h>
#include <math.h>

#define SCRIPT_NAME "@math.lua"

static int math_lerp_3nnn_1n(lua_State *L);
static int math_invlerp_3nnn_1n(lua_State *L);
static int math_clamp_v_1n(lua_State *L);
static int math_step_2nn_1n(lua_State *L);
static int math_smoothstep_3nnn_1n(lua_State *L);
static int math_smootherstep_3nnn_1n(lua_State *L);
static int math_sign_1n_1n(lua_State *L);
static int math_signum_1n_1n(lua_State *L);
static int math_sincos_1n_2nn(lua_State *L);
static int math_angle_to_rotation_1n_1n(lua_State *L);
static int math_rotation_to_angle_1n_1n(lua_State *L);
static int math_invsqrt_1n_1n(lua_State *L);
static int math_finvsqrt_1n_1n(lua_State *L);
static int math_rotate_3nnn_2nn(lua_State *L);
static int math_wave_v_1f(lua_State *L);
static int math_tweener_v_1f(lua_State *L);
static int math_noise_v_1f(lua_State *L);

static const char _math_lua[] = {
#include "math.inc"
};

int math_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){
            .data = _math_lua,
            .size = sizeof(_math_lua) / sizeof(char),
            .name = SCRIPT_NAME
        },
        (const struct luaL_Reg[]){
            { "lerp", math_lerp_3nnn_1n },
            { "invlerp", math_invlerp_3nnn_1n },
            { "clamp", math_clamp_v_1n },
            { "step", math_step_2nn_1n },
            { "smoothstep", math_smoothstep_3nnn_1n },
            { "smootherstep", math_smootherstep_3nnn_1n },
            { "sign", math_sign_1n_1n },
            { "signum", math_signum_1n_1n },
            { "sincos", math_sincos_1n_2nn },
            { "angle_to_rotation", math_angle_to_rotation_1n_1n },
            { "rotation_to_angle", math_rotation_to_angle_1n_1n },
            { "invsqrt", math_invsqrt_1n_1n },
            { "finvsqrt", math_finvsqrt_1n_1n },
            { "rotate", math_rotate_3nnn_2nn },
            { "wave", math_wave_v_1f },
            { "tweener", math_tweener_v_1f },
            { "noise", math_noise_v_1f },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { "SINCOS_PERIOD", LUA_CT_INTEGER, { .i = SINCOS_PERIOD } },
            { "EPSILON", LUA_CT_NUMBER, { .n = __FLT_EPSILON__ } },
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, NULL);
}

static int math_lerp_3nnn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float v0 = LUAX_NUMBER(L, 1);
    float v1 = LUAX_NUMBER(L, 2);
    float t = LUAX_NUMBER(L, 3);

    float v = FLERP(v0, v1, t);

    lua_pushnumber(L, (lua_Number)v);

    return 1;
}

static int math_invlerp_3nnn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float v0 = LUAX_NUMBER(L, 1);
    float v1 = LUAX_NUMBER(L, 2);
    float v = LUAX_NUMBER(L, 3);

    float t = FINVLERP(v0, v1, v);

    lua_pushnumber(L, (lua_Number)t);

    return 1;
}

static int math_clamp_3nNN_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = LUAX_NUMBER(L, 1);
    float lower = LUAX_OPTIONAL_NUMBER(L, 2, 0.0f);
    float upper = LUAX_OPTIONAL_NUMBER(L, 3, 1.0f);

    float v = FCLAMP(x, lower, upper);

    lua_pushnumber(L, (lua_Number)v);

    return 1;
}

static int math_clamp_v_1n(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, math_clamp_3nNN_1n)
        LUAX_OVERLOAD_ARITY(3, math_clamp_3nNN_1n)
    LUAX_OVERLOAD_END
}

static int math_step_2nn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float edge = LUAX_NUMBER(L, 1);
    float x = LUAX_NUMBER(L, 2);

    float v = FSTEP(edge, x);

    lua_pushnumber(L, (lua_Number)v);

    return 1;
}

static int math_smoothstep_3nnn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float edge0 = LUAX_NUMBER(L, 1);
    float edge1 = LUAX_NUMBER(L, 2);
    float x = LUAX_NUMBER(L, 3);

    float v = fsmoothstep(edge0, edge1, x);

    lua_pushnumber(L, (lua_Number)v);

    return 1;
}

static int math_smootherstep_3nnn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float edge0 = LUAX_NUMBER(L, 1);
    float edge1 = LUAX_NUMBER(L, 2);
    float x = LUAX_NUMBER(L, 3);

    float v = fsmootherstep(edge0, edge1, x);

    lua_pushnumber(L, (lua_Number)v);

    return 1;
}

static int math_sign_1n_1n(lua_State *L) // This never returns 0.
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, (lua_Number)copysignf(1.0f, x)); // absolute value of the 1st, sign of the 2nd.

    return 1;
}

static int math_signum_1n_1n(lua_State *L) // Returns -1, 0, 1
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = LUAX_NUMBER(L, 1);

    lua_pushinteger(L, (lua_Integer)FSIGNUM(x));

    return 1;
}

static int math_sincos_1n_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int rotation = LUAX_INTEGER(L, 1);

    float s, c;
    fsincos(rotation, &s, &c);

    lua_pushnumber(L, (lua_Number)s);
    lua_pushnumber(L, (lua_Number)c);

    return 2;
}

static int math_angle_to_rotation_1n_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float angle = LUAX_NUMBER(L, 1);

    int rotation = fator(angle);

    lua_pushinteger(L, (lua_Integer)rotation);

    return 1;
}

static int math_rotation_to_angle_1n_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int rotation = LUAX_INTEGER(L, 1);

    float angle = frtoa(rotation);

    lua_pushnumber(L, (lua_Number)angle);

    return 1;
}

static int math_invsqrt_1n_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = LUAX_NUMBER(L, 1);

    const float y = 1.0f / sqrtf(x);
    lua_pushnumber(L, (lua_Number)y);

    return 1;
}

// See: https://en.wikipedia.org/wiki/Fast_inverse_square_root
//
// The magic number is for doubles is from https://cs.uwaterloo.ca/~m32rober/rsqrt.pdf
//     i = 0x5fe6eb50c7b537a9 - (i >> 1);
static inline float _Q_rsqrt(float number)
{
    const float x2 = number * 0.5f;
    float y = number;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
    int32_t i = *(int32_t *)&y;             // evil floating point bit level hacking
    i = 0x5f3759df - (i >> 1);              // what the fuck?
    y = *(float *)&i;
#pragma GCC diagnostic pop
    y = y * (1.5f - (x2 * y * y));          // 1st iteration
//	y = y * (threehalfs - (x2 * y * y));    // 2nd iteration, this can be removed
    return y;
}

static int math_finvsqrt_1n_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = LUAX_NUMBER(L, 1);

    const float y = _Q_rsqrt(x);
    lua_pushnumber(L, (lua_Number)y);

    return 1;
}

static int math_rotate_3nnn_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = LUAX_NUMBER(L, 1);
    float y = LUAX_NUMBER(L, 2);
    int rotation = LUAX_INTEGER(L, 3);

    float s, c;
    fsincos(rotation, &s, &c);

    float rx = c * x - s * y;
    float ry = s * x + c * y;

    lua_pushnumber(L, (lua_Number)rx);
    lua_pushnumber(L, (lua_Number)ry);

    return 2;
}

static int _vanilla_wave_1n_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float t = LUAX_NUMBER(L, 1);

    const Wave_t *wave = (const Wave_t *)LUAX_USERDATA(L, lua_upvalueindex(1));

    float value = wave->function(t);

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int _normalize_wave_1n_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float time = LUAX_NUMBER(L, 1);

    const Wave_t *wave = (const Wave_t *)LUAX_USERDATA(L, lua_upvalueindex(1));
    float period = LUAX_NUMBER(L, lua_upvalueindex(2));
    float amplitude = LUAX_NUMBER(L, lua_upvalueindex(3));

    float ratio = time / period;
    float value = wave->function(ratio) * amplitude;

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int math_wave_1s_1f(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);

    const Wave_t *wave = wave_from_name(name);
    if (!wave) {
        return luaL_error(L, "unknown wave `%s`", name);
    }

    lua_pushlightuserdata(L, (void *)wave);
    lua_pushcclosure(L, _vanilla_wave_1n_1n, 1);

    return 1;
}

static int math_wave_3snN_1f(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    float period = LUAX_NUMBER(L, 2);
    float amplitude = LUAX_OPTIONAL_NUMBER(L, 3, 1.0f);

    const Wave_t *wave = wave_from_name(name);
    if (!wave) {
        return luaL_error(L, "unknown wave `%s`", name);
    }

    lua_pushlightuserdata(L, (void *)wave);
    lua_pushnumber(L, (lua_Number)period);
    lua_pushnumber(L, (lua_Number)amplitude);
    lua_pushcclosure(L, _normalize_wave_1n_1n, 3);

    return 1;
}

static int math_wave_v_1f(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, math_wave_1s_1f)
        LUAX_OVERLOAD_ARITY(2, math_wave_3snN_1f)
        LUAX_OVERLOAD_ARITY(3, math_wave_3snN_1f)
    LUAX_OVERLOAD_END
}

static int _vanilla_tweener_1n_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    const Easing_t *easing = (const Easing_t *)LUAX_USERDATA(L, lua_upvalueindex(1));

    float value = easing->function(ratio);

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int _normalize_tweener_1n_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float time = LUAX_NUMBER(L, 1);

    const Easing_t *easing = (const Easing_t *)LUAX_USERDATA(L, lua_upvalueindex(1));
    float duration = LUAX_NUMBER(L, lua_upvalueindex(2));

    float ratio = time / duration;
    float value = easing->function(ratio);

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int _normalize_lerp_tweener_1n_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float time = LUAX_NUMBER(L, 1);

    const Easing_t *easing = (const Easing_t *)LUAX_USERDATA(L, lua_upvalueindex(1));
    float duration = LUAX_NUMBER(L, lua_upvalueindex(2));
    float from = LUAX_NUMBER(L, lua_upvalueindex(3));
    float to = LUAX_NUMBER(L, lua_upvalueindex(4));

    float ratio = time / duration;
    float eased_ratio = easing->function(ratio);
    float value = FLERP(from, to, eased_ratio);

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int math_tweener_1s_1f(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);

    const Easing_t *easing  = easing_from_id(name);
    if (!easing) {
        return luaL_error(L, "unknown easing `%s`", name);
    }

    lua_pushlightuserdata(L, (void *)easing);
    lua_pushcclosure(L, _vanilla_tweener_1n_1n, 1);

    return 1;
}

static int math_tweener_2sn_1f(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    float duration = LUAX_NUMBER(L, 2);

    const Easing_t *easing  = easing_from_id(name);
    if (!easing) {
        return luaL_error(L, "unknown easing `%s`", name);
    }

    lua_pushlightuserdata(L, (void *)easing);
    lua_pushnumber(L, (lua_Number)duration);
    lua_pushcclosure(L, _normalize_tweener_1n_1n, 2);

    return 1;
}

static int math_tweener_4snnn_1f(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    float duration = LUAX_NUMBER(L, 2);
    float from = LUAX_NUMBER(L, 3);
    float to = LUAX_NUMBER(L, 4);

    const Easing_t *easing  = easing_from_id(name);
    if (!easing) {
        return luaL_error(L, "unknown easing `%s`", name);
    }

    lua_pushlightuserdata(L, (void *)easing);
    lua_pushnumber(L, (lua_Number)duration);
    lua_pushnumber(L, (lua_Number)from);
    lua_pushnumber(L, (lua_Number)to);
    lua_pushcclosure(L, _normalize_lerp_tweener_1n_1n, 4);

    return 1;
}

static int math_tweener_v_1f(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, math_tweener_1s_1f)
        LUAX_OVERLOAD_ARITY(2, math_tweener_2sn_1f)
        LUAX_OVERLOAD_ARITY(4, math_tweener_4snnn_1f)
    LUAX_OVERLOAD_END
}

static int math_noise_2sn_1f(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *type = LUAX_STRING(L, 1);
    float x = LUAX_NUMBER(L, 2);

    if (type[0] == 's') {
        const float value = snoise1(x);
        lua_pushnumber(L, (lua_Number)value);
    } else
    if (type[0] == 'p') {
        const float value = noise1(x);
        lua_pushnumber(L, (lua_Number)value);
    }

    return 1;
}

static int math_noise_3snn_1f(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *type = LUAX_STRING(L, 1);
    float x = LUAX_NUMBER(L, 2);
    float y = LUAX_NUMBER(L, 3);

    if (type[0] == 's') {
        const float value = snoise2(x, y);
        lua_pushnumber(L, (lua_Number)value);
    } else
    if (type[0] == 'p') {
        const float value = noise2(x, y);
        lua_pushnumber(L, (lua_Number)value);
    }

    return 1;
}

static int math_noise_4snnn_1f(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *type = LUAX_STRING(L, 1);
    float x = LUAX_NUMBER(L, 2);
    float y = LUAX_NUMBER(L, 3);
    float z = LUAX_NUMBER(L, 4);

    if (type[0] == 's') {
        const float value = snoise3(x, y, z);
        lua_pushnumber(L, (lua_Number)value);
    } else
    if (type[0] == 'p') {
        const float value = noise3(x, y, z);
        lua_pushnumber(L, (lua_Number)value);
    }

    return 1;
}

static int math_noise_5snnnn_1f(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *type = LUAX_STRING(L, 1);
    float x = LUAX_NUMBER(L, 2);
    float y = LUAX_NUMBER(L, 3);
    float z = LUAX_NUMBER(L, 4);
    float w = LUAX_NUMBER(L, 5);

    if (type[0] == 's') {
        const float value = snoise4(x, y, z, w);
        lua_pushnumber(L, (lua_Number)value);
    } else
    if (type[0] == 'p') {
        const float value = noise4(x, y, z, w);
        lua_pushnumber(L, (lua_Number)value);
    }

    return 1;
}

static int math_noise_v_1f(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, math_noise_2sn_1f)
        LUAX_OVERLOAD_ARITY(3, math_noise_3snn_1f)
        LUAX_OVERLOAD_ARITY(4, math_noise_4snnn_1f)
        LUAX_OVERLOAD_ARITY(5, math_noise_5snnnn_1f)
    LUAX_OVERLOAD_END
}
