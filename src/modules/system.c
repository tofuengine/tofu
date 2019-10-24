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

#include "system.h"

#include "../core/luax.h"

#include "../environment.h"
#include "../log.h"

#include <string.h>

typedef struct _System_Class_t {
    const void *bogus;
} System_Class_t;

static int system_time(lua_State *L);
static int system_fps(lua_State *L);
static int system_quit(lua_State *L);
static int system_log(lua_State *L);

static const struct luaL_Reg _system_functions[] = {
    { "time", system_time },
    { "fps", system_fps },
    { "quit", system_quit },
    { "log", system_log },
    { NULL, NULL }
};

static const luaX_Const _system_constants[] = {
    { NULL }
};

int system_loader(lua_State *L)
{
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, NULL, _system_functions, _system_constants, nup, LUAX_CLASS(System_Class_t));
}

static int system_time(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    lua_pushnumber(L, environment->time);

    return 1;
}

static int system_fps(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    lua_pushinteger(L, environment->fps);

    return 1;
}

static int system_quit(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    environment->quit = true;

    return 0;
}

static int system_log(lua_State *L)
{
    int argc = lua_gettop(L);
    lua_getglobal(L, "tostring"); // F
    for (int i = 1; i <= argc; i++) {
        lua_pushvalue(L, -1); // F -> F F
        lua_pushvalue(L, i); // F F -> F F I
        lua_call(L, 1, 1); // F F I -> F R
        const char *s = lua_tostring(L, -1);
        if (s == NULL) {
            return luaL_error(L, "`tostring` must return a string `log`");
        }
        Log_write(LOG_LEVELS_INFO, (i > 1) ? "\t%s" : "<SYSTEM> %s", s);
        lua_pop(L, 1); // F R -> F
    }
    lua_pop(L, 1); // F -> <empty>

    return 0;
}
