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

#include "easing.h"

#include <config.h>
#include <core/vm/interpreter.h>
#include <libs/log.h>
#include <libs/easing.h>

#include "udt.h"

#define META_TABLE  "Tofu_Util_Easing_mt"

static int easing_tweener(lua_State *L);

static const struct luaL_Reg _easing_functions[] = {
    { "tweener", easing_tweener },
    { NULL, NULL }
};

int easing_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _easing_functions, NULL, nup, META_TABLE);
}

static int _vanilla(lua_State *L)
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

static int _normalize(lua_State *L)
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

static inline float _lerpf(float a, float b, float r)
{
    return a * (1.0f - r) + b * r; // Precise method, which guarantees correct result `r = 1`.
}

static int _normalize_lerp(lua_State *L)
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

static int easing_tweener1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);

    const Easing_t *easing  = easing_from_id(name);
    if (!easing) {
        luaL_error(L, "unknown easing `%s`", name);
    }

    lua_pushlightuserdata(L, (void *)easing);
    lua_pushcclosure(L, _vanilla, 1);

    return 1;
}

static int easing_tweener2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    float duration = LUAX_NUMBER(L, 2);

    const Easing_t *easing  = easing_from_id(name);
    if (!easing) {
        luaL_error(L, "unknown easing `%s`", name);
    }

    lua_pushlightuserdata(L, (void *)easing);
    lua_pushnumber(L, duration);
    lua_pushcclosure(L, _normalize, 2);

    return 1;
}

static int easing_tweener4(lua_State *L)
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
        luaL_error(L, "unknown easing `%s`", name);
    }

    lua_pushlightuserdata(L, (void *)easing);
    lua_pushnumber(L, duration);
    lua_pushnumber(L, from);
    lua_pushnumber(L, to);
    lua_pushcclosure(L, _normalize_lerp, 4);

    return 1;
}

static int easing_tweener(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, easing_tweener1)
        LUAX_OVERLOAD_ARITY(2, easing_tweener2)
        LUAX_OVERLOAD_ARITY(4, easing_tweener4)
    LUAX_OVERLOAD_END
}
