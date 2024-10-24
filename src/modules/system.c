/*
 * MIT License
 *
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "internal/udt.h"

#include <core/config.h>
#include <core/version.h>
#include <libs/stb.h>
#include <libs/sysinfo.h>
#include <systems/environment.h>

#include <time.h>

#define _MAX_DATE_LENGTH 64

static int system_version_0_3nnn(lua_State *L);
static int system_information_0_1t(lua_State *L);
static int system_clock_0_1n(lua_State *L);
static int system_time_0_1n(lua_State *L);
static int system_date_2SS_1s(lua_State *L);
static int system_fps_0_1n(lua_State *L);
#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
static int system_stats_0_4nnnn(lua_State *L);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
#if defined(TOFU_ENGINE_HEAP_STATISTICS)
static int system_heap_1S_1n(lua_State *L);
#endif  /* TOFU_ENGINE_HEAP_STATISTICS */
static int system_active_0_1b(lua_State *L);
static int system_quit_0_0(lua_State *L);

int system_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- accessors --
            { "version", system_version_0_3nnn },
            { "information", system_information_0_1t },
            { "clock", system_clock_0_1n },
            { "time", system_time_0_1n },
            { "date", system_date_2SS_1s },
            { "fps", system_fps_0_1n },
#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
            { "stats", system_stats_0_4nnnn },
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
#if defined(TOFU_ENGINE_HEAP_STATISTICS)
            { "heap", system_heap_1S_1n },
#endif  /* TOFU_ENGINE_HEAP_STATISTICS */
            { "active", system_active_0_1b },
            // -- operations --
            { "quit", system_quit_0_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
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

    SysInfo_Data_t si;
    bool result = SysInfo_inspect(&si);
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

    const Environment_t *environment = (const Environment_t *)udt_get_userdata(L, USERDATA_ENVIRONMENT);

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

    char date[_MAX_DATE_LENGTH] = { 0 };
    size_t length = strftime(date, _MAX_DATE_LENGTH, format, &tm);

    lua_pushlstring(L, date, length);

    return 1;
}

static int system_fps_0_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)udt_get_userdata(L, USERDATA_ENVIRONMENT);

    const Environment_State_t *state = Environment_get_state(environment);
    const Environment_Stats_t *stats = &state->stats;
    lua_pushinteger(L, (lua_Integer)stats->fps);

    return 1;
}

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
static int system_stats_0_4nnnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)udt_get_userdata(L, USERDATA_ENVIRONMENT);

    const Environment_State_t *state = Environment_get_state(environment);
    const Environment_Stats_t *stats = &state->stats;
    lua_pushnumber(L, (lua_Number)stats->times[0]);
    lua_pushnumber(L, (lua_Number)stats->times[1]);
    lua_pushnumber(L, (lua_Number)stats->times[2]);
    lua_pushnumber(L, (lua_Number)stats->times[3]);
    lua_pushnumber(L, (lua_Number)stats->times[4]);

    return 5;
}
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

#if defined(TOFU_ENGINE_HEAP_STATISTICS)
static int system_heap_1S_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *unit = LUAX_OPTIONAL_STRING(L, 1, "b");

    const Environment_t *environment = (const Environment_t *)udt_get_userdata(L, USERDATA_ENVIRONMENT);

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
#endif  /* TOFU_ENGINE_HEAP_STATISTICS */

static int system_active_0_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Environment_t *environment = (const Environment_t *)udt_get_userdata(L, USERDATA_ENVIRONMENT);

    const Environment_State_t *state = Environment_get_state(environment);
    lua_pushboolean(L, state->is_active);

    return 1;
}

static int system_quit_0_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Display_t *display = (Display_t *)udt_get_userdata(L, USERDATA_DISPLAY);

    Display_close(display);

    return 0;
}
