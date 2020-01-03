/*
 * Copyright (c) 2019-2020 by Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "math.h"

#include <config.h>
#include <libs/sincos.h>
#include <libs/log.h>

#include "udt.h"

#define MATH_MT         "Tofu_Math_mt"

static int math_sincos(lua_State *L);
static int math_angle_to_rotation(lua_State *L);
static int math_rotation_to_angle(lua_State *L);

static const struct luaL_Reg _math_functions[] = {
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
    return luaX_newmodule(L, &_math_script, _math_functions, _math_constants, nup, MATH_MT);
}

static int math_sincos(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int rotation = lua_tointeger(L, 1);

    float s, c;
    fsincos(rotation, &s, &c);

    lua_pushnumber(L, s);
    lua_pushnumber(L, c);

    return 2;
}

static int math_angle_to_rotation(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float angle = lua_tonumber(L, 1);

    int rotation = fator(angle);

    lua_pushinteger(L, rotation);

    return 1;
}

static int math_rotation_to_angle(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int rotation = lua_tointeger(L, 1);

    float angle = frtoa(rotation);

    lua_pushnumber(L, angle);

    return 1;
}
