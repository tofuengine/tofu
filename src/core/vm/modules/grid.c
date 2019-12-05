/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "grid.h"

#include <config.h>
#include <core/vm/interpreter.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "udt.h"

#include <stdlib.h>

#define GRID_MT        "Tofu_Grid_mt"

static int grid_new(lua_State *L);
static int grid_gc(lua_State *L);
static int grid_width(lua_State *L);
static int grid_height(lua_State *L);
static int grid_fill(lua_State *L);
static int grid_stride(lua_State *L);
static int grid_peek(lua_State *L);
static int grid_poke(lua_State *L);
static int grid_scan(lua_State *L);
static int grid_process(lua_State *L);

static const struct luaL_Reg _grid_functions[] = {
    { "new", grid_new },
    {"__gc", grid_gc },
    {"width", grid_width },
    {"height", grid_height },
    {"fill", grid_fill },
    {"stride", grid_stride },
    {"peek", grid_peek },
    {"poke", grid_poke },
    {"scan", grid_scan },
    {"process", grid_process },
//    {"path", grid_path },
    { NULL, NULL }
};

static const luaX_Const _grid_constants[] = {
    { NULL }
};

static const unsigned char _grid_lua[] = {
#include "grid.inc"
};

static luaX_Script _grid_script = { (const char *)_grid_lua, sizeof(_grid_lua), "@grid.lua" }; // Trace as filename internally.

int grid_loader(lua_State *L)
{
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, &_grid_script, _grid_functions, _grid_constants, nup, GRID_MT);
}

static int grid_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable, luaX_isnumber)
    LUAX_SIGNATURE_END
    size_t width = (size_t)lua_tointeger(L, 1);
    size_t height = (size_t)lua_tointeger(L, 2);
    int type = lua_type(L, 3);

    Grid_Class_t *instance = (Grid_Class_t *)lua_newuserdata(L, sizeof(Grid_Class_t));

    size_t data_size = width * height;
    Cell_t *data = malloc(data_size * sizeof(Cell_t));
    Cell_t **data_rows = malloc(height * sizeof(Cell_t *));

    if (!data || !data_rows) {
        return luaL_error(L, "<GRID> can't allocate memory");
    }

    for (size_t i = 0; i < height; ++i) { // Precompute the pointers to the data rows for faster access (old-school! :D).
        data_rows[i] = data + (i * width);
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

            Cell_t value = (Cell_t)lua_tonumber(L, -1);
            *(ptr++) = value;

            lua_pop(L, 1);
        }
    } else
    if (type == LUA_TNUMBER) {
        Cell_t value = (Cell_t)lua_tonumber(L, 3);

        while (ptr < eod) {
            *(ptr++) = value;
        }
    }

    *instance = (Grid_Class_t){
            .width = width,
            .height = height,
            .data = data,
            .data_rows = data_rows,
            .data_size = data_size
        };

    Log_write(LOG_LEVELS_DEBUG, "<GRID> grid #%p allocated", instance);

    luaL_setmetatable(L, GRID_MT);

    return 1;
}

static int grid_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);

    Log_write(LOG_LEVELS_DEBUG, "<GRID> finalizing grid #%p", instance);

    free(instance->data);
    free(instance->data_rows);

    return 0;
}

static int grid_width(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->width);

    return 1;
}

static int grid_height(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->height);

    return 1;
}

static int grid_fill(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable, luaX_isnumber)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);
    int type = lua_type(L, 2);

    Cell_t *ptr = instance->data;
    Cell_t *eod = ptr + instance->data_size;

    if (type == LUA_TTABLE) {
        lua_pushnil(L);
        while (lua_next(L, 2)) {
            if (ptr == eod) {
                lua_pop(L, 2);
                break;
            }

            Cell_t value = (Cell_t)lua_tonumber(L, -1);
            *(ptr++) = value;

            lua_pop(L, 1);
        }
    } else
    if (type == LUA_TNUMBER) {
        Cell_t value = (Cell_t)lua_tonumber(L, 2);

        while (ptr < eod) {
            *(ptr++) = value;
        }
    }

    return 0;
}

static int grid_stride(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable, luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);
    size_t column = (size_t)lua_tointeger(L, 2);
    size_t row = (size_t)lua_tointeger(L, 3);
    int type = lua_type(L, 4);
    size_t amount = (size_t)lua_tointeger(L, 5);
#ifdef DEBUG
    if (column >= instance->width) {
        return luaL_error(L, "column %d is out of range (0, %d)", column, instance->width);
    } else
    if (row >= instance->height) {
        return luaL_error(L, "row %d is out of range (0, %d)", row, instance->height);
    }
#endif

    Cell_t *ptr = instance->data_rows[row] + column;
    Cell_t *eod = ptr + (instance->data_size < amount ? instance->data_size : amount);

    if (type == LUA_TTABLE) {
        lua_pushnil(L);
        while (lua_next(L, 4)) {
            if (ptr == eod) {
                lua_pop(L, 2);
                break;
            }

            Cell_t value = (Cell_t)lua_tonumber(L, -1);
            *(ptr++) = value;

            lua_pop(L, 1);
        }
    } else
    if (type == LUA_TNUMBER) {
        Cell_t value = (Cell_t)lua_tonumber(L, 4);

        for (size_t i = 0; (ptr < eod) && (i < amount); ++i) {
            *(ptr++) = value;
        }
    }

    return 0;
}

static int grid_peek(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);
    size_t column = (size_t)lua_tointeger(L, 2);
    size_t row = (size_t)lua_tointeger(L, 3);
#ifdef DEBUG
    if (column >= instance->width) {
        return luaL_error(L, "column %d is out of range (0, %d)", column, instance->width);
    } else
    if (row >= instance->height) {
        return luaL_error(L, "row %d is out of range (0, %d)", row, instance->height);
    }
#endif

    Cell_t *data = instance->data_rows[row];

    Cell_t value = data[column];

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int grid_poke(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);
    size_t column = (size_t)lua_tointeger(L, 2);
    size_t row = (size_t)lua_tointeger(L, 3);
    Cell_t value = (Cell_t)lua_tonumber(L, 4);
#ifdef DEBUG
    if (column >= instance->width) {
        return luaL_error(L, "column %d is out of range (0, %d)", column, instance->width);
    } else
    if (row >= instance->height) {
        return luaL_error(L, "row %d is out of range (0, %d)", row, instance->height);
    }
#endif

    Cell_t *data = instance->data_rows[row];

    data[column] = value;

    return 0;
}

static int grid_scan(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isfunction)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);
//    luaX_Reference callback = luaX_tofunction(L, 2);

    Interpreter_t *interpreter = (Interpreter_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const Cell_t *data = instance->data;

    for (size_t row = 0; row < instance->height; ++row) {
        for (size_t column = 0; column < instance->width; ++column) {
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
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isfunction)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);
//    luaX_Reference callback = luaX_tofunction(L, 2);

    Interpreter_t *interpreter = (Interpreter_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const Cell_t *data = instance->data;

    for (size_t row = 0; row < instance->height; ++row) {
        for (size_t column = 0; column < instance->width; ++column) {
            lua_pushvalue(L, 2); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed meanwhile)
            lua_pushinteger(L, column);
            lua_pushinteger(L, row);
            lua_pushnumber(L, *(data++));
            Interpreter_call(interpreter, 3, 3);

            size_t column = lua_tointeger(L, -3);
            size_t row = lua_tointeger(L, -2);
            Cell_t value = lua_tonumber(L, -1);
            instance->data_rows[row][column] = value;

            lua_pop(L, 3);
        }
    }

    return 0;
}
