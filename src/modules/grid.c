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

#include "../core/luax.h"

#include "../config.h"
#include "../log.h"

#include <stdlib.h>
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif

#ifdef __GRID_INTEGER_CELL__
typedef long Cell_t;
#else
typedef double Cell_t;
#endif

typedef struct _Grid_Class_t {
    int width;
    int height;
    Cell_t *data;
    Cell_t **offsets; // Precomputed pointers to the line of data.
} Grid_Class_t;

static int grid_new(lua_State *L);
static int grid_gc(lua_State *L);
static int grid_width(lua_State *L);
static int grid_height(lua_State *L);
static int grid_fill(lua_State *L);
static int grid_stride(lua_State *L);
static int grid_peek(lua_State *L);
static int grid_poke(lua_State *L);

static const struct luaL_Reg _grid_functions[] = {
    { "new", grid_new },
    {"__gc", grid_gc },
    {"width", grid_width },
    {"height", grid_height },
    {"fill", grid_fill },
    {"stride", grid_stride },
    {"peek", grid_peek },
    {"poke", grid_poke },
//    {"path", grid_path },
    { NULL, NULL }
};

static const luaX_Const _grid_constants[] = {
    { NULL }
};

int grid_loader(lua_State *L)
{
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, NULL, _grid_functions, _grid_constants, nup, LUAX_CLASS(Grid_Class_t));
}

static int grid_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable, luaX_isnumber)
    LUAX_SIGNATURE_END
    int width = lua_tointeger(L, 1);
    int height = lua_tointeger(L, 2);
    int type = lua_type(L, 3);

    Grid_Class_t *instance = (Grid_Class_t *)lua_newuserdata(L, sizeof(Grid_Class_t));

    Cell_t *data = malloc((width * height) * sizeof(Cell_t));
    Cell_t **offsets = malloc(height * sizeof(Cell_t *));

    for (int i = 0; i < height; ++i) { // Precompute the pointers to the data rows for faster access (old-school! :D).
        offsets[i] = data + (i * width);
    }

    Cell_t *ptr = data;
    Cell_t *eod = ptr + (width * height);

    if (type == LUA_TTABLE) {
        lua_pushnil(L); // first key
        while (lua_next(L, 3)) {
#if 0
            const char *key_type = lua_typename(L, lua_type(L, -2)); // uses 'key' (at index -2) and 'value' (at index -1)
#endif
            Cell_t value = (Cell_t)lua_tonumber(L, -1);

            if (ptr < eod) {
                *(ptr++) = value;
            }

            lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
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
            .offsets = offsets
        };

    Log_write(LOG_LEVELS_DEBUG, "<GRID> grid #%p allocated", instance);

    luaL_setmetatable(L, LUAX_CLASS(Grid_Class_t));

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
    free(instance->offsets);

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

    int width = instance->width;
    int height = instance->height;
    Cell_t *data = instance->data;

    Cell_t *ptr = data;
    Cell_t *eod = ptr + (width * height);

    if (type == LUA_TTABLE) {
        lua_pushnil(L); // first key
        while (lua_next(L, 2)) {
#if 0
            const char *key_type = lua_typename(L, lua_type(L, -2)); // uses 'key' (at index -2) and 'value' (at index -1)
#endif
            Cell_t value = (Cell_t)lua_tonumber(L, -1);

            if (ptr < eod) {
                *(ptr++) = value;
            }

            lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
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
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable, luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);
    int column = lua_tointeger(L, 2);
    int row = lua_tointeger(L, 3);
    int type = lua_type(L, 4);
    int amount = lua_tointeger(L, 5);

    int width = instance->width;
    int height = instance->height;
    Cell_t *data = instance->offsets[row];

    Cell_t *ptr = data + column;
    Cell_t *eod = ptr + ((width * height < amount) ? (width * height) : amount);

    if (type == LUA_TTABLE) {
        lua_pushnil(L); // first key
        while (lua_next(L, 4)) {
#if 0
            const char *key_type = lua_typename(L, lua_type(L, -2)); // uses 'key' (at index -2) and 'value' (at index -1)
#endif
            Cell_t value = (Cell_t)lua_tonumber(L, -1);

            if (ptr < eod) {
                *(ptr++) = value;
            }

            lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
        }
    } else
    if (type == LUA_TNUMBER) {
        Cell_t value = (Cell_t)lua_tonumber(L, 4);

        for (int i = 0; (ptr < eod) && (i < amount); ++i) {
            *(ptr++) = value;
        }
    }

    return 0;
}

static int grid_peek(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);
    int column = lua_tointeger(L, 2);
    int row = lua_tointeger(L, 3);

    Cell_t *data = instance->offsets[row];

    Cell_t value = data[column];

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int grid_poke(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Grid_Class_t *instance = (Grid_Class_t *)lua_touserdata(L, 1);
    int column = lua_tointeger(L, 2);
    int row = lua_tointeger(L, 3);
    Cell_t value = (Cell_t)lua_tonumber(L, 4);

    Cell_t *data = instance->offsets[row];

    data[column] = value;

    return 0;
}
