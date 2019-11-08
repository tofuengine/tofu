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

#include <config.h>
#include <core/vm/interpreter.h>
#include <libs/log.h>

#include "udt.h"

#define TIMER_MT    "Tofu_Timer_mt"

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
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, NULL, _timer_functions, _timer_constants, nup, TIMER_MT);
}

static int timer_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isfunction)
    LUAX_SIGNATURE_END
    float period = lua_tonumber(L, 1);
    int repeats = lua_tointeger(L, 2);
    luaX_Reference callback = luaX_tofunction(L, 3);

    Interpreter_t *interpreter = (Interpreter_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INTERPRETER));

    Timer_Class_t *instance = (Timer_Class_t *)lua_newuserdata(L, sizeof(Timer_Class_t));
    *instance = (Timer_Class_t){
            .callback = callback,
            .timer = TimerPool_allocate(&interpreter->timer_pool, period, repeats, BUNDLE_FROM_INT(callback))
        };
    Log_write(LOG_LEVELS_DEBUG, "<TIMER> timer #%p allocated (pool-entry #%p)", instance, instance->timer);

    luaL_setmetatable(L, TIMER_MT);

    return 1;
}

static int timer_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Timer_Class_t *instance = (Timer_Class_t *)lua_touserdata(L, 1);

    Log_write(LOG_LEVELS_DEBUG, "<TIMER> finalizing timer #%p (pool-entry #%p)", instance, instance->timer);

    TimerPool_release(instance->timer); // Mark the entry as finalized.

    luaL_unref(L, LUA_REGISTRYINDEX, instance->callback);

    return 0;
}

static int timer_reset(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Timer_Class_t *instance = (Timer_Class_t *)lua_touserdata(L, 1);

    TimerPool_reset(instance->timer);

    return 0;
}

static int timer_cancel(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Timer_Class_t *instance = (Timer_Class_t *)lua_touserdata(L, 1);

    TimerPool_cancel(instance->timer);

    return 0;
}
