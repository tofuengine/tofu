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

#include "environment.h"

#include "../core/luax.h"

#include "../environment.h"
#include "../log.h"

#include <string.h>

typedef struct _Environment_Class_t {
} Environment_Class_t;

static int environment_fps(lua_State *L);
static int environment_quit(lua_State *L);

static const struct luaL_Reg environment_f[] = {
    { "fps", environment_fps },
    { "quit", environment_quit },
    { NULL, NULL }
};

static const struct luaL_Reg environment_m[] = {
    { NULL, NULL }
};

static const luaX_Const environment_c[] = {
    { NULL }
};

const char environment_script[] =
    "\n"
;

int environment_loader(lua_State *L)
{
    return luaX_newclass(L, environment_f, environment_m, environment_c, LUAX_CLASS(Environment_Class_t));
}

static int environment_fps(lua_State *L)
{
    if (lua_gettop(L) != 0) {
        return luaL_error(L, "<ENVIRONMENT> function requires 0 argument");
    }

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    lua_pushinteger(L, environment->fps);
    return 1;
}

static int environment_quit(lua_State *L)
{
    if (lua_gettop(L) != 0) {
        return luaL_error(L, "<ENVIRONMENT> function requires 0 argument");
    }

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    environment->should_close = true;

    return 0;
}
