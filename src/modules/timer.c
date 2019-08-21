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

static const struct luaL_Reg _timer_functions[] = {
    { "new", timer_new },
    {"__gc", timer_gc },
    { "reset", timer_reset },
    { "cancel", timer_cancel },
    { NULL, NULL }
};

static const luaX_Const _timer_constants[] = {
    { NULL }
};

int timer_loader(lua_State *L)
{
    lua_pushvalue(L, lua_upvalueindex(1)); // Duplicate the upvalue to pass it to the module.
    return luaX_newmodule(L, NULL, _timer_functions, _timer_constants, 1, LUAX_CLASS(Timer_Class_t));
}

static int timer_new(lua_State *L)
{
    luaX_checkcall(L, "nif");
    double period = lua_tonumber(L, 1);
    int repeats = lua_tointeger(L, 2);
    int callback = luaX_tofunction(L, 3); // NOTE! This need to be released when the timer is detached!
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.new() -> %f, %d, %d", period, repeats, callback);
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));
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
    luaX_checkcall(L, "u");
    Timer_Class_t *instance = (Timer_Class_t *)lua_touserdata(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.gc()");
#endif

    Log_write(LOG_LEVELS_DEBUG, "<TIMER> finalizing timer #%p", instance->timer);

    luaL_unref(L, LUA_REGISTRYINDEX, instance->timer->value.callback);

    TimerPool_release(instance->timer_pool, instance->timer);

    return 0;
}

static int timer_reset(lua_State *L)
{
    luaX_checkcall(L, "u");
    Timer_Class_t *instance = (Timer_Class_t *)lua_touserdata(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.cancel()");
#endif

    TimerPool_reset(instance->timer_pool, instance->timer);

    return 0;
}

static int timer_cancel(lua_State *L)
{
    luaX_checkcall(L, "u");
    Timer_Class_t *instance = (Timer_Class_t *)lua_touserdata(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.cancel()");
#endif

    TimerPool_cancel(instance->timer_pool, instance->timer);

    return 0;
}
