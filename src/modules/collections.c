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

#include "collections.h"

#include <stdlib.h>

#include "../config.h"
#include "../log.h"

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

static const char *collections_lua =
    "\n"
;

static int collections_grid_new(lua_State *L);
static int collections_grid_gc(lua_State *L);
static int collections_grid_width(lua_State *L);
static int collections_grid_height(lua_State *L);
static int collections_grid_fill(lua_State *L);
static int collections_grid_stride(lua_State *L);
static int collections_grid_peek(lua_State *L);
static int collections_grid_poke(lua_State *L);

static const struct luaL_Reg collections_grid_f[] = {
    { "new", collections_grid_new },
    { NULL, NULL }
};

static const struct luaL_Reg collections_grid_m[] = {
    {"__gc", collections_grid_gc },
    {"width", collections_grid_width },
    {"height", collections_grid_height },
    {"fill", collections_grid_fill },
    {"stride", collections_grid_stride },
    {"peek", collections_grid_peek },
    {"poke", collections_grid_poke },
    { NULL, NULL }
};

static const luaX_Const collections_grid_c[] = {
    { NULL }
};

static int luaopen_collections(lua_State *L)
{
    lua_newtable(L);

    luaX_newclass(L, collections_grid_f, collections_grid_m, collections_grid_c, LUAX_CLASS(Grid_Class_t));
    lua_setfield(L, -2, "Grid");

    return 1;
}

bool collections_initialize(lua_State *L)
{
    luaX_preload(L, "collections", luaopen_collections);

    if (luaL_dostring(L, collections_lua) != 0) {
        Log_write(LOG_LEVELS_FATAL, "<VM> can't open script: %s", lua_tostring(L, -1));
        return false;
    }

    return true;
}

static int collections_grid_new(lua_State *L)
{
    if (lua_gettop(L) != 3) {
        return luaL_error(L, "<COLLECTIONS> grid constructor requires 3 arguments");
    }
    int width = luaL_checkinteger(L, 1);
    int height = luaL_checkinteger(L, 2);
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
        while (lua_next(L, 3) != 0) {
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
        Cell_t value = (Cell_t)lua_tonumber(L, 3);;

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

    Log_write(LOG_LEVELS_DEBUG, "<COLLECTIONS> grid #%p allocated", instance);

    luaL_setmetatable(L, LUAX_CLASS(Grid_Class_t));

    return 1;
}

static int collections_grid_gc(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<COLLECTIONS> method requires 1 arguments");
    }
    Grid_Class_t *instance = (Grid_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Grid_Class_t));

    Log_write(LOG_LEVELS_DEBUG, "<COLLECTIONS> finalizing grid #%p", instance);

    free(instance->data);
    free(instance->offsets);

    return 0;
}

static int collections_grid_width(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<COLLECTIONS> method requires 1 arguments");
    }
    Grid_Class_t *instance = (Grid_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Grid_Class_t));

    lua_pushinteger(L, instance->width);

    return 1;
}

static int collections_grid_height(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<COLLECTIONS> method requires 1 arguments");
    }
    Grid_Class_t *instance = (Grid_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Grid_Class_t));

    lua_pushinteger(L, instance->height);

    return 1;
}

static int collections_grid_fill(lua_State *L)
{
    if (lua_gettop(L) != 2) {
        return luaL_error(L, "<COLLECTIONS> method requires 2 arguments");
    }
    Grid_Class_t *instance = (Grid_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Grid_Class_t));
    int type = lua_type(L, 2);

    int width = instance->width;
    int height = instance->height;
    Cell_t *data = instance->data;

    Cell_t *ptr = data;
    Cell_t *eod = ptr + (width * height);

    if (type == LUA_TTABLE) {
        lua_pushnil(L); // first key
        while (lua_next(L, 1) != 0) {
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
        Cell_t value = (Cell_t)lua_tonumber(L, 1);;

        while (ptr < eod) {
            *(ptr++) = value;
        }
    }

    return 0;
}

static int collections_grid_stride(lua_State *L)
{
    if (lua_gettop(L) != 5) {
        return luaL_error(L, "<COLLECTIONS> method requires 5 arguments");
    }
    Grid_Class_t *instance = (Grid_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Grid_Class_t));
    int column = luaL_checkinteger(L, 2);
    int row = luaL_checkinteger(L, 3);
    int type = lua_type(L, 4);
    int amount = luaL_checkinteger(L, 5);

    int width = instance->width;
    int height = instance->height;
    Cell_t *data = instance->offsets[row];

    Cell_t *ptr = data + column;
    Cell_t *eod = ptr + ((width * height < amount) ? (width * height) : amount);

    if (type == LUA_TTABLE) {
        lua_pushnil(L); // first key
        while (lua_next(L, 1) != 0) {
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
        Cell_t value = (Cell_t)lua_tonumber(L, 1);;

        for (int i = 0; (ptr < eod) && (i < amount); ++i) {
            *(ptr++) = value;
        }
    }

    return 0;
}

static int collections_grid_peek(lua_State *L)
{
    if (lua_gettop(L) != 3) {
        return luaL_error(L, "<COLLECTIONS> method requires 3 arguments");
    }
    Grid_Class_t *instance = (Grid_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Grid_Class_t));
    int column = luaL_checkinteger(L, 2);
    int row = luaL_checkinteger(L, 3);

    Cell_t *data = instance->offsets[row];

    Cell_t value = data[column];

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int collections_grid_poke(lua_State *L)
{
    if (lua_gettop(L) != 4) {
        return luaL_error(L, "<COLLECTIONS> method requires 4 arguments");
    }
    Grid_Class_t *instance = (Grid_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Grid_Class_t));
    int column = luaL_checkinteger(L, 2);
    int row = luaL_checkinteger(L, 3);
    Cell_t value = (Cell_t)luaL_checknumber(L, 4);

    Cell_t *data = instance->offsets[row];

    data[column] = value;

    return 0;
}
