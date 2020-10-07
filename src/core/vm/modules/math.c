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

#include "math.h"

#include <config.h>
#include <libs/easing.h>
#include <libs/log.h>
#include <libs/sincos.h>
#include <libs/wave.h>

#include <math.h>

#include "udt.h"

static int math_lerp(lua_State *L);
static int math_invlerp(lua_State *L);
static int math_clamp(lua_State *L);
static int math_step(lua_State *L);
static int math_smoothstep(lua_State *L);
static int math_smootherstep(lua_State *L);
static int math_sign(lua_State *L);
static int math_signum(lua_State *L);
static int math_sincos(lua_State *L);
static int math_angle_to_rotation(lua_State *L);
static int math_rotation_to_angle(lua_State *L);
static int math_wave(lua_State *L);
static int math_tweener(lua_State *L);

static const struct luaL_Reg _math_functions[] = {
    { "lerp", math_lerp },
    { "invlerp", math_invlerp },
    { "clamp", math_clamp },
    { "step", math_step },
    { "smoothstep", math_smoothstep },
    { "smootherstep", math_smootherstep },
    { "sign", math_sign },
    { "signum", math_signum },
    { "sincos", math_sincos },
    { "angle_to_rotation", math_angle_to_rotation },
    { "rotation_to_angle", math_rotation_to_angle },
    { "wave", math_wave },
    { "tweener", math_tweener },
    { NULL, NULL }
};

static const luaX_Const _math_constants[] = {
    { "SINCOS_PERIOD", LUA_CT_INTEGER, { .i = SINCOS_PERIOD } },
    { "EPSILON", LUA_CT_NUMBER, { .n = __FLT_EPSILON__ } },
    { NULL }
};

static const uint8_t _math_lua[] = {
#include "math.inc"
};

static luaX_Script _math_script = { (const char *)_math_lua, sizeof(_math_lua), "@math.lua" }; // Trace as filename internally.

int math_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_math_script, _math_functions, _math_constants, nup, NULL);
}

static inline float _lerpf(float v0, float v1, float t)
{
    // More numerical stable than the following one.
    // return (v1 - v0) * t + v0
    // see: https://en.wikipedia.org/wiki/Linear_interpolation
    return v0 * (1.0f - t) + v1 * t;
}

static int math_lerp(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float v0 = LUAX_NUMBER(L, 1);
    float v1 = LUAX_NUMBER(L, 2);
    float t = LUAX_NUMBER(L, 3);

    float v = _lerpf(v0, v1, t);

    lua_pushnumber(L, v);

    return 1;
}

static inline float _invlerpf(float v0, float v1, float v)
{
	return (v - v0) / (v1 - v0);
}

static int math_invlerp(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float v0 = LUAX_NUMBER(L, 1);
    float v1 = LUAX_NUMBER(L, 2);
    float v = LUAX_NUMBER(L, 3);

    float t = _invlerpf(v0, v1, v);

    lua_pushnumber(L, t);

    return 1;
}

static inline float _clampf(float x, float lower, float upper) // TODO: move to a separate lib file?
{
    return x < lower ? lower : (x > upper ? upper : x);
}

static int math_clamp(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = LUAX_NUMBER(L, 1);
    float lower = LUAX_NUMBER(L, 2);
    float upper = LUAX_NUMBER(L, 3);

    float v = _clampf(x, lower, upper);

    lua_pushnumber(L, v);

    return 1;
}

static int math_step(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float edge = LUAX_NUMBER(L, 1);
    float x = LUAX_NUMBER(L, 2);

    float v = x < edge ? 0.0f : 1.0f;

    lua_pushnumber(L, v);

    return 1;
}

static int math_smoothstep(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float edge0 = LUAX_NUMBER(L, 1);
    float edge1 = LUAX_NUMBER(L, 2);
    float x = LUAX_NUMBER(L, 3);

    x = _clampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f); // Scale, bias and saturate x to [0, 1] range.
    float v = x * x * (3.0f - 2.0f * x); // Evaluate polynomial.

    lua_pushnumber(L, v);

    return 1;
}

static int math_smootherstep(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float edge0 = LUAX_NUMBER(L, 1);
    float edge1 = LUAX_NUMBER(L, 2);
    float x = LUAX_NUMBER(L, 3);

    x = _clampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    float v = x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);

    lua_pushnumber(L, v);

    return 1;
}

static int math_sign(lua_State *L) // This never returns 0.
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, copysignf(1.0f, x)); // absolute value of the 1st, sign of the 2nd.

    return 1;
}

static int math_signum(lua_State *L) // Returns -1, 0, 1
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = LUAX_NUMBER(L, 1);

//    int sign = x > 0.0f ? 1 : (x < 0.0f ? -1 : 0);
//    lua_pushinteger(L, sign);
    lua_pushinteger(L, (x > 0.0f) - (x < 0.0f));

    return 1;
}

static int math_sincos(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int rotation = LUAX_INTEGER(L, 1);

    float s, c;
    fsincos(rotation, &s, &c);

    lua_pushnumber(L, s);
    lua_pushnumber(L, c);

    return 2;
}

static int math_angle_to_rotation(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float angle = LUAX_NUMBER(L, 1);

    int rotation = fator(angle);

    lua_pushinteger(L, rotation);

    return 1;
}

static int math_rotation_to_angle(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int rotation = LUAX_INTEGER(L, 1);

    float angle = frtoa(rotation);

    lua_pushnumber(L, angle);

    return 1;
}

static int _vanilla_wave(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float t = LUAX_NUMBER(L, 1);

    const Wave_t *wave = (const Wave_t *)LUAX_USERDATA(L, lua_upvalueindex(1));

    float value = wave->function(t);

    lua_pushnumber(L, value);

    return 1;
}

static int _normalize_wave(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float time = LUAX_NUMBER(L, 1);

    const Wave_t *wave = (const Wave_t *)LUAX_USERDATA(L, lua_upvalueindex(1));
    float period = LUAX_NUMBER(L, lua_upvalueindex(2));
    float amplitude = LUAX_NUMBER(L, lua_upvalueindex(3));

    float t = time / period;
    float value = wave->function(t) * amplitude;

    lua_pushnumber(L, value);

    return 1;
}

static int math_wave1(lua_State *L)
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
    lua_pushcclosure(L, _vanilla_wave, 1);

    return 1;
}

static int math_wave2_3(lua_State *L)
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
    lua_pushnumber(L, period);
    lua_pushnumber(L, amplitude);
    lua_pushcclosure(L, _normalize_wave, 3);

    return 1;
}

static int math_wave(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, math_wave1)
        LUAX_OVERLOAD_ARITY(2, math_wave2_3)
        LUAX_OVERLOAD_ARITY(3, math_wave2_3)
    LUAX_OVERLOAD_END
}

static int _vanilla_tweener(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    const Easing_t *easing = (const Easing_t *)LUAX_USERDATA(L, lua_upvalueindex(1));

    float value = easing->function(ratio);

    lua_pushnumber(L, value);

    return 1;
}

static int _normalize_tweener(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float time = LUAX_NUMBER(L, 1);

    const Easing_t *easing = (const Easing_t *)LUAX_USERDATA(L, lua_upvalueindex(1));
    float duration = LUAX_NUMBER(L, lua_upvalueindex(2));

    float ratio = time / duration;
    float value = easing->function(ratio);

    lua_pushnumber(L, value);

    return 1;
}

static int _normalize_lerp_tweener(lua_State *L)
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
    float value = _lerpf(from, to, easing->function(ratio));

    lua_pushnumber(L, value);

    return 1;
}

static int math_tweener1(lua_State *L)
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
    lua_pushcclosure(L, _vanilla_tweener, 1);

    return 1;
}

static int math_tweener2(lua_State *L)
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
    lua_pushnumber(L, duration);
    lua_pushcclosure(L, _normalize_tweener, 2);

    return 1;
}

static int math_tweener4(lua_State *L)
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
    lua_pushnumber(L, duration);
    lua_pushnumber(L, from);
    lua_pushnumber(L, to);
    lua_pushcclosure(L, _normalize_lerp_tweener, 4);

    return 1;
}

static int math_tweener(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, math_tweener1)
        LUAX_OVERLOAD_ARITY(2, math_tweener2)
        LUAX_OVERLOAD_ARITY(4, math_tweener4)
    LUAX_OVERLOAD_END
}
