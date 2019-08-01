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

const char util_lua[] =
    "foreign class Timer {\n"
    "\n"
    "    construct new(period, repeats, callback) {}\n"
    "\n"
    "    foreign reset()\n"
    "    foreign cancel()\n"
    "\n"
    "}\n"
    "\n"
    "foreign class Math {\n"
    "\n"
    "    static max(a, b) {\n"
    "        if (a > b) {\n"
    "            return a\n"
    "        }\n"
    "        return b\n"
    "    }\n"
    "\n"
    "    static min(a, b) {\n"
    "        if (a < b) {\n"
    "            return a\n"
    "        }\n"
    "        return b\n"
    "    }\n"
    "\n"
    "}\n"
;


/*
http://webcache.googleusercontent.com/search?q=cache:RLoR9dkMeowJ:howtomakeanrpg.com/a/classes-in-lua.html+&cd=4&hl=en&ct=clnk&gl=it
https://hisham.hm/2014/01/02/how-to-write-lua-modules-in-a-post-module-world/
https://www.oreilly.com/library/view/creating-solid-apis/9781491986301/ch01.html
file:///C:/Users/mlizza/Downloads/[Roberto_Ierusalimschy]_Programming_in_Lua(z-lib.org).pdf (page 269)
https://nachtimwald.com/2014/07/12/wrapping-a-c-library-in-lua/
https://www.lua.org/pil/28.5.html
https://stackoverflow.com/questions/16713837/hand-over-global-custom-data-to-lua-implemented-functions
https://stackoverflow.com/questions/29449296/extending-lua-check-number-of-parameters-passed-to-a-function
*/

static const char *util_timer_module = "util.Timer";
static const char *util_timer_metatable = "util.Timer_mt";

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

int luaopen_util_timer(lua_State *L)
{
    luaL_newmetatable(L, util_timer_metatable); /* create metatable */
    lua_pushvalue(L, -1); /* duplicate the metatable */
    lua_setfield(L, -2, "__index"); /* mt.__index = mt */
    luaL_setfuncs(L, util_timer_f, 0); /* register metamethods */
    luaL_newlib(L, util_timer_m); /* create lib table */
    return 1;
}

int luax_preload(lua_State *L, const char *name, lua_CFunction f)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushcfunction(L, f);
    lua_setfield(L, -2, name);
    lua_pop(L, 2);
    return 0;
}

bool util_initialize(lua_State *L)
{
    luax_preload(L, util_timer_module, luaopen_util_timer);
    return true;
}

LUALIB_API lua_Integer luaL_checkfunction (lua_State *L, int arg) {
  if (lua_type(L, arg) != LUA_TFUNCTION)) {
    tag_error(L, arg, LUA_TFUNCTION);
    return -1;
  }
  lua_pushvalue(L, arg);
  return luaL_ref(L, LUA_REGISTRYINDEX);
}


static int util_timer_new(lua_State *L)
{
    if (lua_gettop(L) != 3) {
        return luaL_error(L, "expecting exactly 3 arguments");
    }
    double period = luaL_checknumber(L, 1);
    int repeats = luaL_checkinteger(L, 2);
    int callback = luaL_checkfunction(L, 3); // NOTE! This need to be released when the timer is detached!
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.new() -> %f, %d, %d", period, repeats, callback);
#endif

/*
//Set your userdata as a global
lua_pushlightuserdata(L, environment);
lua_setglobal(L, "environment");
*/

    lua_getglobal(L, "environment");
    Environment_t *environment = (Environment_t *)lua_touserdata(L, -1);  //Get it from the top of the stack
    Timer_Pool_t *timer_pool = environment->timer_pool;

    Timer_Class_t *instance = (Timer_Class_t *)lua_newuserdata(L, sizeof(Timer_Class_t));
    instance->timer_pool = timer_pool;
    instance->timer = TimerPool_allocate(timer_pool, (Timer_Value_t){
            .period = period,
            .repeats = repeats,
            .callback = callback
        });

    luaL_setmetatable(L, util_timer_metatable);

    return 1;
}

#if 0
static int lua_callback(lua_State * L, int ref, EXTRA_ARGS)
{
  lua_rawgeti(L, ref); /* push stored function */
  int nargs = /* Number of EXTRA_ARGS */;
  /* ...push EXTRA_ARGS to stack... */
  /* call function (error checking omitted) */
  lua_pcall(L, nargs,  LUA_MULTRET);
  /* Process results */
}

#endif

static int util_timer_gc(lua_State *L)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.gc()");
#endif

    Timer_Class_t *instance = (Timer_Class_t *)luaL_checkudata(L, 1, util_timer_metatable);

    Log_write(LOG_LEVELS_DEBUG, "<UTIL> finalizing timer #%p", instance->timer);

    lua_unref(L, instance->timer->callback); // TODO: move to the timer gc callback.

    TimerPool_release(instance->timer_pool, instance->timer);

    return 0;
}

static int util_timer_reset(lua_State *L)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.cancel()");
#endif

    Timer_Class_t *instance = (Timer_Class_t *)luaL_checkudata(L, 1, util_timer_metatable);

    TimerPool_reset(instance->timer_pool, instance->timer);

    return 0;
}

static int util_timer_cancel(lua_State *L)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Timer.cancel()");
#endif

    Timer_Class_t *instance = (Timer_Class_t *)luaL_checkudata(L, 1, util_timer_metatable);

    TimerPool_cancel(instance->timer_pool, instance->timer);

    return 0;
}
