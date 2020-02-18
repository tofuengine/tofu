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

#include "easings.c"
#include "udt.h"

#include <math.h>

#define META_TABLE  "Tofu_Util_Easing_mt"

static int easing_linear(lua_State *L);
static int easing_quadratic_in(lua_State *L);
static int easing_quadratic_out(lua_State *L);
static int easing_quadratic_in_out(lua_State *L);
static int easing_cubic_in(lua_State *L);
static int easing_cubic_out(lua_State *L);
static int easing_cubic_in_out(lua_State *L);
static int easing_sine_in(lua_State *L);
static int easing_sine_out(lua_State *L);
static int easing_sine_in_out(lua_State *L);
static int easing_circular_in(lua_State *L);
static int easing_circular_out(lua_State *L);
static int easing_circular_in_out(lua_State *L);
static int easing_exponential_in(lua_State *L);
static int easing_exponential_out(lua_State *L);
static int easing_exponential_in_out(lua_State *L);
static int easing_elastic_in(lua_State *L);
static int easing_elastic_out(lua_State *L);
static int easing_elastic_in_out(lua_State *L);
static int easing_back_in(lua_State *L);
static int easing_back_out(lua_State *L);
static int easing_back_in_out(lua_State *L);
static int easing_bounce_in(lua_State *L);
static int easing_bounce_out(lua_State *L);
static int easing_bounce_in_out(lua_State *L);

static const struct luaL_Reg _easing_functions[] = {
    { "linear", easing_linear },
    { "quadratic_in", easing_quadratic_in },
    { "quadratic_out", easing_quadratic_out },
    { "quadratic_in_out", easing_quadratic_in_out },
    { "cubic_in", easing_cubic_in },
    { "cubic_out", easing_cubic_out },
    { "cubic_in_out", easing_cubic_in_out },
    { "sine_in", easing_sine_in },
    { "sine_out", easing_sine_out },
    { "sine_in_out", easing_sine_in_out },
    { "circular_in", easing_circular_in },
    { "circular_out", easing_circular_out },
    { "circular_in_out", easing_circular_in_out },
    { "exponential_in", easing_exponential_in },
    { "exponential_out", easing_exponential_out },
    { "exponential_in_out", easing_exponential_in_out },
    { "elastic_in", easing_elastic_in },
    { "elastic_out", easing_elastic_out },
    { "elastic_in_out", easing_elastic_in_out },
    { "back_in", easing_back_in },
    { "back_out", easing_back_out },
    { "back_in_out", easing_back_in_out },
    { "bounce_in", easing_bounce_in },
    { "bounce_out", easing_bounce_out },
    { "bounce_in_out", easing_bounce_in_out },
    { NULL, NULL }
};

static const uint8_t _easing_lua[] = {
#include "easing.inc"
};

static luaX_Script _easing_script = { (const char *)_easing_lua, sizeof(_easing_lua), "@easing.lua" }; // Trace as filename internally.

int easing_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_easing_script, _easing_functions, NULL, nup, NULL);
}

static int easing_tweener1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);

    lua_pushlightuserdata(L, easings_find(name));
    lua_pushcclosure(L, _ratio, 1);

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

    lua_pushlightuserdata(L, easings_find(name));
    lua_pushnumber(L, duration);
    lua_pushcclosure(L, _time_duration, 2);

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

    lua_pushlightuserdata(L, easings_find(name));
    lua_pushnumber(L, duration);
    lua_pushnumber(L, from);
    lua_pushnumber(L, to);
    lua_pushcclosure(L, _time_duration_from_to, 4);

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

typedef float (*Easing_Function_t)(float ratio); // No need to expose the easings!!!

static int _ratio(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    Easing_Function_t function = (Easing_Function_t)LUAX_USERDATA(L, lua_upvalueindex(1));

    float value = function(ratio);

    lua_pushnumber(L, value);

    return 1;
}

static int _time_duration(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float time = LUAX_NUMBER(L, 1);

    Easing_Function_t function = (Easing_Function_t)LUAX_USERDATA(L, lua_upvalueindex(1));
    float duration = (const File_System_t *)LUAX_USERDATA(L, lua_upvalueindex(2));

    float ratio = time / duration;
    float value = function(ratio);

    lua_pushnumber(L, value);

    return 1;
}

static int _time_duration_from_to(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float time = LUAX_NUMBER(L, 1);

    Easing_Function_t function = (Easing_Function_t)LUAX_USERDATA(L, lua_upvalueindex(1));
    float duration = LUAX_NUMBER(L, lua_upvalueindex(2));
    float from = LUAX_NUMBER(L, lua_upvalueindex(3));
    float to = LUAX_NUMBER(L, lua_upvalueindex(4));

    float ratio = time / duration;
    float value = function(ratio);

    lua_pushnumber(L, (1.0f - value) * from + value * to); // Precise method, which guarantees correct result `r = 1`.

    return 1;
}

static int easing_linear(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _linear(ratio));

    return 1;
}

static int easing_quadratic_in(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _quadratic_in(ratio));

    return 1;
}

static int easing_quadratic_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _quadratic_out(ratio));

    return 1;
}

static int easing_quadratic_in_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _quadratic_in_out(ratio));

    return 1;
}

static int easing_cubic_in(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _cubic_in(ratio));

    return 1;
}

static int easing_cubic_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _cubic_out(ratio));

    return 1;
}

static int easing_cubic_in_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _cubic_in_out(ratio));

    return 1;
}

static int easing_sine_in(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _sine_in(ratio));

    return 1;
}

static int easing_sine_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _sine_out(ratio));

    return 1;
}

static int easing_sine_in_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _sine_in_out(ratio));

    return 1;
}

static int easing_circular_in(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _circular_in(ratio));

    return 1;
}

static int easing_circular_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _circular_out(ratio));

    return 1;
}

static int easing_circular_in_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _circular_in_out(ratio));

    return 1;
}

static int easing_exponential_in(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _exponential_in(ratio));

    return 1;
}

static int easing_exponential_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _exponential_out(ratio));

    return 1;
}

static int easing_exponential_in_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _exponential_in_out(ratio));

    return 1;
}

static int easing_elastic_in(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _elastic_in(ratio));

    return 1;
}

static int easing_elastic_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _elastic_out(ratio));

    return 1;
}

static int easing_elastic_in_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _elastic_in_out(ratio));

    return 1;
}

static int easing_back_in(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _back_in(ratio));

    return 1;
}

static int easing_back_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _back_out(ratio));

    return 1;
}

static int easing_back_in_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _back_in_out(ratio));

    return 1;
}

static int easing_bounce_in(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _bounce_in(ratio));

    return 1;
}

static int easing_bounce_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _bounce_out(ratio));

    return 1;
}

static int easing_bounce_in_out(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, _bounce_in_out(ratio));

    return 1;
}
