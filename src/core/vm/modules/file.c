/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "file.h"

#include <config.h>
#include <core/vm/interpreter.h>
#include <libs/log.h>

#include "udt.h"

#include <string.h>
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif

#define FILE_MT        "Tofu_File_mt"

static int file_as_string(lua_State *L);
static int file_as_binary(lua_State *L);

static const struct luaL_Reg _file_functions[] = {
    { "as_string", file_as_string },
    { "as_binary", file_as_binary },
    { NULL, NULL }
};

static const luaX_Const _file_constants[] = {
    { NULL }
};

int file_loader(lua_State *L)
{
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, NULL, _file_functions, _file_constants, nup, FILE_MT);
}

static int file_as_string(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
    LUAX_SIGNATURE_END
    const char *file = lua_tostring(L, 1);

    Interpreter_t *interpreter = (Interpreter_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INTERPRETER));

    size_t size;
    char *buffer = FS_load_as_string(&interpreter->file_system, file, &size);
    if (!buffer) {
        luaL_error(L, "<FILE> can't load file '%s'", file);
    }
    lua_pushlstring(L, buffer, size);
    free(buffer);

    return 1;
}

static int file_as_binary(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
    LUAX_SIGNATURE_END
    const char *file = lua_tostring(L, 1);

    Interpreter_t *interpreter = (Interpreter_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INTERPRETER));

    size_t size;
    void *buffer = FS_load_as_binary(&interpreter->file_system, file, &size);
    if (!buffer) {
        luaL_error(L, "<FILE> can't load file '%s'", file);
    }
//    lua_pushlstring(L, buffer, size);
    lua_pushnil(L); // TODO: read the file as a Base64 or similar encoded string.
    free(buffer);

    return 1;
}
