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
#include <libs/luax.h>
#include <libs/stb.h>
#include <libs/sysinfo.h>
#include <systems/environment.h>
#include <version.h>

#include "udt.h"

#include <time.h>

#define LOG_CONTEXT "system"

#define MAX_DATE_LENGTH 64

static int system_args_0_1t(lua_State *L);
static int system_version_0_3nnn(lua_State *L);
static int system_information_0_1t(lua_State *L);
static int system_clock_0_1n(lua_State *L);
static int system_time_0_1n(lua_State *L);
static int system_date_2SS_1s(lua_State *L);
static int system_fps_0_1n(lua_State *L);
#ifdef __ENGINE_PERFORMANCE_STATISTICS__
static int system_stats_0_4nnnn(lua_State *L);
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */
#ifdef __SYSTEM_HEAP_STATISTICS__
static int system_heap_1S_1n(lua_State *L);
#endif  /* __SYSTEM_HEAP_STATISTICS__ */
static int system_quit_0_0(lua_State *L);

int system_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "args", system_args_0_1t },
            { "version", system_version_0_3nnn },
            { "information", system_information_0_1t },
            { "clock", system_clock_0_1n },
            { "time", system_time_0_1n },
            { "date", system_date_2SS_1s },
            { "fps", system_fps_0_1n },
#ifdef __ENGINE_PERFORMANCE_STATISTICS__
            { "stats", system_stats_0_4nnnn },
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */
#ifdef __SYSTEM_HEAP_STATISTICS__
            { "heap", system_heap_1S_1n },
#endif  /* __SYSTEM_HEAP_STATISTICS__ */
            { "quit", system_quit_0_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, NULL);
}

static int system_args_0_1t(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    lua_newtable(L); // Initially empty.
    size_t count = arrlenu(environment->args);
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

static int system_information_0_1t(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    System_Information_t si;
    bool result = SI_inspect(&si);
    if (!result) {
        return luaL_error(L, "can't get system information");
    }

    lua_createtable(L, 0, 4);
    lua_pushstring(L, si.system);
    lua_setfield(L, -2, "system");
    lua_pushstring(L, si.release);
    lua_setfield(L, -2, "release");
    lua_pushstring(L, si.version);
    lua_setfield(L, -2, "version");
    lua_pushstring(L, si.architecture);
    lua_setfield(L, -2, "architecture");

    return 1;
}

static int system_clock_0_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    lua_pushnumber(L, ((lua_Number)clock()) / (lua_Number)CLOCKS_PER_SEC);

    return 1;
}

static int system_time_0_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    const Environment_State_t *state = Environment_get_state(environment);
    lua_pushnumber(L, (lua_Number)state->time);

    return 1;
}

static int system_date_2SS_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *format = LUAX_OPTIONAL_STRING(L, 1, "%Y-%m-%dT%H:%M:%S");
    const char *timezone = LUAX_OPTIONAL_STRING(L, 2, "local");

    const time_t t = time(NULL);
    const struct tm tm = (timezone[0] == 'g') ? *gmtime(&t) : *localtime(&t); // TODO: use table lookup.

    char date[MAX_DATE_LENGTH] = { 0 };
    size_t length = strftime(date, MAX_DATE_LENGTH, format, &tm);

    lua_pushlstring(L, date, length);

    return 1;
}

static int system_fps_0_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    const Environment_State_t *state = Environment_get_state(environment);
    const Environment_Stats_t *stats = &state->stats;
    lua_pushinteger(L, (lua_Integer)stats->fps);

    return 1;
}

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
static int system_stats_0_4nnnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_ENVIRONMENT));

    const Environment_State_t *state = Environment_get_state(environment);
    const Environment_Stats_t *stats = &state->stats;
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

    const Environment_State_t *state = Environment_get_state(environment);
    const Environment_Stats_t *stats = &state->stats;
    float usage = 0.0f;
    switch (unit[0]) {
        case 'm': { usage = (float)stats->memory_usage / (1024.0f * 1024.0f); } break;
        case 'k': { usage = (float)stats->memory_usage / 1024.0f; } break;
        case 'b': { usage = (float)stats->memory_usage; } break;
    }
    lua_pushnumber(L, (lua_Number)usage);

    return 1;
}
#endif  /* __SYSTEM_HEAP_STATISTICS__ */

static int system_quit_0_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_close(display);

    return 0;
}
