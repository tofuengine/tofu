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
#include <libs/log.h>
#include <libs/path.h>
#include <systems/input.h>
#include <systems/storage.h>

#define LOG_CONTEXT "keyboard"
#define MODULE_NAME "tofu.input.keyboard"
#define META_TABLE  "Tofu_Input_Keyboard_mt"

static int keyboard_new_0_1o(lua_State *L);
static int keyboard_gc_1o_0(lua_State *L);
static int keyboard_is_available_1o_1b(lua_State *L);
static int keyboard_is_down_2oe_1b(lua_State *L);
static int keyboard_is_up_2oe_1b(lua_State *L);
static int keyboard_is_pressed_2oe_1b(lua_State *L);
static int keyboard_is_released_2oe_1b(lua_State *L);

int keyboard_loader(lua_State *L)
{
    char file[PLATFORM_PATH_MAX] = { 0 };
    path_lua_to_fs(file, MODULE_NAME);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));
    Storage_Resource_t *script = Storage_load(storage, file + 1, STORAGE_RESOURCE_STRING);

    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){
            .data = S_SCHARS(script),
            .size = S_SLENTGH(script),
            .name = file
        },
        (const struct luaL_Reg[]){
            { "new", keyboard_new_0_1o },
            { "__gc", keyboard_gc_1o_0 },
            { "is_available", keyboard_is_available_1o_1b },
            { "is_down", keyboard_is_down_2oe_1b },
            { "is_up", keyboard_is_up_2oe_1b },
            { "is_pressed", keyboard_is_pressed_2oe_1b },
            { "is_released", keyboard_is_released_2oe_1b },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
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

    Keyboard_Object_t *self = (Keyboard_Object_t *)luaX_newobject(L, sizeof(Keyboard_Object_t), &(Keyboard_Object_t){
            .keyboard = keyboard,
        }, OBJECT_TYPE_KEYBOARD, META_TABLE);

    LOG_D(LOG_CONTEXT, "keyboard %p allocated w/ keyboard %p", self, keyboard);

    return 1;
}

static int keyboard_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Keyboard_Object_t *self = (Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);

    LOG_D(LOG_CONTEXT, "keyboard %p finalized", self);

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

static const char *_button_ids[Input_Keyboard_Buttons_t_CountOf + 1] = {
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

static const Input_Keyboard_Buttons_t _button_values[Input_Keyboard_Buttons_t_CountOf] = {
    INPUT_KEYBOARD_BUTTON_1,
    INPUT_KEYBOARD_BUTTON_2,
    INPUT_KEYBOARD_BUTTON_3,
    INPUT_KEYBOARD_BUTTON_4,
    INPUT_KEYBOARD_BUTTON_5,
    INPUT_KEYBOARD_BUTTON_6,
    INPUT_KEYBOARD_BUTTON_7,
    INPUT_KEYBOARD_BUTTON_8,
    INPUT_KEYBOARD_BUTTON_9,
    INPUT_KEYBOARD_BUTTON_0,
    INPUT_KEYBOARD_BUTTON_Q,
    INPUT_KEYBOARD_BUTTON_W,
    INPUT_KEYBOARD_BUTTON_E,
    INPUT_KEYBOARD_BUTTON_R,
    INPUT_KEYBOARD_BUTTON_T,
    INPUT_KEYBOARD_BUTTON_Y,
    INPUT_KEYBOARD_BUTTON_U,
    INPUT_KEYBOARD_BUTTON_I,
    INPUT_KEYBOARD_BUTTON_O,
    INPUT_KEYBOARD_BUTTON_P,
    INPUT_KEYBOARD_BUTTON_A,
    INPUT_KEYBOARD_BUTTON_S,
    INPUT_KEYBOARD_BUTTON_D,
    INPUT_KEYBOARD_BUTTON_F,
    INPUT_KEYBOARD_BUTTON_G,
    INPUT_KEYBOARD_BUTTON_H,
    INPUT_KEYBOARD_BUTTON_J,
    INPUT_KEYBOARD_BUTTON_K,
    INPUT_KEYBOARD_BUTTON_L,
    INPUT_KEYBOARD_BUTTON_Z,
    INPUT_KEYBOARD_BUTTON_X,
    INPUT_KEYBOARD_BUTTON_C,
    INPUT_KEYBOARD_BUTTON_V,
    INPUT_KEYBOARD_BUTTON_B,
    INPUT_KEYBOARD_BUTTON_N,
    INPUT_KEYBOARD_BUTTON_M,
    INPUT_KEYBOARD_BUTTON_UP,
    INPUT_KEYBOARD_BUTTON_DOWN,
    INPUT_KEYBOARD_BUTTON_LEFT,
    INPUT_KEYBOARD_BUTTON_RIGHT,
    INPUT_KEYBOARD_BUTTON_ENTER,
    INPUT_KEYBOARD_BUTTON_SPACE,
    INPUT_KEYBOARD_BUTTON_F1,
    INPUT_KEYBOARD_BUTTON_F2,
    INPUT_KEYBOARD_BUTTON_F3,
    INPUT_KEYBOARD_BUTTON_F4,
    INPUT_KEYBOARD_BUTTON_F5,
    INPUT_KEYBOARD_BUTTON_F6,
    INPUT_KEYBOARD_BUTTON_F7,
    INPUT_KEYBOARD_BUTTON_F8,
    INPUT_KEYBOARD_BUTTON_F9,
    INPUT_KEYBOARD_BUTTON_F10,
    INPUT_KEYBOARD_BUTTON_F11,
    INPUT_KEYBOARD_BUTTON_F12
};

static int keyboard_is_down_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    const int id = LUAX_ENUM(L, 2, _button_ids);

    lua_pushboolean(L, Input_keyboard_get_button(self->keyboard, _button_values[id]).down);

    return 1;
}

static int keyboard_is_up_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    const int id = LUAX_ENUM(L, 2, _button_ids);

    lua_pushboolean(L, !Input_keyboard_get_button(self->keyboard, _button_values[id]).down);

    return 1;
}

static int keyboard_is_pressed_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    const int id = LUAX_ENUM(L, 2, _button_ids);

    lua_pushboolean(L, Input_keyboard_get_button(self->keyboard, _button_values[id]).pressed);

    return 1;
}

static int keyboard_is_released_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    const int id = LUAX_ENUM(L, 2, _button_ids);

    lua_pushboolean(L, Input_keyboard_get_button(self->keyboard, _button_values[id]).released);

    return 1;
}
