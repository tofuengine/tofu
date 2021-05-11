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

#include "system.h"

#include <config.h>
#include <core/environment.h>
#include <libs/log.h>
#include <libs/luax.h>
#include <libs/stb.h>
#include <version.h>

#include "udt.h"

#define LOG_CONTEXT "system"

static int system_args_0_1t(lua_State *L);
static int system_version_0_3nnn(lua_State *L);
static int system_time_0_1n(lua_State *L);
static int system_fps_0_1n(lua_State *L);
#ifdef __ENGINE_PERFORMANCE_STATISTICS__
static int system_stats_0_4nnnn(lua_State *L);
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */
#ifdef __SYSTEM_HEAP_STATISTICS__
static int system_heap_1S_1n(lua_State *L);
#endif  /* __SYSTEM_HEAP_STATISTICS__ */
#ifdef __DISPLAY_FOCUS_SUPPORT__
static int system_is_active_0_1b(lua_State *L);
#endif  /* __DISPLAY_FOCUS_SUPPORT__ */
static int system_quit_0_0(lua_State *L);
static int system_info_v_0(lua_State *L);
static int system_warning_v_0(lua_State *L);
static int system_error_v_0(lua_State *L);
static int system_fatal_v_0(lua_State *L);

static const struct luaL_Reg _system_functions[] = {
    { "args", system_args_0_1t },
    { "version", system_version_0_3nnn },
    { "time", system_time_0_1n },
    { "fps", system_fps_0_1n },
#ifdef __ENGINE_PERFORMANCE_STATISTICS__
    { "stats", system_stats_0_4nnnn },
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */
#ifdef __SYSTEM_HEAP_STATISTICS__
    { "heap", system_heap_1S_1n },
#endif  /* __SYSTEM_HEAP_STATISTICS__ */
#ifdef __DISPLAY_FOCUS_SUPPORT__
    { "is_active", system_is_active_0_1b },
#endif  /* __DISPLAY_FOCUS_SUPPORT__ */
    { "quit", system_quit_0_0 },
    { "info", system_info_v_0 },
    { "warning", system_warning_v_0 },
    { "error", system_error_v_0 },
    { "fatal", system_fatal_v_0 },
    { NULL, NULL }
};

int system_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _system_functions, NULL, nup, NULL);
}

static int system_args_0_1t(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    lua_createtable(L, 0, 0); // Initially empty.
    size_t count = arrlen(environment->args);
    for (size_t i = 0; i < count; ++i) {
        lua_pushstring(L, environment->args[i]);
        lua_rawseti(L, -2, (lua_Integer)(i + 1));
    }

    return 1;
}

static int system_version_0_3nnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    lua_pushinteger(L, (lua_Integer)TOFU_VERSION_MAJOR);
    lua_pushinteger(L, (lua_Integer)TOFU_VERSION_MINOR);
    lua_pushinteger(L, (lua_Integer)TOFU_VERSION_REVISION);

    return 3;
}

static int system_time_0_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    lua_pushnumber(L, (lua_Number)Environment_get_time(environment));

    return 1;
}

static int system_fps_0_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    const Environment_Stats_t *stats = Environment_get_stats(environment);
    lua_pushnumber(L, (lua_Number)stats->fps);

    return 1;
}

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
static int system_stats_0_4nnnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    const Environment_Stats_t *stats = Environment_get_stats(environment);
    lua_pushnumber(L, (lua_Number)stats->times[0]);
    lua_pushnumber(L, (lua_Number)stats->times[1]);
    lua_pushnumber(L, (lua_Number)stats->times[2]);
    lua_pushnumber(L, (lua_Number)stats->times[3]);

    return 4;
}
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */

#ifdef __SYSTEM_HEAP_STATISTICS__
static int system_heap_1S_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *unit = LUAX_OPTIONAL_STRING(L, 1, "b");

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    const Environment_Heap_t *heap = Environment_get_heap(environment);
    size_t usage;
    switch (unit[0]) {
        case 'm': { usage = heap->memory_usage / (1024 * 1024); } break;
        case 'k': { usage = heap->memory_usage / 1024; } break;
        case 'b': { usage = heap->memory_usage; } break;
    }
    lua_pushnumber(L, (lua_Number)usage);

    return 1;
}
#endif  /* __SYSTEM_HEAP_STATISTICS__ */

#ifdef __DISPLAY_FOCUS_SUPPORT__
static int system_is_active_0_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    lua_pushboolean(L, Environment_is_active(environment));

    return 1;
}
#endif  /* __DISPLAY_FOCUS_SUPPORT__ */

static int system_quit_0_0(lua_State *L)
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

static int system_info_v_0(lua_State *L)
{
    return log_write(L, LOG_LEVELS_INFO);
}

static int system_warning_v_0(lua_State *L)
{
    return log_write(L, LOG_LEVELS_WARNING);
}

static int system_error_v_0(lua_State *L)
{
    return log_write(L, LOG_LEVELS_ERROR);
}

static int system_fatal_v_0(lua_State *L)
{
    return log_write(L, LOG_LEVELS_FATAL);
}
