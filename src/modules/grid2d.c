/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "grid2d.h"

#include "internal/udt.h"

#include <core/config.h>
#define _LOG_TAG "grid"
#include <libs/log.h>
#include <libs/stb.h>
#include <systems/interpreter.h>

static int grid2d_new_3nnT_1o(lua_State *L);
static int grid2d_gc_1o_0(lua_State *L);
static int grid2d_size_1o_2nn(lua_State *L);
static int grid2d_fill_2ot_0(lua_State *L);
static int grid2d_copy_2oo_0(lua_State *L);
static int grid2d_peek_v_1n(lua_State *L);
static int grid2d_poke_v_0(lua_State *L);
static int grid2d_scan_2of_0(lua_State *L);
static int grid2d_process_2of_0(lua_State *L);
static int grid2d_path_5onnnn_1t(lua_State *L);

int grid2d_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", grid2d_new_3nnT_1o },
            { "__gc", grid2d_gc_1o_0 },
            // -- accessors --
            { "size", grid2d_size_1o_2nn },
            // -- mutators --
            { "fill", grid2d_fill_2ot_0 },
            { "copy", grid2d_copy_2oo_0 },
            { "peek", grid2d_peek_v_1n },
            { "poke", grid2d_poke_v_0 },
            // -- operations --
            { "scan", grid2d_scan_2of_0 },
            { "process", grid2d_process_2of_0 },
            { "path", grid2d_path_5onnnn_1t },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static int grid2d_new_3nnT_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TTABLE)
    LUAX_SIGNATURE_END
    size_t width = LUAX_UNSIGNED(L, 1);
    size_t height = LUAX_UNSIGNED(L, 2);
    int data_table = LUAX_OPTIONAL_TABLE(L, 3, 0);

    size_t data_size = width * height;
    Grid_Object_Value_t *data = malloc(sizeof(Grid_Object_Value_t) * data_size);
    if (!data) {
        return luaL_error(L, "can't allocate %dx%d grid", width, height);
    }

    Grid_Object_t *self = (Grid_Object_t *)udt_newobject(L, sizeof(Grid_Object_t), &(Grid_Object_t){
            .width = width,
            .height = height,
            .data = data,
            .data_size = data_size
        }, OBJECT_TYPE_GRID);

    if (data_table != LUAX_NIL_TABLE) {
        size_t length = lua_rawlen(L, data_table);
        if (length > data_size) {
            return luaL_error(L, "table is too long for grid data (table has %d items, but grid data-size is %d)", length, data_size);
        }

        Grid_Object_Value_t *ptr = data;
        lua_pushnil(L);
        while (lua_next(L, data_table)) {
            Grid_Object_Value_t value = (Grid_Object_Value_t)LUAX_NUMBER(L, -1);

            *(ptr++) = value;

            lua_pop(L, 1);
        }
    } else {
        LOG_W("grid content left uninitialized");
    }

    LOG_D("grid %p allocated w/ data %p", self, data);

    return 1;
}

static int grid2d_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);

    free(self->data);
    LOG_D("data %p freed", self->data);

    LOG_D("grid %p finalized", self);

    return 0;
}

static int grid2d_size_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);

    lua_pushinteger(L, (lua_Integer)self->width);
    lua_pushinteger(L, (lua_Integer)self->height);

    return 2;
}

static int grid2d_fill_2ot_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    int data_table = LUAX_TABLE(L, 2);

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    size_t length = lua_rawlen(L, data_table);
    if (length > self->data_size) {
        return luaL_error(L, "table is too long for grid data (table has %d items, but grid data-size is %d)", length, self->data_size);
    }
#endif  /* TOFU_CORE_DEFENSIVE_CHECKS */

    Grid_Object_Value_t *ptr = self->data;

    lua_pushnil(L);
    while (lua_next(L, data_table)) {
        Grid_Object_Value_t value = (Grid_Object_Value_t)LUAX_NUMBER(L, -1);

        *(ptr++) = value;

        lua_pop(L, 1);
    }

    return 0;
}

static int grid2d_copy_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    const Grid_Object_t *other = (const Grid_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_GRID);

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (self->data_size != other->data_size) {
        return luaL_error(L, "grid data-size don't match");
    }
#endif  /* TOFU_CORE_DEFENSIVE_CHECKS */

    Grid_Object_Value_t *dptr = self->data;
    const Grid_Object_Value_t *sptr = other->data;

    for (size_t i = self->data_size; i; --i) {
        *(dptr++) = *(sptr++);
    }

    return 0;
}

static int grid2d_peek_2on_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t offset = LUAX_UNSIGNED_RANGE(L, 2, 0, self->data_size);

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (offset >= self->data_size) {
        return luaL_error(L, "offset %d is out of range (0, %d)", offset, self->data_size);
    }
#endif  /* TOFU_CORE_DEFENSIVE_CHECKS */

    Grid_Object_Value_t value = self->data[offset];

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int grid2d_peek_3onn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t column = LUAX_UNSIGNED_RANGE(L, 2, 0, self->width);
    size_t row = LUAX_UNSIGNED_RANGE(L, 3, 0, self->height);

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (column >= self->width) {
        return luaL_error(L, "column %d is out of range (0, %d)", column, self->width);
    } else
    if (row >= self->height) {
        return luaL_error(L, "row %d is out of range (0, %d)", row, self->height);
    }
#endif  /* TOFU_CORE_DEFENSIVE_CHECKS */

    Grid_Object_Value_t value = self->data[row * self->width + column];

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int grid2d_peek_v_1n(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(grid2d_peek_2on_1n, 2)
        LUAX_OVERLOAD_BY_ARITY(grid2d_peek_3onn_1n, 3)
    LUAX_OVERLOAD_END
}

static int grid2d_poke_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t offset = LUAX_UNSIGNED_RANGE(L, 2, 0, self->data_size);
    Grid_Object_Value_t value = (Grid_Object_Value_t)LUAX_NUMBER(L, 3);

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (offset >= self->data_size) {
        return luaL_error(L, "offset %d is out of range [0, %d)", offset, self->data_size);
    }
#endif  /* TOFU_CORE_DEFENSIVE_CHECKS */

    self->data[offset] = value;

    return 0;
}

static int grid2d_poke_4onnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    size_t column = LUAX_UNSIGNED_RANGE(L, 2, 0, self->width);
    size_t row = LUAX_UNSIGNED_RANGE(L, 3, 0, self->height);
    Grid_Object_Value_t value = (Grid_Object_Value_t)LUAX_NUMBER(L, 4);

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (column >= self->width) {
        return luaL_error(L, "column %d is out of range [0, %d)", column, self->width);
    } else
    if (row >= self->height) {
        return luaL_error(L, "row %d is out of range [0, %d)", row, self->height);
    }
#endif  /* TOFU_CORE_DEFENSIVE_CHECKS */

    self->data[row * self->width + column] = value;

    return 0;
}

static int grid2d_poke_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(grid2d_poke_3onn_0, 3)
        LUAX_OVERLOAD_BY_ARITY(grid2d_poke_4onnn_0, 4)
    LUAX_OVERLOAD_END
}

static int grid2d_scan_2of_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
    const Grid_Object_t *self = (const Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    int callback_index = LUAX_FUNCTION(L, 2);

    const Interpreter_t *interpreter = (const Interpreter_t *)udt_get_userdata(L, USERDATA_INTERPRETER);

    const Grid_Object_Value_t *data = self->data;

    for (size_t row = 0; row < self->height; ++row) {
        for (size_t column = 0; column < self->width; ++column) {
            lua_pushvalue(L, callback_index); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed meanwhile)
            lua_pushinteger(L, (lua_Integer)column);
            lua_pushinteger(L, (lua_Integer)row);
            lua_pushnumber(L, (lua_Number)*(data++));
            Interpreter_call(interpreter, 3, 0);
        }
    }

    return 0;
}

static int grid2d_process_2of_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
    Grid_Object_t *self = (Grid_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_GRID);
    int callback_index = LUAX_FUNCTION(L, 2);

    const Interpreter_t *interpreter = (const Interpreter_t *)udt_get_userdata(L, USERDATA_INTERPRETER);

    Grid_Object_Value_t *data = self->data;

    const size_t width = self->width;
    const size_t height = self->height;

    const Grid_Object_Value_t *ptr = data;

    for (size_t row = 0; row < height; ++row) {
        for (size_t column = 0; column < width; ++column) {
            lua_pushvalue(L, callback_index); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed meanwhile)
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
static int grid2d_path_5onnnn_1t(lua_State *L)
{
    return 0;
}
