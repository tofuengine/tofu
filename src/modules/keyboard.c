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

#include "keyboard.h"

#include <config.h>
#include <libs/log.h>
#include <libs/path.h>
#include <systems/input.h>
#include <systems/storage.h>

#include "udt.h"
#include "utils/map.h"

#define LOG_CONTEXT "keyboard"
#define MODULE_NAME "tofu.input.keyboard"
#define META_TABLE  "Tofu_Input_Keyboard_mt"

static int keyboard_new_0_1o(lua_State *L);
static int keyboard_gc_1o_0(lua_State *L);
static int keyboard_is_available_1o_1b(lua_State *L);
static int keyboard_is_down_2os_1b(lua_State *L);
static int keyboard_is_up_2os_1b(lua_State *L);
static int keyboard_is_pressed_2os_1b(lua_State *L);
static int keyboard_is_released_2os_1b(lua_State *L);

int keyboard_loader(lua_State *L)
{
    char file[PATH_MAX] = { 0 };
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
            { "is_down", keyboard_is_down_2os_1b },
            { "is_up", keyboard_is_up_2os_1b },
            { "is_pressed", keyboard_is_pressed_2os_1b },
            { "is_released", keyboard_is_released_2os_1b },
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

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "keyboard %p allocated w/ keyboard %p", self, keyboard);

    return 1;
}

static int keyboard_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Keyboard_Object_t *self = (Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "keyboard %p finalized", self);

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

static const Map_Entry_t _buttons[Input_Keyboard_Buttons_t_CountOf] = {
    // TODO: add more keys? Function keys?
    { "1", INPUT_KEYBOARD_BUTTON_1 },
    { "2", INPUT_KEYBOARD_BUTTON_2 },
    { "3", INPUT_KEYBOARD_BUTTON_3 },
    { "4", INPUT_KEYBOARD_BUTTON_4 },
    { "5", INPUT_KEYBOARD_BUTTON_5 },
    { "6", INPUT_KEYBOARD_BUTTON_6 },
    { "7", INPUT_KEYBOARD_BUTTON_7 },
    { "8", INPUT_KEYBOARD_BUTTON_8 },
    { "9", INPUT_KEYBOARD_BUTTON_9 },
    { "0", INPUT_KEYBOARD_BUTTON_0 },
    { "q", INPUT_KEYBOARD_BUTTON_Q },
    { "w", INPUT_KEYBOARD_BUTTON_W },
    { "e", INPUT_KEYBOARD_BUTTON_E },
    { "r", INPUT_KEYBOARD_BUTTON_R },
    { "t", INPUT_KEYBOARD_BUTTON_T },
    { "y", INPUT_KEYBOARD_BUTTON_Y },
    { "u", INPUT_KEYBOARD_BUTTON_U },
    { "i", INPUT_KEYBOARD_BUTTON_I },
    { "o", INPUT_KEYBOARD_BUTTON_O },
    { "p", INPUT_KEYBOARD_BUTTON_P },
    { "a", INPUT_KEYBOARD_BUTTON_A },
    { "s", INPUT_KEYBOARD_BUTTON_S },
    { "d", INPUT_KEYBOARD_BUTTON_D },
    { "f", INPUT_KEYBOARD_BUTTON_F },
    { "g", INPUT_KEYBOARD_BUTTON_G },
    { "h", INPUT_KEYBOARD_BUTTON_H },
    { "j", INPUT_KEYBOARD_BUTTON_J },
    { "k", INPUT_KEYBOARD_BUTTON_K },
    { "l", INPUT_KEYBOARD_BUTTON_L },
    { "z", INPUT_KEYBOARD_BUTTON_Z },
    { "x", INPUT_KEYBOARD_BUTTON_X },
    { "c", INPUT_KEYBOARD_BUTTON_C },
    { "v", INPUT_KEYBOARD_BUTTON_V },
    { "b", INPUT_KEYBOARD_BUTTON_B },
    { "n", INPUT_KEYBOARD_BUTTON_N },
    { "m", INPUT_KEYBOARD_BUTTON_M },
    { "up", INPUT_KEYBOARD_BUTTON_UP },
    { "down", INPUT_KEYBOARD_BUTTON_DOWN },
    { "left", INPUT_KEYBOARD_BUTTON_LEFT },
    { "right", INPUT_KEYBOARD_BUTTON_RIGHT },
    { "enter", INPUT_KEYBOARD_BUTTON_ENTER },
    { "space", INPUT_KEYBOARD_BUTTON_SPACE }
};

static int keyboard_is_down_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Keyboard_Buttons_t_CountOf);
    lua_pushboolean(L, Input_keyboard_get_button(self->keyboard, (Input_Keyboard_Buttons_t)entry->value).down);

    return 1;
}

static int keyboard_is_up_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Keyboard_Buttons_t_CountOf);
    lua_pushboolean(L, !Input_keyboard_get_button(self->keyboard, (Input_Keyboard_Buttons_t)entry->value).down);

    return 1;
}

static int keyboard_is_pressed_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Keyboard_Buttons_t_CountOf);
    lua_pushboolean(L, Input_keyboard_get_button(self->keyboard, (Input_Keyboard_Buttons_t)entry->value).pressed);

    return 1;
}

static int keyboard_is_released_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Keyboard_Object_t *self = (const Keyboard_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_KEYBOARD);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Keyboard_Buttons_t_CountOf);
    lua_pushboolean(L, Input_keyboard_get_button(self->keyboard, (Input_Keyboard_Buttons_t)entry->value).released);

    return 1;
}
