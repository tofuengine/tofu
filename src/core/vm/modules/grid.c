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

#include "grid.h"

#include <config.h>
#include <core/vm/interpreter.h>
#include <libs/log.h>
#include <libs/luax.h>
#include <libs/stb.h>

#include "udt.h"

#define LOG_CONTEXT "grid"
#define META_TABLE  "Tofu_Collections_Grid_mt"
#define SCRIPT_NAME "@grid.lua"

static int grid_new_3nnt_1u(lua_State *L);
static int grid_gc_1u_0(lua_State *L);
static int grid_size_1u_2nn(lua_State *L);
static int grid_fill_2ut_0(lua_State *L);
static int grid_copy_2uu_0(lua_State *L);
static int grid_peek_v_1n(lua_State *L);
static int grid_poke_v_0(lua_State *L);
static int grid_scan_2uf_0(lua_State *L);
static int grid_process_2uf_0(lua_State *L);

static const char _grid_lua[] = {
#include "grid.inc"
};

int grid_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){
            .buffer = _grid_lua,
            .size = sizeof(_grid_lua) / sizeof(char),
            .name = SCRIPT_NAME
        },
        (const struct luaL_Reg[]){
            { "new", grid_new_3nnt_1u },
            { "__gc", grid_gc_1u_0 },
            { "size", grid_size_1u_2nn },
            { "fill", grid_fill_2ut_0 },
            { "copy", grid_copy_2uu_0 },
            { "peek", grid_peek_v_1n },
            { "poke", grid_poke_v_0 },
            { "scan", grid_scan_2uf_0 },
            { "process", grid_process_2uf_0 },
//            { "path", grid_path },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int grid_new_3nnt_1u(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    size_t width = (size_t)LUAX_INTEGER(L, 1);
    size_t height = (size_t)LUAX_INTEGER(L, 2);

    size_t data_size = width * height;
    Cell_t *data = malloc(sizeof(Cell_t) * data_size);
    if (!data) {
        return luaL_error(L, "can't allocate %dx%d grid", width, height);
    }

    Cell_t *ptr = data;

    size_t length = lua_rawlen(L, 3);
    if (length > 0) {
        for (size_t i = 0; i < data_size; ++i) {
            size_t index = ((i % length) + 1);
            lua_rawgeti(L, 3, index);

            Cell_t value = (Cell_t)LUAX_NUMBER(L, -1);
            *(ptr++) = value;

            lua_pop(L, 1);
        }
    } else {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "grid content left uninitialized");
    }

    Grid_Object_t *self = (Grid_Object_t *)luaX_newobject(L, sizeof(Grid_Object_t), &(Grid_Object_t){
            .width = width,
            .height = height,
            .data = data,
            .data_size = data_size
        }, OBJECT_TYPE_GRID);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "grid %p allocated w/ data %p", self, data);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int grid_gc_1u_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);

    free(self->data);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "data %p freed", self->data);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "grid %p finalized", self);

    return 0;
}

static int grid_size_1u_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);

    lua_pushinteger(L, (lua_Integer)self->width);
    lua_pushinteger(L, (lua_Integer)self->height);

    return 2;
}

static int grid_fill_2ut_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);

    size_t length = lua_rawlen(L, 2);
    if (length == 0) {
        return luaL_error(L, "table can't be empty");
    }

    Cell_t *ptr = self->data;

    for (size_t i = 0; i < self->data_size; ++i) {
        size_t index = ((i % length) + 1);
        lua_rawgeti(L, 2, index);

        Cell_t value = (Cell_t)LUAX_NUMBER(L, -1);
        *(ptr++) = value;

        lua_pop(L, 1);
    }

    return 0;
}

static int grid_copy_2uu_0(lua_State *L)
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

    Cell_t *dptr = self->data;
    const Cell_t *sptr = other->data;

    for (size_t i = self->data_size; i; --i) {
        *(dptr++) = *(sptr++);
    }

    return 0;
}

static int grid_peek_2un_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t offset = (size_t)LUAX_INTEGER(L, 2);
#ifdef DEBUG
    if (offset >= self->data_size) {
        return luaL_error(L, "offset %d is out of range (0, %d)", offset, self->data_size);
    }
#endif

    Cell_t value = self->data[offset];

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int grid_peek_3unn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t column = (size_t)LUAX_INTEGER(L, 2);
    size_t row = (size_t)LUAX_INTEGER(L, 3);
#ifdef DEBUG
    if (column >= self->width) {
        return luaL_error(L, "column %d is out of range (0, %d)", column, self->width);
    } else
    if (row >= self->height) {
        return luaL_error(L, "row %d is out of range (0, %d)", row, self->height);
    }
#endif

    Cell_t value = self->data[row * self->width + column];

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int grid_peek_v_1n(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, grid_peek_2un_1n)
        LUAX_OVERLOAD_ARITY(3, grid_peek_3unn_1n)
    LUAX_OVERLOAD_END
}

static int grid_poke_3unn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t offset = (size_t)LUAX_INTEGER(L, 2);
    Cell_t value = (Cell_t)LUAX_NUMBER(L, 3);
#ifdef DEBUG
    if (offset >= self->data_size) {
        return luaL_error(L, "offset %d is out of range (0, %d)", offset, self->data_size);
    }
#endif

    self->data[offset] = value;

    return 0;
}

static int grid_poke_4unnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t column = (size_t)LUAX_INTEGER(L, 2);
    size_t row = (size_t)LUAX_INTEGER(L, 3);
    Cell_t value = (Cell_t)LUAX_NUMBER(L, 4);
#ifdef DEBUG
    if (column >= self->width) {
        return luaL_error(L, "column %d is out of range (0, %d)", column, self->width);
    } else
    if (row >= self->height) {
        return luaL_error(L, "row %d is out of range (0, %d)", row, self->height);
    }
#endif

    self->data[row * self->width + column] = value;

    return 0;
}

static int grid_poke_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, grid_poke_3unn_0)
        LUAX_OVERLOAD_ARITY(4, grid_poke_4unnn_0)
    LUAX_OVERLOAD_END
}

static int grid_scan_2uf_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
//    luaX_Reference callback = luaX_tofunction(L, 2);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const Cell_t *data = self->data;

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

static int grid_process_2uf_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
//    luaX_Reference callback = luaX_tofunction(L, 2);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    Cell_t *data = self->data;

    const size_t width = self->width;
    const size_t height = self->height;

    const Cell_t *ptr = data;

    for (size_t row = 0; row < height; ++row) {
        for (size_t column = 0; column < width; ++column) {
            lua_pushvalue(L, 2); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed meanwhile)
            lua_pushinteger(L, (lua_Integer)column);
            lua_pushinteger(L, (lua_Integer)row);
            lua_pushnumber(L, (lua_Number)*(ptr++));
            Interpreter_call(interpreter, 3, 3);

            size_t dcolumn = (size_t)LUAX_INTEGER(L, -3);
            size_t drow = (size_t)LUAX_INTEGER(L, -2);
            Cell_t dvalue = (Cell_t)LUAX_NUMBER(L, -1);
            data[drow * width + dcolumn] = dvalue;

            lua_pop(L, 3);
        }
    }

    return 0;
}
