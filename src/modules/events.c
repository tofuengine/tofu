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

#include "events.h"

#include "../environment.h"
#include "../log.h"

#include <string.h>

static const char *events_lua =
    "\n"
;

static int events_environment_fps(lua_State *L);
static int events_environment_quit(lua_State *L);
static int events_input_is_key_down(lua_State *L);
static int events_input_is_key_up(lua_State *L);
static int events_input_is_key_pressed(lua_State *L);
static int events_input_is_key_released(lua_State *L);

static const struct luaL_Reg events_environment_f[] = {
    { "fps", events_environment_fps },
    { "quit", events_environment_quit },
    { NULL, NULL }
};

static const struct luaL_Reg events_environment_m[] = {
    { NULL, NULL }
};

static const luaX_Const events_environment_c[] = {
    { NULL }
};

static const struct luaL_Reg events_input_f[] = {
    { "is_key_down", events_input_is_key_down },
    { "is_key_up", events_input_is_key_up },
    { "is_key_pressed", events_input_is_key_pressed },
    { "is_key_released", events_input_is_key_released },
    { NULL, NULL }
};

static const struct luaL_Reg events_input_m[] = {
    { NULL, NULL }
};

static const luaX_Const events_input_c[] = {
    { "UP", LUA_CT_INTEGER, { .i = 265 } },
    { "DOWN", LUA_CT_INTEGER, { .i = 264 } },
    { "LEFT", LUA_CT_INTEGER, { .i = 263 } },
    { "RIGHT", LUA_CT_INTEGER, { .i = 262 } },
    { "SPACE", LUA_CT_INTEGER, { .i = 32 } },
    { "ENTER", LUA_CT_INTEGER, { .i = 257 } },
    { "ESCAPE", LUA_CT_INTEGER, { .i = 256 } },
    { "Z", LUA_CT_INTEGER, { .i = 90 } },
    { "X", LUA_CT_INTEGER, { .i = 88 } },
    { "Q", LUA_CT_INTEGER, { .i = 81 } },
    { NULL }
};

static int luaopen_events(lua_State *L)
{
    lua_newtable(L);

    luaX_newclass(L, events_environment_f, events_environment_m, events_environment_c, "Environment");
    lua_setfield(L, -2, "Environment");

    luaX_newclass(L, events_input_f, events_input_m, events_input_c, "Input");
    lua_setfield(L, -2, "Input");

    return 1;
}

bool events_initialize(lua_State *L)
{
    luaX_preload(L, "events", luaopen_events);

    if (luaL_dostring(L, events_lua) != 0) {
        Log_write(LOG_LEVELS_FATAL, "<EVENTS> can't open script: %s", lua_tostring(L, -1));
        return false;
    }

    return true;
}

static int events_environment_fps(lua_State *L)
{
    if (lua_gettop(L) != 0) {
        return luaL_error(L, "<EVENTS> function requires 0 argument");
    }

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    lua_pushinteger(L, environment->fps);
    return 1;
}

static int events_environment_quit(lua_State *L)
{
    if (lua_gettop(L) != 0) {
        return luaL_error(L, "<EVENTS> function requires 0 argument");
    }

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    environment->should_close = true;

    return 0;
}

static int events_input_is_key_down(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<EVENTS> function requires 1 argument");
    }
    int key = luaL_checkinteger(L, 1);

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    bool is_down = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].down : false;

    lua_pushboolean(L, is_down);
    return 1;
}

static int events_input_is_key_up(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<EVENTS> function requires 1 argument");
    }
    int key = luaL_checkinteger(L, 1);

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    bool is_down = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].down : false;

    lua_pushboolean(L, !is_down);
    return 1;
}

static int events_input_is_key_pressed(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<EVENTS> function requires 1 argument");
    }
    int key = luaL_checkinteger(L, 1);

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    bool is_pressed = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].pressed : false;

    lua_pushboolean(L, is_pressed);
    return 1;
}

static int events_input_is_key_released(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<EVENTS> function requires 1 argument");
    }
    int key = luaL_checkinteger(L, 1);

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    bool is_released = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].released : false;

    lua_pushboolean(L, is_released);
    return 1;
}
