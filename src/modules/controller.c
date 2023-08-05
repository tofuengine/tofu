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

#include "controller.h"

#include "internal/udt.h"

#include <core/config.h>
#include <libs/log.h>
#include <libs/path.h>
#include <systems/input.h>
#include <systems/storage.h>

#define LOG_CONTEXT "controller"
#define MODULE_NAME "tofu.input.controller"
#define META_TABLE  "Tofu_Input_Controller_mt"

static int controller_from_id_1n_1o(lua_State *L);
static int controller_gc_1o_0(lua_State *L);
static int controller_is_available_1o_1b(lua_State *L);
static int controller_is_down_2oe_1b(lua_State *L);
static int controller_is_up_2oe_1b(lua_State *L);
static int controller_is_pressed_2oe_1b(lua_State *L);
static int controller_is_released_2oe_1b(lua_State *L);
static int controller_stick_2oe_4nnnn(lua_State *L);
static int controller_triggers_1o_2nn(lua_State *L);

int controller_loader(lua_State *L)
{
    char name[PLATFORM_PATH_MAX] = { 0 };
    const char *file = path_lua_to_fs(name, MODULE_NAME);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));
    Storage_Resource_t *script = Storage_load(storage, file, STORAGE_RESOURCE_STRING);

    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){
            .data = S_SCHARS(script),
            .size = S_SLENTGH(script),
            .name = name
        },
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "from_id", controller_from_id_1n_1o },
            { "__gc", controller_gc_1o_0 },
            // -- accessors --
            { "is_available", controller_is_available_1o_1b },
            { "is_down", controller_is_down_2oe_1b },
            { "is_up", controller_is_up_2oe_1b },
            { "is_pressed", controller_is_pressed_2oe_1b },
            { "is_released", controller_is_released_2oe_1b },
            { "stick", controller_stick_2oe_4nnnn },
            { "triggers", controller_triggers_1o_2nn },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int controller_from_id_1n_1o(lua_State *L)
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

    LOG_D(LOG_CONTEXT, "controller %p allocated w/ controller %p for id `%u`", self, controller, id);

    return 1;
}

static int controller_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Controller_Object_t *self = (Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);

    LOG_D(LOG_CONTEXT, "controller %p finalized", self);

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

static const char *_buttons[Input_Controller_Buttons_t_CountOf + 1] = {
    "up",
    "down",
    "left",
    "right",
    "lb",
    "rb",
    "lt",
    "rt",
    "y",
    "x",
    "b",
    "a",
    "select",
    "start",
    NULL
};

static int controller_is_down_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);
    Input_Controller_Buttons_t id = (Input_Controller_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, Input_controller_get_button(self->controller, id).down);

    return 1;
}

static int controller_is_up_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);
    Input_Controller_Buttons_t id = (Input_Controller_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, !Input_controller_get_button(self->controller, id).down);

    return 1;
}

static int controller_is_pressed_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);
    Input_Controller_Buttons_t id = (Input_Controller_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, Input_controller_get_button(self->controller, id).pressed);

    return 1;
}

static int controller_is_released_2oe_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);
    Input_Controller_Buttons_t id = (Input_Controller_Buttons_t)LUAX_ENUM(L, 2, _buttons);

    lua_pushboolean(L, Input_controller_get_button(self->controller, id).released);

    return 1;
}

static const char *_sticks[Input_Controller_Sticks_t_CountOf + 1] = {
    "left",
    "right",
    NULL
};

static int controller_stick_2oe_4nnnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Controller_Object_t *self = (const Controller_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CONTROLLER);
    Input_Controller_Sticks_t id = (Input_Controller_Sticks_t)LUAX_ENUM(L, 2, _sticks);

    const Input_Controller_Stick_t stick = Input_controller_get_stick(self->controller, id);
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
