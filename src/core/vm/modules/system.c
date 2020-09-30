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

#include "system.h"

#include <config.h>
#include <core/environment.h>
#include <libs/log.h>

#include "udt.h"

#include <string.h>

#define LOG_CONTEXT "system"

static int system_time(lua_State *L);
static int system_fps(lua_State *L);
static int system_quit(lua_State *L);
static int system_info(lua_State *L);
static int system_warning(lua_State *L);
static int system_error(lua_State *L);
static int system_fatal(lua_State *L);

static const struct luaL_Reg _system_functions[] = {
    { "time", system_time },
    { "fps", system_fps },
    { "quit", system_quit },
    { "info", system_info },
    { "warning", system_warning },
    { "error", system_error },
    { "fatal", system_fatal },
    { NULL, NULL }
};

int system_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _system_functions, NULL, nup, NULL);
}

static int system_time(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    lua_pushnumber(L, Environment_get_time(environment));

    return 1;
}

static int system_fps(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    lua_pushnumber(L, Environment_get_fps(environment));

    return 1;
}

static int system_quit(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Environment_t *environment = (Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    Environment_quit(environment);

    return 0;
}

static int log_write(lua_State *L, Log_Levels_t level)
{
    int argc = lua_gettop(L);
    lua_getglobal(L, "tostring"); // F
    for (int i = 1; i <= argc; ++i) {
        lua_pushvalue(L, -1); // F -> F F
        lua_pushvalue(L, i); // F F -> F F I
        lua_call(L, 1, 1); // F F I -> F R
        const char *s = lua_tostring(L, -1);
        if (s == NULL) {
            return luaL_error(L, "`tostring` must return a string `log_write`");
        }
        Log_write(level, LOG_CONTEXT, (i > 1) ? "\t%s" : "%s", s);
        lua_pop(L, 1); // F R -> F
    }
    lua_pop(L, 1); // F -> <empty>

    return 0;
}

static int system_info(lua_State *L)
{
    return log_write(L, LOG_LEVELS_INFO);
}

static int system_warning(lua_State *L)
{
    return log_write(L, LOG_LEVELS_WARNING);
}

static int system_error(lua_State *L)
{
    return log_write(L, LOG_LEVELS_ERROR);
}

static int system_fatal(lua_State *L)
{
    return log_write(L, LOG_LEVELS_FATAL);
}
