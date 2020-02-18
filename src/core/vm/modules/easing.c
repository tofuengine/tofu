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

// https://github.com/warrenm/AHEasing/blob/master/AHEasing/easing.c

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
