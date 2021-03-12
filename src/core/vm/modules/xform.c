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

#include "xform.h"

#include <config.h>
#include <core/io/display.h>
#include <libs/log.h>
#include <libs/luax.h>
#include <libs/map.h>
#include <libs/stb.h>

#include "udt.h"

#include <math.h>

#ifndef M_PI
  #define M_PI  3.14159265358979323846f
#endif
#ifndef M_PI_2
  #define M_PI_2    1.57079632679489661923
#endif

#define LOG_CONTEXT "xform"
#define META_TABLE  "Tofu_Graphics_XForm_mt"

static int xform_new(lua_State *L);
static int xform_gc(lua_State *L);
static int xform_offset(lua_State *L);
static int xform_matrix(lua_State *L);
static int xform_clamp(lua_State *L);
static int xform_table(lua_State *L);
// TODO: add helper functions to generate common transformations?
static int xform_project(lua_State *L);
static int xform_warp(lua_State *L);

static const struct luaL_Reg _xform_functions[] = {
    { "new", xform_new },
    { "__gc", xform_gc },
    { "offset", xform_offset },
    { "matrix", xform_matrix },
    { "clamp", xform_clamp },
    { "table", xform_table },
    { "project", xform_project },
    { "warp", xform_warp },
    { NULL, NULL }
};

int xform_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _xform_functions, NULL, nup, META_TABLE);
}

static int xform_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    XForm_Object_t *self = (XForm_Object_t *)lua_newuserdatauv(L, sizeof(XForm_Object_t), 1);
    *self = (XForm_Object_t){
            .xform = (GL_XForm_t){
                    .registers = {
                        0.0f, 0.0f, // No offset
                        1.0f, 0.0f, 1.0f, 0.0f, // Identity matrix.
                        0.0f, 0.0f, // No offset
                    },
                    .clamp = GL_XFORM_CLAMP_REPEAT,
                    .table = NULL
                }
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "xform %p allocated for default canvas", self);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int xform_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    XForm_Object_t *self = (XForm_Object_t *)LUAX_USERDATA(L, 1);

    if (self->xform.table) {
        arrfree(self->xform.table);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "xform scan-line table %p freed", self->xform.table);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "xform %p finalized", self);

    return 0;
}

static int xform_offset(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    XForm_Object_t *self = (XForm_Object_t *)LUAX_USERDATA(L, 1);
    float h = LUAX_NUMBER(L, 2);
    float v = LUAX_NUMBER(L, 3);

    GL_XForm_t *xform = &self->xform;
    xform->registers[GL_XFORM_REGISTER_H] = h;
    xform->registers[GL_XFORM_REGISTER_V] = v;

    return 0;
}

static int xform_matrix3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    XForm_Object_t *self = (XForm_Object_t *)LUAX_USERDATA(L, 1);
    float x0 = LUAX_NUMBER(L, 2);
    float y0 = LUAX_NUMBER(L, 3);

    GL_XForm_t *xform = &self->xform;
    xform->registers[GL_XFORM_REGISTER_X] = x0;
    xform->registers[GL_XFORM_REGISTER_Y] = y0;

    return 0;
}

static int xform_matrix5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    XForm_Object_t *self = (XForm_Object_t *)LUAX_USERDATA(L, 1);
    float a = LUAX_NUMBER(L, 2);
    float b = LUAX_NUMBER(L, 3);
    float c = LUAX_NUMBER(L, 4);
    float d = LUAX_NUMBER(L, 5);

    GL_XForm_t *xform = &self->xform;
    xform->registers[GL_XFORM_REGISTER_A] = a;
    xform->registers[GL_XFORM_REGISTER_B] = b;
    xform->registers[GL_XFORM_REGISTER_C] = c;
    xform->registers[GL_XFORM_REGISTER_D] = d;

    return 0;
}

static int xform_matrix7(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    XForm_Object_t *self = (XForm_Object_t *)LUAX_USERDATA(L, 1);
    float a = LUAX_NUMBER(L, 2);
    float b = LUAX_NUMBER(L, 3);
    float c = LUAX_NUMBER(L, 4);
    float d = LUAX_NUMBER(L, 5);
    float x0 = LUAX_NUMBER(L, 6);
    float y0 = LUAX_NUMBER(L, 7);

    GL_XForm_t *xform = &self->xform;
    xform->registers[GL_XFORM_REGISTER_A] = a;
    xform->registers[GL_XFORM_REGISTER_B] = b;
    xform->registers[GL_XFORM_REGISTER_C] = c;
    xform->registers[GL_XFORM_REGISTER_D] = d;
    xform->registers[GL_XFORM_REGISTER_X] = x0;
    xform->registers[GL_XFORM_REGISTER_Y] = y0;

    return 0;
}

static int xform_matrix(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, xform_matrix3)
        LUAX_OVERLOAD_ARITY(5, xform_matrix5)
        LUAX_OVERLOAD_ARITY(7, xform_matrix7)
    LUAX_OVERLOAD_END
}

static int xform_clamp(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    XForm_Object_t *self = (XForm_Object_t *)LUAX_USERDATA(L, 1);
    const char *mode = LUAX_STRING(L, 2);

    GL_XForm_t *xform = &self->xform;
    if (mode[0] == 'e') {
        xform->clamp = GL_XFORM_CLAMP_EDGE;
    } else
    if (mode[0] == 'b') {
        xform->clamp = GL_XFORM_CLAMP_BORDER;
    } else
    if (mode[0] == 'r') {
        xform->clamp = GL_XFORM_CLAMP_REPEAT;
    }

    return 0;
}

static int xform_table1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    XForm_Object_t *self = (XForm_Object_t *)LUAX_USERDATA(L, 1);

    if (self->xform.table) {
        arrfree(self->xform.table);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "scan-line table %p freed", self->xform.table);
    }
    self->xform.table = NULL;

    return 0;
}

static const Map_Entry_t _registers[GL_XForm_Registers_t_CountOf] = { // Need to be sorted for `bsearch()`
    { "a", GL_XFORM_REGISTER_A },
    { "b", GL_XFORM_REGISTER_B },
    { "c", GL_XFORM_REGISTER_C },
    { "d", GL_XFORM_REGISTER_D },
    { "h", GL_XFORM_REGISTER_H },
    { "v", GL_XFORM_REGISTER_V },
    { "x", GL_XFORM_REGISTER_X },
    { "y", GL_XFORM_REGISTER_Y }
};

static int xform_table2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    XForm_Object_t *self = (XForm_Object_t *)LUAX_USERDATA(L, 1);

    GL_XForm_Table_Entry_t *table = NULL;

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        int index = LUAX_INTEGER(L, -2);
        GL_XForm_Table_Entry_t entry = { .scan_line = index - 1 }; // The scan-line indicator is the array index (minus one).

        lua_pushnil(L);
        for (size_t i = 0; lua_next(L, -2); ++i) { // Scan the value, which is an array.
            if (i == GL_XForm_Registers_t_CountOf) {
                Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "too many operations for table entry w/ id #%d", index);
                lua_pop(L, 2);
                break;
            }
            entry.count = i + 1;
            entry.operations[i].id = (GL_XForm_Registers_t)map_find(L, LUAX_STRING(L, -2), _registers, GL_XForm_Registers_t_CountOf)->value;
            entry.operations[i].value = (float)LUAX_NUMBER(L, -1);

            lua_pop(L, 1);
        }

        arrpush(table, entry);

        lua_pop(L, 1);
    }
    arrpush(table, (GL_XForm_Table_Entry_t){ .scan_line = -1 }); // Set the end-of-data (safety) marker

    GL_XForm_t *xform = &self->xform;
    if (xform->table) {
        arrfree(xform->table);
//        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "scan-line table %p reallocated as %p", xform->table, table);
    }
    xform->table = table;

    return 0;
}

static int xform_table(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, xform_table1)
        LUAX_OVERLOAD_ARITY(2, xform_table2)
    LUAX_OVERLOAD_END
}

static int xform_project(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    XForm_Object_t *self = (XForm_Object_t *)LUAX_USERDATA(L, 1);
    int height = LUAX_INTEGER(L, 2);
    float angle = LUAX_NUMBER(L, 3);
    float elevation = LUAX_NUMBER(L, 4);

    const float cos = cosf(angle), sin = sinf(angle);
    const float a = cos, b = sin;
    const float c = -sin, d = cos;

    GL_XForm_Table_Entry_t *table = NULL;

    // SEE: https://www.coranac.com/tonc/text/mode7.htm
    //      https://gamedev.stackexchange.com/questions/24957/doing-an-snes-mode-7-affine-transform-effect-in-pygame
    for (int scan_line = 0; scan_line < height; ++scan_line) {
        const float yc = (float)scan_line;
        const float p = elevation / yc;

        GL_XForm_Table_Entry_t entry = {
                .scan_line = scan_line,
                .operations = {
                        { .id = GL_XFORM_REGISTER_A, .value = a * p },
                        { .id = GL_XFORM_REGISTER_B, .value = b * p },
                        { .id = GL_XFORM_REGISTER_C, .value = c * p },
                        { .id = GL_XFORM_REGISTER_D, .value = d * p }
                    },
                .count = 4
            };

        arrpush(table, entry);
    }
    arrpush(table, (GL_XForm_Table_Entry_t){ .scan_line = -1 }); // Set the end-of-data (safety) marker

    GL_XForm_t *xform = &self->xform;
    if (xform->table) {
        arrfree(xform->table);
//        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "scan-line table %p reallocated as %p", xform->table, table);
    }
    xform->table = table;

    return 0;
}

static int xform_warp(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    XForm_Object_t *self = (XForm_Object_t *)LUAX_USERDATA(L, 1);
    int height = LUAX_INTEGER(L, 2);
    float factor = LUAX_NUMBER(L, 3);

    GL_XForm_Table_Entry_t *table = NULL;

    for (int scan_line = 0; scan_line < height; ++scan_line) {
        const float angle = ((float)scan_line / (float)height) * M_PI; // Could be partially pre-computed, but who cares...
        const float sx = (1.0f - sinf(angle)) * factor + 1.0f;

        GL_XForm_Table_Entry_t entry = {
                .scan_line = scan_line,
                .operations = {
                        { .id = GL_XFORM_REGISTER_Y, .value = scan_line },
                        { .id = GL_XFORM_REGISTER_A, .value = sx },
                        { .id = GL_XFORM_REGISTER_B, .value = 0.0f },
                        { .id = GL_XFORM_REGISTER_C, .value = 0.0f },
                        { .id = GL_XFORM_REGISTER_D, .value = sx }
                    },
                .count = 5
            };

        arrpush(table, entry);
    }
    arrpush(table, (GL_XForm_Table_Entry_t){ .scan_line = -1 }); // Set the end-of-data (safety) marker

    GL_XForm_t *xform = &self->xform;
    if (xform->table) {
        arrfree(xform->table);
//        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "scan-line table %p reallocated as %p", xform->table, table);
    }
    xform->table = table;

    return 0;
}
