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

#include "file.h"

#include <config.h>
#include <core/vm/interpreter.h>
#include <libs/fs/fsaux.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "udt.h"

#include <string.h>

#define META_TABLE  "Tofu_IO_File_mt"

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
    return luaX_newmodule(L, NULL, _file_functions, NULL, nup, META_TABLE);
}

static int file_as_string(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *file = LUAX_REQUIRED_STRING(L, 1);

    const File_System_t *file_system = (const File_System_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));

    File_System_Chunk_t chunk = FSaux_load(file_system, file, FILE_SYSTEM_CHUNK_BLOB);
    if (chunk.type == FILE_SYSTEM_CHUNK_NULL) {
        return luaL_error(L, "can't load file `%s`", file);
    }
    lua_pushlstring(L, chunk.var.blob.ptr, chunk.var.blob.size);
    FSaux_release(chunk);

    return 1;
}

static int file_as_binary(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *file = LUAX_REQUIRED_STRING(L, 1);

    const File_System_t *file_system = (const File_System_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));

    File_System_Chunk_t chunk = FSaux_load(file_system, file, FILE_SYSTEM_CHUNK_BLOB);
    if (chunk.type == FILE_SYSTEM_CHUNK_NULL) {
        return luaL_error(L, "can't load file `%s`", file);
    }
//    lua_pushlstring(L, buffer, size);
    lua_pushnil(L); // TODO: read the file as a Base64 or similar encoded string.
    FSaux_release(chunk); // FIME: useless, Lua's strings can contain bytes.

    return 1;
}
