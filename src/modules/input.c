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

#include "input.h"

#include "../core/luax.h"

#include "../environment.h"
#include "../log.h"

#include <string.h>

typedef struct _Input_Class_t {
} Input_Class_t;

static int input_is_key_down(lua_State *L);
static int input_is_key_up(lua_State *L);
static int input_is_key_pressed(lua_State *L);
static int input_is_key_released(lua_State *L);

static const struct luaL_Reg input_f[] = {
    { "is_key_down", input_is_key_down },
    { "is_key_up", input_is_key_up },
    { "is_key_pressed", input_is_key_pressed },
    { "is_key_released", input_is_key_released },
    { NULL, NULL }
};

static const struct luaL_Reg input_m[] = {
    { NULL, NULL }
};

static const luaX_Const input_c[] = {
    { "UP", LUA_CT_INTEGER, { .i = 0 } },
    { "DOWN", LUA_CT_INTEGER, { .i = 1 } },
    { "LEFT", LUA_CT_INTEGER, { .i = 2 } },
    { "RIGHT", LUA_CT_INTEGER, { .i = 3 } },
    { "Y", LUA_CT_INTEGER, { .i = 4 } },
    { "X", LUA_CT_INTEGER, { .i = 5 } },
    { "B", LUA_CT_INTEGER, { .i = 6 } },
    { "A", LUA_CT_INTEGER, { .i = 7 } },
    { "SELECT", LUA_CT_INTEGER, { .i = 8 } },
    { "START", LUA_CT_INTEGER, { .i = 9 } },
    { NULL }
};

const char input_script[] =
    "\n"
;

int input_loader(lua_State *L)
{
    return luaX_newclass(L, input_f, input_m, input_c, LUAX_CLASS(Input_Class_t));
}

static int input_is_key_down(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<INPUT> function requires 1 argument");
    }
    int key = luaL_checkinteger(L, 1);

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    bool is_down = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].down : false;

    lua_pushboolean(L, is_down);
    return 1;
}

static int input_is_key_up(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<INPUT> function requires 1 argument");
    }
    int key = luaL_checkinteger(L, 1);

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    bool is_down = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].down : false;

    lua_pushboolean(L, !is_down);
    return 1;
}

static int input_is_key_pressed(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<INPUT> function requires 1 argument");
    }
    int key = luaL_checkinteger(L, 1);

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    bool is_pressed = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].pressed : false;

    lua_pushboolean(L, is_pressed);
    return 1;
}

static int input_is_key_released(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<INPUT> function requires 1 argument");
    }
    int key = luaL_checkinteger(L, 1);

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    bool is_released = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].released : false;

    lua_pushboolean(L, is_released);
    return 1;
}
