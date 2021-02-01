/*
 * MIT License
 * 
 * Copyright (c) 2019-2021 Marco Lizza
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

#include "file.h"

#include <config.h>
#include <core/io/storage.h>
#include <libs/luax.h>

#include "udt.h"

static int file_as_string(lua_State *L);
static int file_as_binary(lua_State *L);

static const struct luaL_Reg _file_functions[] = {
    { "as_string", file_as_string },
    { "as_binary", file_as_binary },
    { NULL, NULL }
};

int file_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _file_functions, NULL, nup, NULL);
}

static int file_as_string(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));

    const Storage_Resource_t *resource = Storage_load(storage, name, STORAGE_RESOURCE_BLOB);
    if (!resource) {
        return luaL_error(L, "can't load file `%s`", name);
    }
    lua_pushlstring(L, S_BPTR(resource), S_BSIZE(resource));

    return 1;
}

static int file_as_binary(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));

    const Storage_Resource_t *resource = Storage_load(storage, name, STORAGE_RESOURCE_BLOB);
    if (!resource) {
        return luaL_error(L, "can't load file `%s`", name);
    }
//    lua_pushlstring(L, buffer, size);
    lua_pushnil(L); // TODO: read the file as a Base64 or similar encoded string.
    // FIXME: useless, Lua's strings can contain bytes.

    return 1;
}
