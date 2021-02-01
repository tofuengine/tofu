/*
 * MIT License
 * 
 * Copyright (c) 2019-2021 Marco Lizza
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

#include "input.h"

#include <config.h>
#include <core/io/input.h>
#include <libs/map.h>

#include "udt.h"

#ifndef countof
  #define countof(a)  (sizeof((a)) / sizeof((a)[0]))
#endif

static int input_is_down(lua_State *L);
static int input_is_up(lua_State *L);
static int input_is_pressed(lua_State *L);
static int input_is_released(lua_State *L);
static int input_auto_repeat(lua_State *L);
static int input_cursor(lua_State *L);
static int input_cursor_area(lua_State *L);
static int input_stick(lua_State *L);
static int input_triggers(lua_State *L);
static int input_mode(lua_State *L);

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
    { "mode", input_mode },
    { NULL, NULL }
};

int input_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _input_functions, NULL, nup, NULL);
}

static const Map_Entry_t _buttons[Input_Buttons_t_CountOf] = { // Need to be sorted for `bsearch()`
    { "a", INPUT_BUTTON_A },
    { "b", INPUT_BUTTON_B },
    { "down", INPUT_BUTTON_DOWN },
    { "lb", INPUT_BUTTON_LB },
    { "left", INPUT_BUTTON_LEFT },
    { "lt", INPUT_BUTTON_LT },
    { "quit", INPUT_BUTTON_QUIT },
    { "rb", INPUT_BUTTON_RB },
    { "right", INPUT_BUTTON_RIGHT },
    { "rt", INPUT_BUTTON_RT },
    { "select", INPUT_BUTTON_SELECT },
    { "start", INPUT_BUTTON_START },
    { "up", INPUT_BUTTON_UP },
    { "x", INPUT_BUTTON_X },
    { "y", INPUT_BUTTON_Y },
    { NULL, 0 },
    { NULL, 0 }
};

static const Map_Entry_t _sticks[Input_Sticks_t_CountOf] = { // Ditto.
    { "left", INPUT_STICK_LEFT },
    { "right", INPUT_STICK_RIGHT }
};

static const Map_Entry_t _modes[] = { // Ditto.
    { "gamepad", INPUT_MODE_GAMEPAD },
    { "keyboard", INPUT_MODE_KEYBOARD },
    { "mouse", INPUT_MODE_MOUSE }
};

static int input_is_down(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, Input_get_button(input, (Input_Buttons_t)entry->value)->down);

    return 1;
}

static int input_is_up(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, !Input_get_button(input, (Input_Buttons_t)entry->value)->down);

    return 1;
}

static int input_is_pressed(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, Input_get_button(input, (Input_Buttons_t)entry->value)->pressed);

    return 1;
}

static int input_is_released(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, Input_get_button(input, (Input_Buttons_t)entry->value)->released);

    return 1;
}

static int input_auto_repeat1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find(L, id, _buttons, Input_Buttons_t_CountOf);
    float period = Input_get_auto_repeat(input, (Input_Buttons_t)entry->value);
    lua_pushnumber(L, (lua_Number)period);

    return 1;
}

static int input_auto_repeat2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);
    float period = LUAX_NUMBER(L, 2);

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find(L, id, _buttons, Input_Buttons_t_CountOf);
    Input_set_auto_repeat(input, (Input_Buttons_t)entry->value, period);

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
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_Cursor_t *cursor = Input_get_cursor(input);
    lua_pushnumber(L, (lua_Number)cursor->x);
    lua_pushnumber(L, (lua_Number)cursor->y);

    return 2;
}

static int input_cursor2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float x = LUAX_NUMBER(L, 1);
    float y = LUAX_NUMBER(L, 2);

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    Input_set_cursor_position(input, x, y);

    return 0;
}

static int input_cursor(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, input_cursor0)
        LUAX_OVERLOAD_ARITY(2, input_cursor2)
    LUAX_OVERLOAD_END
}

static int input_cursor_area0(lua_State *L) 
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_Cursor_t *cursor = Input_get_cursor(input);
    lua_pushnumber(L, (lua_Number)cursor->area.x0);
    lua_pushnumber(L, (lua_Number)cursor->area.y0);
    lua_pushnumber(L, (lua_Number)(cursor->area.x1 - cursor->area.x0 + 1));
    lua_pushnumber(L, (lua_Number)(cursor->area.y1 - cursor->area.y0 + 1));

    return 4;
}

static int input_cursor_area4(lua_State *L) 
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int x = LUAX_INTEGER(L, 1);
    int y = LUAX_INTEGER(L, 2);
    size_t width = (size_t)LUAX_INTEGER(L, 3);
    size_t height = (size_t)LUAX_INTEGER(L, 4);

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    Input_set_cursor_area(input, x, y, x + width - 1, y + height - 1);

    return 0;
}

static int input_cursor_area(lua_State *L)// TODO: rename to `region`?
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, input_cursor_area0)
        LUAX_OVERLOAD_ARITY(4, input_cursor_area4)
    LUAX_OVERLOAD_END
}

static int input_stick(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find(L, id, _sticks, Input_Sticks_t_CountOf);
    const Input_Stick_t *stick = Input_get_stick(input, (Input_Sticks_t)entry->value);
    lua_pushnumber(L, (lua_Number)stick->x);
    lua_pushnumber(L, (lua_Number)stick->y);
    lua_pushnumber(L, (lua_Number)stick->angle);
    lua_pushnumber(L, (lua_Number)stick->magnitude);

    return 4;
}

static int input_triggers(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_Triggers_t *triggers = Input_get_triggers(input);
    lua_pushnumber(L, (lua_Number)triggers->left);
    lua_pushnumber(L, (lua_Number)triggers->right);

    return 2;
}

static int input_mode0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    int mode = Input_get_mode(input);

    lua_createtable(L, 0, 0); // Initially empty.
    for (size_t i = 0; i < countof(_modes); ++i) {
        if (mode & _modes[i].value) {
            lua_pushstring(L, _modes[i].key);
            lua_rawseti(L, -2, (lua_Integer)(i + 1));
        }
    }

    return 1;
}

static int input_mode1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    int mode = INPUT_MODE_NONE;

    lua_pushnil(L);
    while (lua_next(L, 1)) {
//        int index = LUAX_INTEGER(L, -2);
        const char *id = LUAX_STRING(L, -1);

        const Map_Entry_t *entry = map_find(L, id, _modes, countof(_modes));
        mode |= entry->value;

        lua_pop(L, 1);
    }

    Input_set_mode(input, mode);

    return 0;
}

static int input_mode(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, input_mode0)
        LUAX_OVERLOAD_ARITY(1, input_mode1)
    LUAX_OVERLOAD_END
}
