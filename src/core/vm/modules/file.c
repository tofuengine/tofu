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

#include <string.h>

static int file_load_2sS_1s(lua_State *L);
static int file_store_3ssS_0(lua_State *L);

static const struct luaL_Reg _file_functions[] = {
    { "load", file_load_2sS_1s },
    { "store", file_store_3ssS_0 },
    { NULL, NULL }
};

int file_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _file_functions, NULL, nup, NULL);
}

static int file_load_2sS_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    const char *mode = LUAX_OPTIONAL_STRING(L, 2, "string");

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));

    if (mode[0] == 's') { // "string"
        const Storage_Resource_t *resource = Storage_load(storage, name, STORAGE_RESOURCE_STRING);
        if (!resource) {
            return luaL_error(L, "can't load file `%s`", name);
        }
        lua_pushlstring(L, S_SCHARS(resource), S_SLENTGH(resource));
    } else
    if (mode[0] == 'b') { // "blob"
        const Storage_Resource_t *resource = Storage_load(storage, name, STORAGE_RESOURCE_BLOB);
        if (!resource) {
            return luaL_error(L, "can't load file `%s`", name);
        }
        lua_pushlstring(L, S_BPTR(resource), S_BSIZE(resource)); // Lua's strings can contain bytes.
    } else {
        return luaL_error(L, "unknown mode `%s`", mode);
    }

    return 1;
}

static int file_store_3ssS_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    const char *data = LUAX_STRING(L, 2);
    const char *mode = LUAX_OPTIONAL_STRING(L, 3, "string");

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));

    Storage_Resource_t resource = { 0 };
    if (mode[0] == 's') { // "string"
        resource = (Storage_Resource_t){
                // .name = name,
                .type = STORAGE_RESOURCE_STRING,
                .var = {
                    .string = {
                        .chars = (char *)data,
                        .length = strlen(data)
                    }
                }
            };
    } else
    if (mode[0] == 'b') { // "blob"
        resource = (Storage_Resource_t){
                // .name = name,
                .type = STORAGE_RESOURCE_BLOB,
                .var = {
                    .blob = {
                        .ptr = (void *)data,
                        .size = strlen(data) + 1
                    }
                }
            };
    } else {
        return luaL_error(L, "unknown mode `%s`", mode);
    }

    bool stored = Storage_store(storage, name, &resource);
    if (!stored) {
        return luaL_error(L, "can't store file `%s`", name);
    }

    return 0;
}
