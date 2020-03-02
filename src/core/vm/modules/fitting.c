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

#include "fitting.h"

#include <config.h>
#include <libs/log.h>

#include <math.h>

#include "udt.h"

#define META_TABLE  "Tofu_Math_Fitting_mt"

//static int fitting_lerp(lua_State *L);

static const struct luaL_Reg _fitting_functions[] = {
//    { "lerp", fitting_lerp },
    { NULL, NULL }
};

static const luaX_Const _fitting_constants[] = {
    { NULL }
};

static const unsigned char _fitting_lua[] = {
#include "fitting.inc"
};

static luaX_Script _fitting_script = { (const char *)_fitting_lua, sizeof(_fitting_lua), "@fitting.lua" }; // Trace as filename internally.

int fitting_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_fitting_script, _fitting_functions, _fitting_constants, nup, META_TABLE);
}

// static int fitting_lerp(lua_State *L)
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
