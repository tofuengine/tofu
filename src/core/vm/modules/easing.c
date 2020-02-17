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

#include "udt.h"

#define META_TABLE  "Tofu_Util_Easing_mt"

static int easing_linear(lua_State *L);

static const struct luaL_Reg _easing_functions[] = {
    { "linear", easing_linear },
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

static int easing_linear(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float ratio = LUAX_NUMBER(L, 1);

    lua_pushnumber(L, ratio);

    return 1;
}
