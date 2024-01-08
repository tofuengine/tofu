/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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

#include "keyboard.h"

#include "internal/udt.h"

#include <core/config.h>
#define _LOG_TAG "keyboard"
#include <libs/log.h>
#include <systems/input.h>

static int keyboard_new_0_1o(lua_State *L);
static int keyboard_gc_1o_0(lua_State *L);
static int keyboard_is_available_1o_1b(lua_State *L);
static int keyboard_is_down_2oe_1b(lua_State *L);
static int keyboard_is_up_2oe_1b(lua_State *L);
static int keyboard_is_pressed_2oe_1b(lua_State *L);
static int keyboard_is_released_2oe_1b(lua_State *L);

int keyboard_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", keyboard_new_0_1o },
            { "__gc", keyboard_gc_1o_0 },
            // -- accessors --
            { "is_available", keyboard_is_available_1o_1b },
            { "is_down", keyboard_is_down_2oe_1b },
            { "is_up", keyboard_is_up_2oe_1b },
            { "is_pressed", keyboard_is_pressed_2oe_1b },
            { "is_released", keyboard_is_released_2oe_1b },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static int keyboard_new_0_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    Input_Keyboard_t *keyboard = Input_get_keyboard(input);
    if (!keyboard) {
        return luaL_error(L, "can't find keyboard");
    }

    Keyboard_Object_t *self = (Keyboard_Object_t *)udt_newobject(L, sizeof(Keyboard_Object_t), &(Keyboard_Object_t){
            .keyboard = keyboard,
        }, OBJECT_TYPE_KEYBOARD);

    LOG_D("keyboard %p allocated w/ keyboard %p", self, keyboard);

    return 1;
}

static int keyboard_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Keyboard_Object_t *self = (Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);

    LOG_D("keyboard %p finalized", self);

    return 0;
}

static int keyboard_is_available_1o_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);

    lua_pushboolean(L, Input_keyboard_is_available(self->keyboard));

    return 1;
}

static const char *_buttons[Input_Keyboard_Buttons_t_CountOf + 1] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "q",
    "w",
    "e",
    "r",
    "t",
    "y",
    "u",
    "i",
    "o",
    "p",
    "a",
    "s",
    "d",
    "f",
    "g",
    "h",
    "j",
    "k",
    "l",
    "z",
    "x",
    "c",
    "v",
    "b",
    "n",
    "m",
    "up",
    "down",
    "left",
    "right",
    "enter",
    "space",
    "f1",
    "f2",
    "f3",
    "f4",
    "f5",
    "f6",
    "f7",
    "f8",
    "f9",
    "f10",
    "f11",
    "f12",
    NULL
};

static int keyboard_is_down_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    Input_Keyboard_Buttons_t id = (Input_Keyboard_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, Input_keyboard_get_button(self->keyboard, id).down);

    return 1;
}

static int keyboard_is_up_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    Input_Keyboard_Buttons_t id = (Input_Keyboard_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, !Input_keyboard_get_button(self->keyboard, id).down);

    return 1;
}

static int keyboard_is_pressed_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    Input_Keyboard_Buttons_t id = (Input_Keyboard_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, Input_keyboard_get_button(self->keyboard, id).pressed);

    return 1;
}

static int keyboard_is_released_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    Input_Keyboard_Buttons_t id = (Input_Keyboard_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, Input_keyboard_get_button(self->keyboard, id).released);

    return 1;
}
