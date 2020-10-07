/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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

static int grid_new(lua_State *L);
static int grid_gc(lua_State *L);
static int grid_size(lua_State *L);
static int grid_fill(lua_State *L);
static int grid_stride(lua_State *L);
static int grid_peek(lua_State *L);
static int grid_poke(lua_State *L);
static int grid_scan(lua_State *L);
static int grid_process(lua_State *L);

static const struct luaL_Reg _grid_functions[] = {
    { "new", grid_new },
    { "__gc", grid_gc },
    {"size", grid_size },
    {"fill", grid_fill },
    {"stride", grid_stride },
    {"peek", grid_peek },
    {"poke", grid_poke },
    {"scan", grid_scan },
    {"process", grid_process },
//    {"path", grid_path },
    { NULL, NULL }
};

static const unsigned char _grid_lua[] = {
#include "grid.inc"
};

static luaX_Script _grid_script = { (const char *)_grid_lua, sizeof(_grid_lua), "@grid.lua" }; // Trace as filename internally.

int grid_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_grid_script, _grid_functions, NULL, nup, META_TABLE);
}

static int grid_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE, LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t width = (size_t)LUAX_INTEGER(L, 1);
    size_t height = (size_t)LUAX_INTEGER(L, 2);
    int type = lua_type(L, 3);

    size_t data_size = width * height;
    Cell_t *data = malloc(data_size * sizeof(Cell_t));
    if (!data) {
        return luaL_error(L, "can't allocate %dx%d grid", width, height);
    }

    Cell_t *ptr = data;
    Cell_t *eod = ptr + data_size;
    if (type == LUA_TTABLE) {
        lua_pushnil(L);
        while (lua_next(L, 3)) {
            if (ptr == eod) {
                lua_pop(L, 2);
                break;
            }

            Cell_t value = (Cell_t)LUAX_NUMBER(L, -1);
            *(ptr++) = value;

            lua_pop(L, 1);
        }
    } else
    if (type == LUA_TNUMBER) {
        Cell_t value = (Cell_t)LUAX_NUMBER(L, 3);

        while (ptr < eod) {
            *(ptr++) = value;
        }
    }

    Grid_Object_t *self = (Grid_Object_t *)lua_newuserdatauv(L, sizeof(Grid_Object_t), 1);
    *self = (Grid_Object_t){
            .width = width,
            .height = height,
            .data = data,
            .data_size = data_size
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "grid %p allocated w/ data %p", self, data);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int grid_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_USERDATA(L, 1);

    free(self->data);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "data %p freed", self->data);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "grid %p finalized", self);

    return 0;
}

static int grid_size(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushinteger(L, self->width);
    lua_pushinteger(L, self->height);

    return 2;
}

static int grid_fill(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE, LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_USERDATA(L, 1);
    int type = lua_type(L, 2);

    Cell_t *ptr = self->data;
    Cell_t *eod = ptr + self->data_size;

    if (type == LUA_TTABLE) {
        lua_pushnil(L);
        while (lua_next(L, 2)) {
            if (ptr == eod) {
                lua_pop(L, 2);
                break;
            }

            Cell_t value = (Cell_t)LUAX_NUMBER(L, -1);
            *(ptr++) = value;

            lua_pop(L, 1);
        }
    } else
    if (type == LUA_TNUMBER) {
        Cell_t value = (Cell_t)LUAX_NUMBER(L, 2);

        while (ptr < eod) {
            *(ptr++) = value;
        }
    }

    return 0;
}

static int grid_stride(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE, LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_USERDATA(L, 1);
    size_t column = (size_t)LUAX_INTEGER(L, 2);
    size_t row = (size_t)LUAX_INTEGER(L, 3);
    int type = lua_type(L, 4);
    size_t amount = (size_t)LUAX_INTEGER(L, 5);
#ifdef DEBUG
    if (column >= self->width) {
        return luaL_error(L, "column %d is out of range (0, %d)", column, self->width);
    } else
    if (row >= self->height) {
        return luaL_error(L, "row %d is out of range (0, %d)", row, self->height);
    }
#endif

    Cell_t *ptr = self->data + row * self->width + column;
    Cell_t *eod = ptr + (self->data_size < amount ? self->data_size : amount);

    if (type == LUA_TTABLE) {
        lua_pushnil(L);
        while (lua_next(L, 4)) {
            if (ptr == eod) {
                lua_pop(L, 2);
                break;
            }

            Cell_t value = (Cell_t)LUAX_NUMBER(L, -1);
            *(ptr++) = value;

            lua_pop(L, 1);
        }
    } else
    if (type == LUA_TNUMBER) {
        Cell_t value = (Cell_t)LUAX_NUMBER(L, 4);

        for (size_t i = 0; (ptr < eod) && (i < amount); ++i) {
            *(ptr++) = value;
        }
    }

    return 0;
}

static int grid_peek(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_USERDATA(L, 1);
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

static int grid_poke(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_USERDATA(L, 1);
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

static int grid_scan(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_USERDATA(L, 1);
//    luaX_Reference callback = luaX_tofunction(L, 2);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const Cell_t *data = self->data;

    for (size_t row = 0; row < self->height; ++row) {
        for (size_t column = 0; column < self->width; ++column) {
            lua_pushvalue(L, 2); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed meanwhile)
            lua_pushinteger(L, column);
            lua_pushinteger(L, row);
            lua_pushnumber(L, *(data++));
            Interpreter_call(interpreter, 3, 0);
        }
    }

    return 0;
}

static int grid_process(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_USERDATA(L, 1);
//    luaX_Reference callback = luaX_tofunction(L, 2);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    Cell_t *data = self->data;

    const size_t width = self->width;
    const size_t height = self->height;

    const Cell_t *ptr = data;

    for (size_t row = 0; row < height; ++row) {
        for (size_t column = 0; column < width; ++column) {
            lua_pushvalue(L, 2); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed meanwhile)
            lua_pushinteger(L, column);
            lua_pushinteger(L, row);
            lua_pushnumber(L, *(ptr++));
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
