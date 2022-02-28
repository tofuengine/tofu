/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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

#include "cursor.h"

#include <config.h>
#include <systems/input.h>

#include "udt.h"
#include "utils/map.h"

#define LOG_CONTEXT "cursor"
#define META_TABLE  "Tofu_Input_Cursor_mt"

static int cursor_new_v_1o(lua_State *L);
static int cursor_gc_1o_0(lua_State *L);
static int cursor_is_down_1s_1b(lua_State *L);
static int cursor_is_up_1s_1b(lua_State *L);
static int cursor_is_pressed_1s_1b(lua_State *L);
static int cursor_is_released_1s_1b(lua_State *L);
static int cursor_position_v_v(lua_State *L);
static int cursor_area_v_v(lua_State *L);

int cursor_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", cursor_new_v_1o },
            { "__gc", cursor_gc_1o_0 },
            { "is_available", cursor_is_available_1o_1b },
            { "is_down", cursor_is_down_1s_1b },
            { "is_up", cursor_is_up_1s_1b },
            { "is_pressed", cursor_is_pressed_1s_1b },
            { "is_released", cursor_is_released_1s_1b },
            { "position", cursor_position_v_v },
            { "area", cursor_area_v_v },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, NULL);
}

static int cursor_new_v_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_OPTIONAL_STRING(L, 1, "default")

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    Input_Cursor_t *cursor = Input_find_cursor(input, id);
    if (!cursor) {
        return luaL_error(L, "can't find cursor `%s`", id);
    }

    Cursor_Object_t *self = (Cursor_Object_t *)luaX_newobject(L, sizeof(Cursor_Object_t), &(Cursor_Object_t){
            .cursor = cursor,
        }, OBJECT_TYPE_CURSOR, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "cursor %p allocated w/ cursor %p for id `%s`", self, cursor, id);

    return 1;
}

static int cursor_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Input_Cursor_t *self = (Input_Cursor_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);

    Input_cursor_detach(self->cursor);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "cursor %p detached", self->cursor);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "cursor %p finalized", self);

    return 0;
}

static const Map_Entry_t _buttons[Input_Mouse_Buttons_t_CountOf] = {
    { "left", INPUT_MOUSE_BUTTON_LEFT },
    { "center", INPUT_MOUSE_BUTTON_CENTER },
    { "right", INPUT_MOUSE_BUTTON_RIGHT },
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

static int input_cursor_0_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Input_t *input = (const Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    const Input_Cursor_t *cursor = Input_get_cursor(input);
    lua_pushinteger(L, (lua_Integer)cursor->x);
    lua_pushinteger(L, (lua_Integer)cursor->y);

    return 2;
}

static int input_cursor_2nn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int x = LUAX_INTEGER(L, 1);
    int y = LUAX_INTEGER(L, 2);

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
    lua_pushinteger(L, (lua_Integer)cursor->area.x0);
    lua_pushinteger(L, (lua_Integer)cursor->area.y0);
    lua_pushinteger(L, (lua_Integer)(cursor->area.x1 - cursor->area.x0 + 1.0f));
    lua_pushinteger(L, (lua_Integer)(cursor->area.y1 - cursor->area.y0 + 1.0f));

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
    size_t width = LUAX_UNSIGNED(L, 3);
    size_t height = LUAX_UNSIGNED(L, 4);

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    Input_set_cursor_area(input, x, y, width, height);

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
