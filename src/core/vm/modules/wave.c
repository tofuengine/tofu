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

#include "wave.h"

#include <config.h>
#include <libs/log.h>
#include <libs/wave.h>

#include <math.h>

#include "udt.h"

#define META_TABLE  "Tofu_Math_Wave_mt"

static int wave_new(lua_State *L);

static const struct luaL_Reg _wave_functions[] = {
    { "new", wave_new },
    { NULL, NULL }
};

int wave_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _wave_functions, NULL, nup, META_TABLE);
}

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

static int wave_new1(lua_State *L)
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

static int wave_new2_3(lua_State *L)
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

static int wave_new(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, wave_new1)
        LUAX_OVERLOAD_ARITY(2, wave_new2_3)
        LUAX_OVERLOAD_ARITY(3, wave_new2_3)
    LUAX_OVERLOAD_END
}
