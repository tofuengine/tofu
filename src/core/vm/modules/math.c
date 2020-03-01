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
#include <libs/sincos.h>
#include <libs/log.h>
#include <libs/wave.h>

#include <math.h>

#include "udt.h"

#define META_TABLE  "Tofu_Core_Math_mt"

//static int math_lerp(lua_State *L);
static int math_wave(lua_State *L);
static int math_sincos(lua_State *L);
static int math_angle_to_rotation(lua_State *L);
static int math_rotation_to_angle(lua_State *L);

static const struct luaL_Reg _math_functions[] = {
//    { "lerp", math_lerp },
    { "wave", math_wave },
    { "sincos", math_sincos },
    { "angle_to_rotation", math_angle_to_rotation },
    { "rotation_to_angle", math_rotation_to_angle },
    { NULL, NULL }
};

static const luaX_Const _math_constants[] = {
    { "SINCOS_PERIOD", LUA_CT_INTEGER, { .i = SINCOS_PERIOD } },
    { NULL }
};

static const unsigned char _math_lua[] = {
#include "math.inc"
};

static luaX_Script _math_script = { (const char *)_math_lua, sizeof(_math_lua), "@math.lua" }; // Trace as filename internally.

int math_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_math_script, _math_functions, _math_constants, nup, META_TABLE);
}

// static int math_lerp(lua_State *L)
// {
//     LUAX_SIGNATURE_BEGIN(L)
//         LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
//         LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
//         LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
//     LUAX_SIGNATURE_END
//     float a = LUAX_NUMBER(L, 1);
//     float b = LUAX_NUMBER(L, 1);
//     float r = LUAX_NUMBER(L, 1);

//     float value = a * (1.0f - r) + b * r; // Precise method, which guarantees correct result `r = 1`.

//     lua_pushnumber(L, value);

//     return 1;
// }

static int _vanilla(lua_State *L)
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

static int _normalize(lua_State *L)
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
    lua_pushcclosure(L, _vanilla, 1);

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
    lua_pushcclosure(L, _normalize, 3);

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
