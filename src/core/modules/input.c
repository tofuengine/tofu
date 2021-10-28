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
#include <core/systems/input.h>

#include "udt.h"
#include "utils/map.h"

static int input_is_down_1s_1b(lua_State *L);
static int input_is_up_1s_1b(lua_State *L);
static int input_is_pressed_1s_1b(lua_State *L);
static int input_is_released_1s_1b(lua_State *L);
static int input_auto_repeat_v_v(lua_State *L);
static int input_cursor_v_v(lua_State *L);
static int input_cursor_area_v_v(lua_State *L);
static int input_stick_1s_4nnnn(lua_State *L);
static int input_triggers_0_2nn(lua_State *L);
static int input_mode_v_v(lua_State *L);
static int input_has_input_0_1b(lua_State *L);

int input_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "is_down", input_is_down_1s_1b },
            { "is_up", input_is_up_1s_1b },
            { "is_pressed", input_is_pressed_1s_1b },
            { "is_released", input_is_released_1s_1b },
            { "auto_repeat", input_auto_repeat_v_v },
            { "cursor", input_cursor_v_v },
            { "cursor_area", input_cursor_area_v_v },
            { "stick", input_stick_1s_4nnnn },
            { "triggers", input_triggers_0_2nn },
            { "mode", input_mode_v_v },
            { "has_input", input_has_input_0_1b },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, NULL);
}

static const Map_Entry_t _buttons[Input_Buttons_t_CountOf] = {
    { "up", INPUT_BUTTON_UP },
    { "down", INPUT_BUTTON_DOWN },
    { "left", INPUT_BUTTON_LEFT },
    { "right", INPUT_BUTTON_RIGHT },
    { "lb", INPUT_BUTTON_LB },
    { "rb", INPUT_BUTTON_RB },
    { "lt", INPUT_BUTTON_LT },
    { "rt", INPUT_BUTTON_RT },
    { "y", INPUT_BUTTON_Y },
    { "x", INPUT_BUTTON_X },
    { "b", INPUT_BUTTON_B },
    { "a", INPUT_BUTTON_A },
    { "select", INPUT_BUTTON_SELECT },
    { "start", INPUT_BUTTON_START },
#ifdef __GRAPHICS_CAPTURE_SUPPORT__
    { NULL, -1 },
    { NULL, -1 },
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */
    { "quit", INPUT_BUTTON_QUIT }
};

static const Map_Entry_t _sticks[Input_Sticks_t_CountOf] = {
    { "left", INPUT_STICK_LEFT },
    { "right", INPUT_STICK_RIGHT }
};

static const Map_Entry_t _modes[INPUT_MODES_COUNT] = {
    { "keyboard", INPUT_MODE_KEYBOARD },
    { "mouse", INPUT_MODE_MOUSE },
    { "gamepad", INPUT_MODE_GAMEPAD }
};

static int input_is_down_1s_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, Input_get_button(input, (Input_Buttons_t)entry->value)->down);

    return 1;
}

static int input_is_up_1s_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, !Input_get_button(input, (Input_Buttons_t)entry->value)->down);

    return 1;
}

static int input_is_pressed_1s_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, Input_get_button(input, (Input_Buttons_t)entry->value)->pressed);

    return 1;
}

static int input_is_released_1s_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, Input_get_button(input, (Input_Buttons_t)entry->value)->released);

    return 1;
}

static int input_auto_repeat_1s_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Buttons_t_CountOf);
    float period = Input_get_auto_repeat(input, (Input_Buttons_t)entry->value);
    lua_pushnumber(L, (lua_Number)period);

    return 1;
}

static int input_auto_repeat_2sn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);
    float period = LUAX_NUMBER(L, 2);

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Buttons_t_CountOf);
    Input_set_auto_repeat(input, (Input_Buttons_t)entry->value, period);

    return 0;
}

static int input_auto_repeat_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, input_auto_repeat_1s_1n)
        LUAX_OVERLOAD_ARITY(2, input_auto_repeat_2sn_0)
    LUAX_OVERLOAD_END
}

static int input_cursor_0_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_Cursor_t *cursor = Input_get_cursor(input);
    lua_pushnumber(L, (lua_Number)cursor->x);
    lua_pushnumber(L, (lua_Number)cursor->y);

    return 2;
}

static int input_cursor_2nn_0(lua_State *L)
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

static int input_cursor_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, input_cursor_0_2nn)
        LUAX_OVERLOAD_ARITY(2, input_cursor_2nn_0)
    LUAX_OVERLOAD_END
}

static int input_cursor_area_0_4nnnn(lua_State *L) 
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_Cursor_t *cursor = Input_get_cursor(input);
    lua_pushnumber(L, (lua_Number)cursor->area.x0);
    lua_pushnumber(L, (lua_Number)cursor->area.y0);
    lua_pushnumber(L, (lua_Number)(cursor->area.x1 - cursor->area.x0));
    lua_pushnumber(L, (lua_Number)(cursor->area.y1 - cursor->area.y0));

    return 4;
}

static int input_cursor_area_4nnnn_0(lua_State *L) 
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

    Input_set_cursor_area(input, x, y, x + width, y + height);

    return 0;
}

static int input_cursor_area_v_v(lua_State *L)// TODO: rename to `region`?
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, input_cursor_area_0_4nnnn)
        LUAX_OVERLOAD_ARITY(4, input_cursor_area_4nnnn_0)
    LUAX_OVERLOAD_END
}

static int input_stick_1s_4nnnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Map_Entry_t *entry = map_find_key(L, id, _sticks, Input_Sticks_t_CountOf);
    const Input_Stick_t *stick = Input_get_stick(input, (Input_Sticks_t)entry->value);
    lua_pushnumber(L, (lua_Number)stick->x);
    lua_pushnumber(L, (lua_Number)stick->y);
    lua_pushnumber(L, (lua_Number)stick->angle);
    lua_pushnumber(L, (lua_Number)stick->magnitude);

    return 4;
}

static int input_triggers_0_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_Triggers_t *triggers = Input_get_triggers(input);
    lua_pushnumber(L, (lua_Number)triggers->left);
    lua_pushnumber(L, (lua_Number)triggers->right);

    return 2;
}

static int input_mode_0_1t(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    int mode = Input_get_mode(input);

    lua_newtable(L); // Initially empty.
    for (size_t i = 0; i < INPUT_MODES_COUNT; ++i) {
        if (mode & _modes[i].value) {
            lua_pushstring(L, _modes[i].key);
            lua_rawseti(L, -2, (lua_Integer)(lua_rawlen(L, -2) + 1)); // Append to table, same as `table[#table + 1] = value`.
        }
    }

    return 1;
}

static int input_mode_1t_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    // idx #1: LUA_TTABLE

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    int mode = INPUT_MODE_NONE;

    lua_pushnil(L);
    while (lua_next(L, 1)) {
//        int index = LUAX_INTEGER(L, -2);
        const char *id = LUAX_STRING(L, -1);

        const Map_Entry_t *entry = map_find_key(L, id, _modes, INPUT_MODES_COUNT);
        mode |= entry->value;

        lua_pop(L, 1);
    }

    Input_set_mode(input, mode);

    return 0;
}

static int input_mode_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, input_mode_0_1t)
        LUAX_OVERLOAD_ARITY(1, input_mode_1t_0)
    LUAX_OVERLOAD_END
}

static int input_has_input_0_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    bool has_input = (input->mode != INPUT_MODE_NONE)
        && (input->mode & INPUT_MODE_KEYMOUSE || input->gamepad.count == 0);
    lua_pushboolean(L, has_input);

    return 1;
}
