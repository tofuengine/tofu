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

#include "udt.h"
#include "../core/luax.h"
#include "../environment.h"
#include "../fs.h"
#include "../log.h"

#include <string.h>
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif

#define FILE_MT        "Tofu_File_mt"

static int file_read(lua_State *L);

static const struct luaL_Reg _file_functions[] = {
    { "read", file_read },
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

static int file_read(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
    LUAX_SIGNATURE_END
    const char *file = lua_tostring(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "File.read() -> %s", file);
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    char *result = FS_load_as_string(&environment->fs, file);
    Log_write(LOG_LEVELS_DEBUG, "<FILE> file '%s' loaded at %p", file, result);

    lua_pushstring(L, result);

    free(result);

    return 1;
}
