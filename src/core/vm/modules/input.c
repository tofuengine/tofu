/*
 * Copyright (c) 2019-2020 by Marco Lizza (marco.lizza@gmail.com)
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

#include "udt.h"

#include <string.h>

#define INPUT_MT        "Tofu_Input_mt"

static int input_is_down(lua_State *L);
static int input_is_up(lua_State *L);
static int input_is_pressed(lua_State *L);
static int input_is_released(lua_State *L);
static int input_auto_repeat(lua_State *L);
static int input_cursor(lua_State *L);
static int input_cursor_area(lua_State *L);
static int input_stick(lua_State *L);
static int input_triggers(lua_State *L);

static const struct luaL_Reg _input_functions[] = {
    { "is_down", input_is_down },
    { "is_up", input_is_up },
    { "is_pressed", input_is_pressed },
    { "is_released", input_is_released },
    { "auto_repeat", input_auto_repeat },
    { "cursor", input_cursor },
    { "cursor_area", input_cursor_area },
    { "stick", input_stick },
    { "triggers", input_triggers },
    { NULL, NULL }
};

static const luaX_Const _input_constants[] = {
    { "UP", LUA_CT_INTEGER, { .i = INPUT_BUTTON_UP } },
    { "DOWN", LUA_CT_INTEGER, { .i = INPUT_BUTTON_DOWN } },
    { "LEFT", LUA_CT_INTEGER, { .i = INPUT_BUTTON_LEFT } },
    { "RIGHT", LUA_CT_INTEGER, { .i = INPUT_BUTTON_RIGHT } },
    { "LT", LUA_CT_INTEGER, { .i = INPUT_BUTTON_LT } },
    { "RT", LUA_CT_INTEGER, { .i = INPUT_BUTTON_RT } },
    { "Y", LUA_CT_INTEGER, { .i = INPUT_BUTTON_Y } },
    { "X", LUA_CT_INTEGER, { .i = INPUT_BUTTON_X } },
    { "B", LUA_CT_INTEGER, { .i = INPUT_BUTTON_B } },
    { "A", LUA_CT_INTEGER, { .i = INPUT_BUTTON_A } },
    { "SELECT", LUA_CT_INTEGER, { .i = INPUT_BUTTON_SELECT } },
    { "START", LUA_CT_INTEGER, { .i = INPUT_BUTTON_START } },
    { "RESET", LUA_CT_INTEGER, { .i = INPUT_BUTTON_RESET } },
    { "MOUSE_LEFT", LUA_CT_INTEGER, { .i = INPUT_BUTTON_MOUSE_LEFT } },
    { "MOUSE_MIDDLE", LUA_CT_INTEGER, { .i = INPUT_BUTTON_MOUSE_MIDDLE } },
    { "MOUSE_RIGHT", LUA_CT_INTEGER, { .i = INPUT_BUTTON_MOUSE_RIGHT } },
    { NULL }
};

int input_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _input_functions, _input_constants, nup, INPUT_MT);
}

static int input_is_down(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int button = lua_tointeger(L, 1);

    const Input_t *input = (const Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_State_t *state = &input->state;
    bool is_down = (button >= Input_Buttons_t_First && button <= Input_Buttons_t_Last) ? state->buttons[button].state.down : false;

    lua_pushboolean(L, is_down);
    return 1;
}

static int input_is_up(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int button = lua_tointeger(L, 1);

    const Input_t *input = (const Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_State_t *state = &input->state;
    bool is_down = (button >= Input_Buttons_t_First && button <= Input_Buttons_t_Last) ? state->buttons[button].state.down : false;

    lua_pushboolean(L, !is_down);
    return 1;
}

static int input_is_pressed(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int button = lua_tointeger(L, 1);

    const Input_t *input = (const Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_State_t *state = &input->state;
    bool is_pressed = (button >= Input_Buttons_t_First && button <= Input_Buttons_t_Last) ? state->buttons[button].state.pressed : false;

    lua_pushboolean(L, is_pressed);
    return 1;
}

static int input_is_released(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int button = lua_tointeger(L, 1);

    const Input_t *input = (const Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_State_t *state = &input->state;
    bool is_released = (button >= Input_Buttons_t_First && button <= Input_Buttons_t_Last) ? state->buttons[button].state.released : false;

    lua_pushboolean(L, is_released);
    return 1;
}

static int input_auto_repeat1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int button = lua_tointeger(L, 1);

    Input_t *input = (Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    if (button >= Input_Buttons_t_First && button <= Input_Buttons_t_Last) {
        Input_auto_repeat(input, button, 0.0f);
    }

    return 0;
}

static int input_auto_repeat2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int button = lua_tointeger(L, 1);
    float period = lua_tonumber(L, 2);

    Input_t *input = (Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    if (button >= Input_Buttons_t_First && button <= Input_Buttons_t_Last) {
        Input_auto_repeat(input, button, period);
    }

    return 0;
}

static int input_auto_repeat(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, input_auto_repeat1)
        LUAX_OVERLOAD_ARITY(2, input_auto_repeat2)
    LUAX_OVERLOAD_END
}

static int input_cursor0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END

    const Input_t *input = (const Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_State_t *state = &input->state;
    lua_pushnumber(L, state->cursor.x);
    lua_pushnumber(L, state->cursor.y);

    return 2;
}

static int input_cursor2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);

    Input_t *input = (Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    Input_State_t *state = &input->state;
    state->cursor.x = x;
    state->cursor.y = y;

    return 0;
}

static int input_cursor(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, input_cursor0)
        LUAX_OVERLOAD_ARITY(2, input_cursor2)
    LUAX_OVERLOAD_END
}

static int input_cursor_area(lua_State *L) // TODO: rename to `region`?
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);
    size_t width = (size_t)lua_tointeger(L, 3);
    size_t height = (size_t)lua_tointeger(L, 4);

    Input_t *input = (Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    Input_State_t *state = &input->state;
    state->cursor.area.x0 = x;
    state->cursor.area.y0 = y;
    state->cursor.area.x1 = x + width - 1;
    state->cursor.area.y1 = y + height - 1;
//     = (GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height };

    return 0;
}

static int input_stick(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = lua_tostring(L, 1);

    const Input_t *input = (const Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_State_t *state = &input->state;
    const Input_Sticks_t stick = id[0] == 'l' ? INPUT_STICK_LEFT : INPUT_STICK_RIGHT;
    lua_pushnumber(L, state->sticks[stick].x);
    lua_pushnumber(L, state->sticks[stick].y);
    lua_pushnumber(L, state->sticks[stick].angle);
    lua_pushnumber(L, state->sticks[stick].magnitude);

    return 4;
}

static int input_triggers(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END

    const Input_t *input = (const Input_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_State_t *state = &input->state;
    lua_pushnumber(L, state->triggers.left);
    lua_pushnumber(L, state->triggers.right);

    return 2;
}
