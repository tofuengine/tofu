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

#include "internal/udt.h"

#include <core/config.h>
#include <libs/log.h>
#include <libs/map.h>
#include <libs/path.h>
#include <systems/input.h>
#include <systems/storage.h>

#define LOG_CONTEXT "cursor"
#define MODULE_NAME "tofu.input.cursor"
#define META_TABLE  "Tofu_Input_Cursor_mt"

static int cursor_new_0_1o(lua_State *L);
static int cursor_gc_1o_0(lua_State *L);
static int cursor_is_available_1o_1b(lua_State *L);
static int cursor_is_down_2os_1b(lua_State *L);
static int cursor_is_up_2os_1b(lua_State *L);
static int cursor_is_pressed_2os_1b(lua_State *L);
static int cursor_is_released_2os_1b(lua_State *L);
static int cursor_position_v_v(lua_State *L);

int cursor_loader(lua_State *L)
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
            { "new", cursor_new_0_1o },
            { "__gc", cursor_gc_1o_0 },
            { "is_available", cursor_is_available_1o_1b },
            { "is_down", cursor_is_down_2os_1b },
            { "is_up", cursor_is_up_2os_1b },
            { "is_pressed", cursor_is_pressed_2os_1b },
            { "is_released", cursor_is_released_2os_1b },
            { "position", cursor_position_v_v },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int cursor_new_0_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Input_t *input = (Input_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INPUT));

    Input_Cursor_t *cursor = Input_get_cursor(input);
    if (!cursor) {
        return luaL_error(L, "can't find cursor");
    }

    Cursor_Object_t *self = (Cursor_Object_t *)luaX_newobject(L, sizeof(Cursor_Object_t), &(Cursor_Object_t){
            .cursor = cursor,
        }, OBJECT_TYPE_CURSOR, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "cursor %p allocated w/ cursor %p", self, cursor);

    return 1;
}

static int cursor_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Cursor_Object_t *self = (Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "cursor %p finalized", self);

    return 0;
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

static const Map_Entry_t _buttons[Input_Cursor_Buttons_t_CountOf + 1] = {
    { "left", INPUT_CURSOR_BUTTON_LEFT },
    { "right", INPUT_CURSOR_BUTTON_RIGHT },
    { "middle", INPUT_CURSOR_BUTTON_MIDDLE },
    { NULL, 0 }
};

static int cursor_is_down_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Cursor_Object_t *self = (const Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(id, _buttons);
    if (!entry) {
        return luaL_error(L, "unknown cursor button `%s`", id);
    }

    lua_pushboolean(L, Input_cursor_get_button(self->cursor, (Input_Cursor_Buttons_t)entry->value).down);

    return 1;
}

static int cursor_is_up_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Cursor_Object_t *self = (const Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(id, _buttons);
    if (!entry) {
        return luaL_error(L, "unknown cursor button `%s`", id);
    }

    lua_pushboolean(L, !Input_cursor_get_button(self->cursor, (Input_Cursor_Buttons_t)entry->value).down);

    return 1;
}

static int cursor_is_pressed_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Cursor_Object_t *self = (const Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(id, _buttons);
    if (!entry) {
        return luaL_error(L, "unknown cursor button `%s`", id);
    }

    lua_pushboolean(L, Input_cursor_get_button(self->cursor, (Input_Cursor_Buttons_t)entry->value).pressed);

    return 1;
}

static int cursor_is_released_2os_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Cursor_Object_t *self = (const Cursor_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CURSOR);
    const char *id = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(id, _buttons);
    if (!entry) {
        return luaL_error(L, "unknown cursor button `%s`", id);
    }

    lua_pushboolean(L, Input_cursor_get_button(self->cursor, (Input_Cursor_Buttons_t)entry->value).released);

    return 1;
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
        LUAX_OVERLOAD_ARITY(1, cursor_position_1o_2nn)
        LUAX_OVERLOAD_ARITY(3, cursor_position_3onn_0)
    LUAX_OVERLOAD_END
}
