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

#include "controller.h"

#include <config.h>
#include <libs/log.h>
#include <systems/input.h>

#include "udt.h"
#include "utils/map.h"

#define LOG_CONTEXT "controller"
#define META_TABLE  "Tofu_Input_Controller_mt"

static int controller_new_1n_1o(lua_State *L);
static int controller_gc_1o_0(lua_State *L);
static int controller_is_available_1o_1b(lua_State *L);
static int controller_is_down_2os_1b(lua_State *L);
static int controller_is_up_2os_1b(lua_State *L);
static int controller_is_pressed_2os_1b(lua_State *L);
static int controller_is_released_2os_1b(lua_State *L);
static int controller_stick_2os_4nnnn(lua_State *L);
static int controller_triggers_1o_2nn(lua_State *L);
static int controller_flags_v_v(lua_State *L);

int controller_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", controller_new_1n_1o },
            { "__gc", controller_gc_1o_0 },
            { "is_available", controller_is_available_1o_1b },
            { "is_down", controller_is_down_2os_1b },
            { "is_up", controller_is_up_2os_1b },
            { "is_pressed", controller_is_pressed_2os_1b },
            { "is_released", controller_is_released_2os_1b },
            { "stick", controller_stick_2os_4nnnn },
            { "triggers", controller_triggers_1o_2nn },
            { "flags", controller_flags_v_v },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { "FLAG_EMULATED", LUA_CT_INTEGER, { .i = INPUT_FLAG_EMULATED } },
            { "FLAG_DPAD", LUA_CT_INTEGER, { .i = INPUT_FLAG_DPAD } },
            { "FLAG_CURSOR", LUA_CT_INTEGER, { .i = INPUT_FLAG_CURSOR } },
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static const Map_Entry_t _buttons[Input_Controller_Buttons_t_CountOf] = {
    { "up", INPUT_CONTROLLER_BUTTON_UP },
    { "down", INPUT_CONTROLLER_BUTTON_DOWN },
    { "left", INPUT_CONTROLLER_BUTTON_LEFT },
    { "right", INPUT_CONTROLLER_BUTTON_RIGHT },
    { "lb", INPUT_CONTROLLER_BUTTON_LB },
    { "rb", INPUT_CONTROLLER_BUTTON_RB },
    { "lt", INPUT_CONTROLLER_BUTTON_LT },
    { "rt", INPUT_CONTROLLER_BUTTON_RT },
    { "y", INPUT_CONTROLLER_BUTTON_Y },
    { "x", INPUT_CONTROLLER_BUTTON_X },
    { "b", INPUT_CONTROLLER_BUTTON_B },
    { "a", INPUT_CONTROLLER_BUTTON_A },
    { "select", INPUT_CONTROLLER_BUTTON_SELECT },
    { "start", INPUT_CONTROLLER_BUTTON_START }
};

static const Map_Entry_t _sticks[Input_Controller_Sticks_t_CountOf] = {
    { "left", INPUT_CONTROLLER_STICK_LEFT },
    { "right", INPUT_CONTROLLER_STICK_RIGHT }
};

static int controller_new_1n_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t id = LUAX_OPTIONAL_UNSIGNED(L, 1, 0);

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    Input_Controller_t *controller = Input_get_controller(input, id);
    if (!controller) {
        return luaL_error(L, "can't find controller `%u`", id);
    }

    Controller_Object_t *self = (Controller_Object_t *)luaX_newobject(L, sizeof(Controller_Object_t), &(Controller_Object_t){
            .controller = controller,
        }, OBJECT_TYPE_CONTROLLER, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "controller %p allocated w/ controller %p for id `%u`", self, controller, id);

    return 1;
}

static int controller_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Controller_Object_t *self = (Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "controller %p finalized", self);

    return 0;
}

static int controller_is_available_1o_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);

    lua_pushboolean(L, Input_controller_is_available(self->controller));

    return 1;
}

static int controller_is_down_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, Input_controller_get_button(self->controller, (Input_Controller_Buttons_t)entry->value).down);

    return 1;
}

static int controller_is_up_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, !Input_controller_get_button(self->controller, (Input_Controller_Buttons_t)entry->value).down);

    return 1;
}

static int controller_is_pressed_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, Input_controller_get_button(self->controller, (Input_Controller_Buttons_t)entry->value).pressed);

    return 1;
}

static int controller_is_released_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(L, id, _buttons, Input_Buttons_t_CountOf);
    lua_pushboolean(L, Input_controller_get_button(self->controller, (Input_Controller_Buttons_t)entry->value).released);

    return 1;
}

static int controller_stick_2os_4nnnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(L, id, _sticks, Input_Controller_Sticks_t_CountOf);
    const Input_Controller_Stick_t stick = Input_controller_get_stick(self->controller, (Input_Controller_Sticks_t)entry->value);
    lua_pushnumber(L, (lua_Number)stick.x);
    lua_pushnumber(L, (lua_Number)stick.y);
    lua_pushnumber(L, (lua_Number)stick.angle);
    lua_pushnumber(L, (lua_Number)stick.magnitude);

    return 4;
}

static int controller_triggers_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);

    const Input_Controller_Triggers_t triggers = Input_controller_get_triggers(self->controller);
    lua_pushnumber(L, (lua_Number)triggers.left);
    lua_pushnumber(L, (lua_Number)triggers.right);

    return 2;
}

static int controller_flags_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);

    const Input_Controller_t *controller = self->controller;
    lua_pushinteger(L, Input_controller_get_flags(controller));

    return 1;
}

static int controller_flags_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Controller_Object_t *self = (Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);
    int flags = LUAX_INTEGER(L, 2);

    Input_Controller_t *controller = self->controller;
    Input_controller_set_flags(controller, flags);

    return 0;
}

static int controller_flags_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, controller_flags_1o_1n)
        LUAX_OVERLOAD_ARITY(2, controller_flags_2on_0)
    LUAX_OVERLOAD_END
}
