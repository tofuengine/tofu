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

#include "vector.h"

#include <libs/luax.h>

#include "udt.h"

#define META_TABLE  "Tofu_Core_Vector_mt"

static int vector_new(lua_State *L);

static const struct luaL_Reg _vector_functions[] = {
    { "new", vector_new },
    { NULL, NULL }
};

static const uint8_t _vector_lua[] = {
#include "vector.inc"
};

static luaX_Script _vector_script = { (const char *)_vector_lua, sizeof(_vector_lua), "@vector.lua" }; // Trace as filename internally.

int vector_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_vector_script, _vector_functions, NULL, nup, META_TABLE);
}

static int vector_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = = (float)LUAX_OPTIONAL_NUMBER(L, 1, 0.0f);
    float y = = (float)LUAX_OPTIONAL_NUMBER(L, 2, 0.0f);

    Vector_Class_t *self = (Vector_Class_t *)lua_newuserdata(L, sizeof(Vector_Class_t));
    *self = (Vector_Class_t){
            .x = x,
            .y = y
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p allocated w/ sheet %p for default context", self);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}
