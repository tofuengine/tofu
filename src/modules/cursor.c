/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "internal/udt.h"

#include <core/config.h>
#define _LOG_TAG "cursor"
#include <libs/log.h>
#include <systems/input.h>

static int cursor_new_0_1o(lua_State *L);
static int cursor_gc_1o_0(lua_State *L);
static int cursor_position_v_v(lua_State *L);
static int cursor_is_available_1o_1b(lua_State *L);
static int cursor_is_down_2oe_1b(lua_State *L);
static int cursor_is_up_2oe_1b(lua_State *L);
static int cursor_is_pressed_2oe_1b(lua_State *L);
static int cursor_is_released_2oe_1b(lua_State *L);

int cursor_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", cursor_new_0_1o },
            { "__gc", cursor_gc_1o_0 },
            // -- getters/setters --
            { "position", cursor_position_v_v },
            // -- accessors --
            { "is_available", cursor_is_available_1o_1b },
            { "is_down", cursor_is_down_2oe_1b },
            { "is_up", cursor_is_up_2oe_1b },
            { "is_pressed", cursor_is_pressed_2oe_1b },
            { "is_released", cursor_is_released_2oe_1b },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static int cursor_new_0_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Input_t *input = (Input_t *)udt_get_userdata(L, USERDATA_INPUT);

    Input_Cursor_t *cursor = Input_get_cursor(input);
    if (!cursor) {
        return luaL_error(L, "can't find cursor");
    }

    Cursor_Object_t *self = (Cursor_Object_t *)udt_newobject(L, sizeof(Cursor_Object_t), &(Cursor_Object_t){
            .cursor = cursor,
        }, OBJECT_TYPE_CURSOR);

    LOG_D("cursor %p allocated w/ cursor %p", self, cursor);

    return 1;
}

static int cursor_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Cursor_Object_t *self = (Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);

    LOG_D("cursor %p finalized", self);

    return 0;
}


static int cursor_position_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Cursor_Object_t *self = (const Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);

    const Input_Position_t position = Input_cursor_get_position(self->cursor);
    lua_pushinteger(L, (lua_Integer)position.x);
    lua_pushinteger(L, (lua_Integer)position.y);

    return 2;
}

static int cursor_position_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Cursor_Object_t *self = (const Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);

    const Input_Position_t position = (Input_Position_t){ .x = x, .y = y };
    Input_cursor_set_position(self->cursor, position);

    return 0;
}

static int cursor_position_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(cursor_position_1o_2nn, 1)
        LUAX_OVERLOAD_BY_ARITY(cursor_position_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static int cursor_is_available_1o_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Cursor_Object_t *self = (const Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);

    lua_pushboolean(L, Input_cursor_is_available(self->cursor));

    return 1;
}

static const char *_buttons[Input_Cursor_Buttons_t_CountOf + 1] = {
    "left",
    "right",
    "middle",
    NULL
};

static int cursor_is_down_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Cursor_Object_t *self = (const Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);
    Input_Cursor_Buttons_t id = (Input_Cursor_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, Input_cursor_get_button(self->cursor, id).down);

    return 1;
}

static int cursor_is_up_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Cursor_Object_t *self = (const Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);
    Input_Cursor_Buttons_t id = (Input_Cursor_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, !Input_cursor_get_button(self->cursor, id).down);

    return 1;
}

static int cursor_is_pressed_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Cursor_Object_t *self = (const Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);
    Input_Cursor_Buttons_t id = (Input_Cursor_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, Input_cursor_get_button(self->cursor, id).pressed);

    return 1;
}

static int cursor_is_released_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Cursor_Object_t *self = (const Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);
    Input_Cursor_Buttons_t id = (Input_Cursor_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, Input_cursor_get_button(self->cursor, id).released);

    return 1;
}
