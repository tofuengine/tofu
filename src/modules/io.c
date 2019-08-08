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

#include "io.h"

#include "../environment.h"
#include "../file.h"
#include "../log.h"

#include <string.h>

static const char *io_lua =
    "\n"
;

static int io_file_read(lua_State *L);

static const struct luaL_Reg io_file_f[] = {
    { "read", io_file_read },
    { NULL, NULL }
};

static const struct luaL_Reg io_file_m[] = {
    { NULL, NULL }
};

static const luaX_Const io_file_c[] = {
    { NULL }
};

static int luaopen_module(lua_State *L)
{
    lua_newtable(L);

    luaX_newclass(L, io_file_f, io_file_m, io_file_c, "File");
    lua_setfield(L, -2, "File");

    return 1;
}

bool io_initialize(lua_State *L)
{
    luaX_preload(L, "tofu.io", luaopen_module);

    if (luaL_dostring(L, io_lua) != 0) {
        Log_write(LOG_LEVELS_FATAL, "<IO> can't open script: %s", lua_tostring(L, -1));
        return false;
    }

    return true;
}

static int io_file_read(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<IO> function requires 1 argument");
    }
    const char *file = luaL_checkstring(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "File.read() -> %s", file);
#endif
    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file + 2);

    const char *result = file_load_as_string(pathfile, "rt");
    Log_write(LOG_LEVELS_DEBUG, "<IO> file '%s' loaded at %p", pathfile, result);

    lua_pushstring(L, result);

    return 1;
}
