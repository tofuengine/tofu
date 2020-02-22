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

#include <math.h>

#include "udt.h"

#define META_TABLE  "Tofu_Core_Math_mt"

static int math_lerp(lua_State *L);
static int math_sine_wave(lua_State *L);
static int math_square_wave(lua_State *L);
static int math_triangle_wave(lua_State *L);
static int math_sawtooth_wave(lua_State *L);
static int math_sincos(lua_State *L);
static int math_angle_to_rotation(lua_State *L);
static int math_rotation_to_angle(lua_State *L);

static const struct luaL_Reg _math_functions[] = {
    { "lerp", math_lerp },
    { "sine_wave", math_sine_wave },
    { "square_wave", math_square_wave },
    { "triangle_wave", math_triangle_wave },
    { "sawtooth_wave", math_sawtooth_wave },
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

static int math_lerp(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float a = LUAX_NUMBER(L, 1);
    float b = LUAX_NUMBER(L, 1);
    float r = LUAX_NUMBER(L, 1);

    float value = a * (1.0f - r) + b * r; // Precise method, which guarantees correct result `r = 1`.

    lua_pushnumber(L, value);

    return 1;
}

// TODO: make as functions like easings? One creates a wave funtion with a period..
static int math_sine_wave(lua_State *L) // TODO: add overload with two args.
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float r = LUAX_NUMBER(L, 1);

    float value = sinf(r * 2.0f * (float)M_PI); // TODO: move to library for reuse in audio?

    lua_pushnumber(L, value);

    return 1;
}

static int math_square_wave(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float r = LUAX_NUMBER(L, 1);

    float value = 2.0f * (2.0f * floorf(r) - floorf(2.0f * r)) + 1.0f;

    lua_pushnumber(L, value);

    return 1;
}

static int math_triangle_wave(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float r = LUAX_NUMBER(L, 1);

    float value = 2.0f * fabsf(2.0f * (r + 0.25f - floorf(r + 0.75f))) - 1.0f;

    lua_pushnumber(L, value);

    return 1;
}

static int math_sawtooth_wave(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float r = LUAX_NUMBER(L, 1);

    float value = 2.0f * (r - floorf(0.5f + r));

    lua_pushnumber(L, value);

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
