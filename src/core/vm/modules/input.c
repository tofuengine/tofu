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

#include <config.h>
#include <core/environment.h>
#include <core/io/input.h>
#include <libs/log.h>

#include "udt.h"

#include <string.h>

#define INPUT_MT        "Tofu_Input_mt"

static int input_is_key_down(lua_State *L);
static int input_is_key_up(lua_State *L);
static int input_is_key_pressed(lua_State *L);
static int input_is_key_released(lua_State *L);

static const struct luaL_Reg _input_functions[] = {
    { "is_key_down", input_is_key_down },
    { "is_key_up", input_is_key_up },
    { "is_key_pressed", input_is_key_pressed },
    { "is_key_released", input_is_key_released },
    { NULL, NULL }
};

// TODO: implement triggers delay/repeat.
static const luaX_Const _input_constants[] = {
    { "UP", LUA_CT_INTEGER, { .i = INPUT_KEY_UP } },
    { "DOWN", LUA_CT_INTEGER, { .i = INPUT_KEY_DOWN } },
    { "LEFT", LUA_CT_INTEGER, { .i = INPUT_KEY_LEFT } },
    { "RIGHT", LUA_CT_INTEGER, { .i = INPUT_KEY_RIGHT } },
    { "LT", LUA_CT_INTEGER, { .i = INPUT_KEY_LT } },
    { "RT", LUA_CT_INTEGER, { .i = INPUT_KEY_RT } },
    { "Y", LUA_CT_INTEGER, { .i = INPUT_KEY_Y } },
    { "X", LUA_CT_INTEGER, { .i = INPUT_KEY_X } },
    { "B", LUA_CT_INTEGER, { .i = INPUT_KEY_B } },
    { "A", LUA_CT_INTEGER, { .i = INPUT_KEY_A } },
    { "SELECT", LUA_CT_INTEGER, { .i = INPUT_KEY_SELECT } },
    { "START", LUA_CT_INTEGER, { .i = INPUT_KEY_START } },
    { NULL }
};

int input_loader(lua_State *L)
{
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, NULL, _input_functions, _input_constants, nup, INPUT_MT);
}

static int input_is_key_down(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int key = lua_tointeger(L, 1);

    Input_t *input = (Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    bool is_down = (key >= Input_Keys_t_First && key <= Input_Keys_t_Last) ? input->keys[key].state.down : false;

    lua_pushboolean(L, is_down);
    return 1;
}

static int input_is_key_up(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int key = lua_tointeger(L, 1);

    Input_t *input = (Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    bool is_down = (key >= Input_Keys_t_First && key <= Input_Keys_t_Last) ? input->keys[key].state.down : false;

    lua_pushboolean(L, !is_down);
    return 1;
}

static int input_is_key_pressed(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int key = lua_tointeger(L, 1);

    Input_t *input = (Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    bool is_pressed = (key >= Input_Keys_t_First && key <= Input_Keys_t_Last) ? input->keys[key].state.pressed : false;

    lua_pushboolean(L, is_pressed);
    return 1;
}

static int input_is_key_released(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int key = lua_tointeger(L, 1);

    Input_t *input = (Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    bool is_released = (key >= Input_Keys_t_First && key <= Input_Keys_t_Last) ? input->keys[key].state.released : false;

    lua_pushboolean(L, is_released);
    return 1;
}
