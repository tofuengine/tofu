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

#include "grid.h"

#include "internal/udt.h"

#include <core/config.h>
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>
#include <systems/interpreter.h>

#define LOG_CONTEXT "grid"
#define MODULE_NAME "tofu.util.grid"
#define META_TABLE  "Tofu_Util_Grid_mt"

static int grid_new_3nnT_1o(lua_State *L);
static int grid_gc_1o_0(lua_State *L);
static int grid_size_1o_2nn(lua_State *L);
static int grid_fill_2ot_0(lua_State *L);
static int grid_copy_2oo_0(lua_State *L);
static int grid_peek_v_1n(lua_State *L);
static int grid_poke_v_0(lua_State *L);
static int grid_scan_2of_0(lua_State *L);
static int grid_process_2of_0(lua_State *L);
static int grid_path_5onnnn_1t(lua_State *L);

int grid_loader(lua_State *L)
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
            { "new", grid_new_3nnT_1o },
            { "__gc", grid_gc_1o_0 },
            { "size", grid_size_1o_2nn },
            { "fill", grid_fill_2ot_0 },
            { "copy", grid_copy_2oo_0 },
            { "peek", grid_peek_v_1n },
            { "poke", grid_poke_v_0 },
            { "scan", grid_scan_2of_0 },
            { "process", grid_process_2of_0 },
            { "path", grid_path_5onnnn_1t },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int grid_new_3nnT_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TTABLE)
    LUAX_SIGNATURE_END
    size_t width = LUAX_UNSIGNED(L, 1);
    size_t height = LUAX_UNSIGNED(L, 2);
    size_t length = LUAX_OPTIONAL_TABLE(L, 3, 0);

    size_t data_size = width * height;
    Grid_Object_Value_t *data = malloc(sizeof(Grid_Object_Value_t) * data_size);
    if (!data) {
        return luaL_error(L, "can't allocate %dx%d grid", width, height);
    }

    if (length > 0) {
        Grid_Object_Value_t *ptr = data;
        for (size_t i = 0; i < data_size; ++i) {
            size_t index = ((i % length) + 1);
            lua_rawgeti(L, 3, (lua_Integer)index);

            Grid_Object_Value_t value = (Grid_Object_Value_t)LUAX_NUMBER(L, -1);
            *(ptr++) = value;

            lua_pop(L, 1);
        }
    } else {
        LOG_W(LOG_CONTEXT, "grid content left uninitialized");
    }

    Grid_Object_t *self = (Grid_Object_t *)luaX_newobject(L, sizeof(Grid_Object_t), &(Grid_Object_t){
            .width = width,
            .height = height,
            .data = data,
            .data_size = data_size
        }, OBJECT_TYPE_GRID, META_TABLE);

    LOG_D(LOG_CONTEXT, "grid %p allocated w/ data %p", self, data);

    return 1;
}

static int grid_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);

    free(self->data);
    LOG_D(LOG_CONTEXT, "data %p freed", self->data);

    LOG_D(LOG_CONTEXT, "grid %p finalized", self);

    return 0;
}

static int grid_size_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);

    lua_pushinteger(L, (lua_Integer)self->width);
    lua_pushinteger(L, (lua_Integer)self->height);

    return 2;
}

static int grid_fill_2ot_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    // idx #2: LUA_TTABLE

    size_t length = lua_rawlen(L, 2);
    if (length == 0) {
        return luaL_error(L, "table can't be empty");
    }

    Grid_Object_Value_t *ptr = self->data;

    for (size_t i = 0; i < self->data_size; ++i) {
        size_t index = ((i % length) + 1);
        lua_rawgeti(L, 2, (lua_Integer)index);

        Grid_Object_Value_t value = (Grid_Object_Value_t)LUAX_NUMBER(L, -1);
        *(ptr++) = value;

        lua_pop(L, 1);
    }

    return 0;
}

static int grid_copy_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    const Grid_Object_t *other = (const Grid_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_GRID);

    if (self->data_size != other->data_size) {
        return luaL_error(L, "grid data-size don't match");
    }

    Grid_Object_Value_t *dptr = self->data;
    const Grid_Object_Value_t *sptr = other->data;

    for (size_t i = self->data_size; i; --i) {
        *(dptr++) = *(sptr++);
    }

    return 0;
}

static int grid_peek_2on_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t offset = LUAX_UNSIGNED(L, 2);
#if defined(DEBUG)
    if (offset >= self->data_size) {
        return luaL_error(L, "offset %d is out of range (0, %d)", offset, self->data_size);
    }
#endif

    Grid_Object_Value_t value = self->data[offset];

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int grid_peek_3onn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t column = LUAX_UNSIGNED(L, 2);
    size_t row = LUAX_UNSIGNED(L, 3);
#if defined(DEBUG)
    if (column >= self->width) {
        return luaL_error(L, "column %d is out of range (0, %d)", column, self->width);
    } else
    if (row >= self->height) {
        return luaL_error(L, "row %d is out of range (0, %d)", row, self->height);
    }
#endif

    Grid_Object_Value_t value = self->data[row * self->width + column];

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int grid_peek_v_1n(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, grid_peek_2on_1n)
        LUAX_OVERLOAD_ARITY(3, grid_peek_3onn_1n)
    LUAX_OVERLOAD_END
}

static int grid_poke_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t offset = LUAX_UNSIGNED(L, 2);
    Grid_Object_Value_t value = (Grid_Object_Value_t)LUAX_NUMBER(L, 3);
#if defined(DEBUG)
    if (offset >= self->data_size) {
        return luaL_error(L, "offset %d is out of range [0, %d)", offset, self->data_size);
    }
#endif

    self->data[offset] = value;

    return 0;
}

static int grid_poke_4onnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t column = LUAX_UNSIGNED(L, 2);
    size_t row = LUAX_UNSIGNED(L, 3);
    Grid_Object_Value_t value = (Grid_Object_Value_t)LUAX_NUMBER(L, 4);
#if defined(DEBUG)
    if (column >= self->width) {
        return luaL_error(L, "column %d is out of range [0, %d)", column, self->width);
    } else
    if (row >= self->height) {
        return luaL_error(L, "row %d is out of range [0, %d)", row, self->height);
    }
#endif

    self->data[row * self->width + column] = value;

    return 0;
}

static int grid_poke_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, grid_poke_3onn_0)
        LUAX_OVERLOAD_ARITY(4, grid_poke_4onnn_0)
    LUAX_OVERLOAD_END
}

static int grid_scan_2of_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
//    luaX_Reference callback = luaX_tofunction(L, 2);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const Grid_Object_Value_t *data = self->data;

    for (size_t row = 0; row < self->height; ++row) {
        for (size_t column = 0; column < self->width; ++column) {
            lua_pushvalue(L, 2); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed meanwhile)
            lua_pushinteger(L, (lua_Integer)column);
            lua_pushinteger(L, (lua_Integer)row);
            lua_pushnumber(L, (lua_Number)*(data++));
            Interpreter_call(interpreter, 3, 0);
        }
    }

    return 0;
}

static int grid_process_2of_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
//    luaX_Reference callback = luaX_tofunction(L, 2);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    Grid_Object_Value_t *data = self->data;

    const size_t width = self->width;
    const size_t height = self->height;

    const Grid_Object_Value_t *ptr = data;

    for (size_t row = 0; row < height; ++row) {
        for (size_t column = 0; column < width; ++column) {
            lua_pushvalue(L, 2); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed meanwhile)
            lua_pushinteger(L, (lua_Integer)column);
            lua_pushinteger(L, (lua_Integer)row);
            lua_pushnumber(L, (lua_Number)*(ptr++));
            Interpreter_call(interpreter, 3, 3);

            size_t dcolumn = LUAX_UNSIGNED(L, -3);
            size_t drow = LUAX_UNSIGNED(L, -2);
            Grid_Object_Value_t dvalue = (Grid_Object_Value_t)LUAX_NUMBER(L, -1);
            data[drow * width + dcolumn] = dvalue;

            lua_pop(L, 3);
        }
    }

    return 0;
}

// TODO: implmement Dijkstra/A* path-finding.
static int grid_path_5onnnn_1t(lua_State *L)
{
    return 0;
}
