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

#include "timer.h"

#include "../core/luax.h"
#include "../core/timerpool.h"

#include "../config.h"
#include "../environment.h"
#include "../log.h"

typedef struct _Timer_Class_t {
    Timer_Pool_t *timer_pool;
    Timer_t *timer;
} Timer_Class_t;

static int timer_new(lua_State *L);
static int timer_gc(lua_State *L);
static int timer_reset(lua_State *L);
static int timer_cancel(lua_State *L);

static const struct luaL_Reg timer_functions[] = {
    { "new", timer_new },
    {"__gc", timer_gc },
    { "reset", timer_reset },
    { "cancel", timer_cancel },
    { NULL, NULL }
};

static const luaX_Const timer_constants[] = {
    { NULL }
};

int timer_loader(lua_State *L)
{
    return luaX_newmodule(L, NULL, timer_functions, timer_constants, LUAX_CLASS(Timer_Class_t));
}

static int timer_new(lua_State *L)
{
    if (lua_gettop(L) != 3) {
        return luaL_error(L, "<TIMER> timer constructor requires 3 arguments");
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

    Log_write(LOG_LEVELS_DEBUG, "<TIMER> timer #%p allocated", instance->timer);

    luaL_setmetatable(L, LUAX_CLASS(Timer_Class_t));

    return 1;
}

static int timer_gc(lua_State *L)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.gc()");
#endif

    Timer_Class_t *instance = (Timer_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Timer_Class_t));

    Log_write(LOG_LEVELS_DEBUG, "<TIMER> finalizing timer #%p", instance->timer);

    luaL_unref(L, LUA_REGISTRYINDEX, instance->timer->value.callback); // TODO: move to the timer gc callback.

    TimerPool_release(instance->timer_pool, instance->timer);

    return 0;
}

static int timer_reset(lua_State *L)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.cancel()");
#endif

    Timer_Class_t *instance = (Timer_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Timer_Class_t));

    TimerPool_reset(instance->timer_pool, instance->timer);

    return 0;
}

static int timer_cancel(lua_State *L)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.cancel()");
#endif

    Timer_Class_t *instance = (Timer_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Timer_Class_t));

    TimerPool_cancel(instance->timer_pool, instance->timer);

    return 0;
}
