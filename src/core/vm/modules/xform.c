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

#include "xform.h"

#include <config.h>
#include <core/io/display.h>
#include <core/vm/interpreter.h>
#include <libs/fs/fsaux.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "udt.h"
#include "callbacks.h"

#include <math.h>
#include <string.h>

#define LOG_CONTEXT "xform"
#define META_TABLE  "Tofu_Graphics_XForm_mt"

static int xform_new(lua_State *L);
static int xform_gc(lua_State *L);
static int xform_canvas(lua_State *L);
static int xform_blit(lua_State *L);
static int xform_offset(lua_State *L);
static int xform_matrix(lua_State *L);
static int xform_clamp(lua_State *L);
static int xform_table(lua_State *L);

static const struct luaL_Reg _xform_functions[] = {
    { "new", xform_new },
    {"__gc", xform_gc },
    { "canvas", xform_canvas },
    { "blit", xform_blit },
    { "offset", xform_offset },
    { "matrix", xform_matrix },
    { "clamp", xform_clamp },
    { "table", xform_table },
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

    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    XForm_Class_t *instance = (XForm_Class_t *)lua_newuserdata(L, sizeof(XForm_Class_t));
    *instance = (XForm_Class_t){
            .context = display->context,
            .context_reference = LUAX_REFERENCE_NIL,
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
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "xform %p allocated for default canvas", instance);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int xform_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    XForm_Class_t *instance = (XForm_Class_t *)LUAX_REQUIRED_USERDATA(L, 1);

    if (instance->xform.table) {
        arrfree(instance->xform.table);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "xform scan-line table %p freed", instance->xform.table);
    }

    if (instance->context_reference != LUAX_REFERENCE_NIL) {
        luaX_unref(L, instance->context_reference);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context reference #%d released", instance->context_reference);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "xform %p finalized", instance);

    return 0;
}

static int xform_canvas1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    XForm_Class_t *instance = (XForm_Class_t *)LUAX_REQUIRED_USERDATA(L, 1);

    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    if (instance->context_reference != LUAX_REFERENCE_NIL) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context reference #%d released", instance->context_reference);
        luaX_unref(L, instance->context_reference);
    }

    instance->context = display->context;
    instance->context_reference = LUAX_REFERENCE_NIL;
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "default context attached");

    return 0;
}

static int xform_canvas2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    XForm_Class_t *instance = (XForm_Class_t *)LUAX_REQUIRED_USERDATA(L, 1);
    const Canvas_Class_t *canvas = (Canvas_Class_t *)LUAX_REQUIRED_USERDATA(L, 2);

    if (instance->context_reference != LUAX_REFERENCE_NIL) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context reference #%d released", instance->context_reference);
        luaX_unref(L, instance->context_reference);
    }

    instance->context = canvas->context;
    instance->context_reference = luaX_ref(L, 2);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context %p attached w/ reference #%d", instance->context, instance->context_reference);

    return 0;
}

static int xform_canvas(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, xform_canvas1)
        LUAX_OVERLOAD_ARITY(2, xform_canvas2)
    LUAX_OVERLOAD_END
}

static int xform_blit2_4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    XForm_Class_t *instance = (XForm_Class_t *)LUAX_REQUIRED_USERDATA(L, 1); // TODO: rename `instance` to `self`.
    Canvas_Class_t *canvas = (Canvas_Class_t *)LUAX_REQUIRED_USERDATA(L, 2);
    int x = LUAX_OPTIONAL_INTEGER(L, 3, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 4, 0);

    const GL_Context_t *context = instance->context;
    const GL_Surface_t *surface = canvas->context->surface;
    GL_context_blit_x(context, surface, (GL_Point_t){ .x = x, .y = y }, &instance->xform);

    return 0;
}

static int xform_blit(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, xform_blit2_4)
        LUAX_OVERLOAD_ARITY(4, xform_blit2_4) // Nonsense to call it w/ 3 arguments!
    LUAX_OVERLOAD_END
}

static int xform_offset(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    XForm_Class_t *instance = (XForm_Class_t *)LUAX_REQUIRED_USERDATA(L, 1);
    float h = LUAX_REQUIRED_NUMBER(L, 2);
    float v = LUAX_REQUIRED_NUMBER(L, 3);

    instance->xform.registers[GL_XFORM_REGISTER_H] = h;
    instance->xform.registers[GL_XFORM_REGISTER_V] = v;

    return 0;
}

static int xform_matrix3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    XForm_Class_t *instance = (XForm_Class_t *)LUAX_REQUIRED_USERDATA(L, 1);
    float x0 = LUAX_REQUIRED_NUMBER(L, 2);
    float y0 = LUAX_REQUIRED_NUMBER(L, 3);

    instance->xform.registers[GL_XFORM_REGISTER_X] = x0;
    instance->xform.registers[GL_XFORM_REGISTER_Y] = y0;

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
    XForm_Class_t *instance = (XForm_Class_t *)LUAX_REQUIRED_USERDATA(L, 1);
    float a = LUAX_REQUIRED_NUMBER(L, 2);
    float b = LUAX_REQUIRED_NUMBER(L, 3);
    float c = LUAX_REQUIRED_NUMBER(L, 4);
    float d = LUAX_REQUIRED_NUMBER(L, 5);

    instance->xform.registers[GL_XFORM_REGISTER_A] = a;
    instance->xform.registers[GL_XFORM_REGISTER_B] = b;
    instance->xform.registers[GL_XFORM_REGISTER_C] = c;
    instance->xform.registers[GL_XFORM_REGISTER_D] = d;

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
    XForm_Class_t *instance = (XForm_Class_t *)LUAX_REQUIRED_USERDATA(L, 1);
    float a = LUAX_REQUIRED_NUMBER(L, 2);
    float b = LUAX_REQUIRED_NUMBER(L, 3);
    float c = LUAX_REQUIRED_NUMBER(L, 4);
    float d = LUAX_REQUIRED_NUMBER(L, 5);
    float x0 = LUAX_REQUIRED_NUMBER(L, 6);
    float y0 = LUAX_REQUIRED_NUMBER(L, 7);

    instance->xform.registers[GL_XFORM_REGISTER_A] = a;
    instance->xform.registers[GL_XFORM_REGISTER_B] = b;
    instance->xform.registers[GL_XFORM_REGISTER_C] = c;
    instance->xform.registers[GL_XFORM_REGISTER_D] = d;
    instance->xform.registers[GL_XFORM_REGISTER_X] = x0;
    instance->xform.registers[GL_XFORM_REGISTER_Y] = y0;

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
    XForm_Class_t *instance = (XForm_Class_t *)LUAX_REQUIRED_USERDATA(L, 1);
    const char *clamp = lua_tostring(L, 2);

    if (clamp[0] == 'e') {
        instance->xform.clamp = GL_XFORM_CLAMP_EDGE;
    } else
    if (clamp[0] == 'b') {
        instance->xform.clamp = GL_XFORM_CLAMP_BORDER;
    } else
    if (clamp[0] == 'r') {
        instance->xform.clamp = GL_XFORM_CLAMP_REPEAT;
    }

    return 0;
}

static int xform_table1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    XForm_Class_t *instance = (XForm_Class_t *)LUAX_REQUIRED_USERDATA(L, 1);

    if (instance->xform.table) {
        arrfree(instance->xform.table);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "scan-line table %p freed", instance->xform.table);
    }
    instance->xform.table = NULL;

    return 0;
}

static GL_XForm_Registers_t _string_to_register(const char *id) // TODO: move to a bsearched table.
{
    if (id[0] == 'h') {
        return GL_XFORM_REGISTER_H;
    } else
    if (id[0] == 'v') {
        return GL_XFORM_REGISTER_V;
    } else
    if (id[0] == 'a') {
        return GL_XFORM_REGISTER_A;
    } else
    if (id[0] == 'b') {
        return GL_XFORM_REGISTER_B;
    } else
    if (id[0] == 'c') {
        return GL_XFORM_REGISTER_C;
    } else
    if (id[0] == 'd') {
        return GL_XFORM_REGISTER_D;
    } else
    if (id[0] == 'x') {
        return GL_XFORM_REGISTER_X;
    } else
    if (id[0] == 'y') {
        return GL_XFORM_REGISTER_Y;
    }
    Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "unknown register w/ id `%s`", id);
    return GL_XFORM_REGISTER_A;
}

static int xform_table2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    XForm_Class_t *instance = (XForm_Class_t *)LUAX_REQUIRED_USERDATA(L, 1);

    GL_XForm_Table_Entry_t *table = NULL;

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        int index = lua_tointeger(L, -2);
        GL_XForm_Table_Entry_t entry = { 0 };
        entry.scan_line = index - 1; // The scan-line indicator is the array index (minus one).

        lua_pushnil(L);
        for (size_t i = 0; lua_next(L, -2); ++i) { // Scan the value, which is an array.
            if (i == GL_XForm_Registers_t_CountOf) {
                Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "too many operation for table entry w/ id #%d", index);
                lua_pop(L, 2);
                break;
            }
            entry.count = i + 1;
            entry.operations[i].id = lua_isstring(L, -2) ? _string_to_register(lua_tostring(L, -2)) : (GL_XForm_Registers_t)lua_tointeger(L, -2);
            entry.operations[i].value = (float)lua_tonumber(L, -1);

            lua_pop(L, 1);
        }

        arrpush(table, entry);

        lua_pop(L, 1);
    }
    arrpush(table, (GL_XForm_Table_Entry_t){ .scan_line = -1 }); // Set the end-of-data (safety) marker

    if (instance->xform.table) {
        arrfree(instance->xform.table);
//        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "scan-line table %p reallocated as %p", instance->xform.table, table);
    }
    instance->xform.table = table;

    return 0;
}

static int xform_table(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, xform_table1)
        LUAX_OVERLOAD_ARITY(2, xform_table2)
    LUAX_OVERLOAD_END
}
