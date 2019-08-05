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

#include "util.h"

#include "../core/timerpool.h"

#include "../config.h"
#include "../environment.h"
#include "../log.h"

typedef struct _Timer_Class_t {
    Timer_Pool_t *timer_pool;
    Timer_t *timer;
} Timer_Class_t;

#define NAMESPACE_UTIL_TIMER            "util.Timer"

static const char *util_lua =
    "\n"
;

static int util_timer_new(lua_State *L);
static int util_timer_gc(lua_State *L);
static int util_timer_reset(lua_State *L);
static int util_timer_cancel(lua_State *L);

static const struct luaL_Reg util_timer_f[] = {
    { "new", util_timer_new },
    { NULL, NULL }
};

static const struct luaL_Reg util_timer_m[] = {
    {"__gc", util_timer_gc },
    { "reset", util_timer_reset },
    { "cancel", util_timer_cancel },
    { NULL, NULL }
};

static const luaX_Const util_timer_c[] = {
    { NULL }
};

static int luaopen_util_timer(lua_State *L)
{
    return luaX_newclass(L, util_timer_f, util_timer_m, util_timer_c, LUAX_CLASS(NAMESPACE_UTIL_TIMER));
}

bool util_initialize(lua_State *L)
{
    luaX_preload(L, LUAX_MODULE(NAMESPACE_UTIL_TIMER), luaopen_util_timer);

    if (luaL_dostring(L, util_lua) != 0) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't open script: %s", lua_tostring(L, -1));
        return false;
    }

    return true;
}

static int util_timer_new(lua_State *L)
{
    if (lua_gettop(L) != 3) {
        return luaL_error(L, "<UTIL> timer constructor requires 3 arguments");
    }
    double period = luaL_checknumber(L, 1);
    int repeats = luaL_checkinteger(L, 2);
    int callback = luaX_checkfunction(L, 3); // NOTE! This need to be released when the timer is detached!
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.new() -> %f, %d, %d", period, repeats, callback);
#endif

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");
    Timer_Pool_t *timer_pool = environment->timer_pool;

    Timer_Class_t *instance = (Timer_Class_t *)lua_newuserdata(L, sizeof(Timer_Class_t));
    instance->timer_pool = timer_pool;
    instance->timer = TimerPool_allocate(timer_pool, (Timer_Value_t){
            .period = period,
            .repeats = repeats,
            .callback = callback
        });

    Log_write(LOG_LEVELS_DEBUG, "<UTIL> timer #%p allocated", instance->timer);

    luaL_setmetatable(L, LUAX_CLASS(NAMESPACE_UTIL_TIMER));

    return 1;
}

static int util_timer_gc(lua_State *L)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.gc()");
#endif

    Timer_Class_t *instance = (Timer_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(NAMESPACE_UTIL_TIMER));

    Log_write(LOG_LEVELS_DEBUG, "<UTIL> finalizing timer #%p", instance->timer);

    luaL_unref(L, LUA_REGISTRYINDEX, instance->timer->value.callback); // TODO: move to the timer gc callback.

    TimerPool_release(instance->timer_pool, instance->timer);

    return 0;
}

static int util_timer_reset(lua_State *L)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.cancel()");
#endif

    Timer_Class_t *instance = (Timer_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(NAMESPACE_UTIL_TIMER));

    TimerPool_reset(instance->timer_pool, instance->timer);

    return 0;
}

static int util_timer_cancel(lua_State *L)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.cancel()");
#endif

    Timer_Class_t *instance = (Timer_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(NAMESPACE_UTIL_TIMER));

    TimerPool_cancel(instance->timer_pool, instance->timer);

    return 0;
}
