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

#include "bank.h"

#include <config.h>
#include <core/io/display.h>
#include <core/vm/interpreter.h>
#include <libs/log.h>
#include <libs/gl/gl.h>
#include <libs/stb.h>

#include "udt.h"
#include "callbacks.h"

#include <math.h>
#include <string.h>

#define LOG_CONTEXT "bank"

#define BANK_MT     "Tofu_Bank_mt"

static int bank_new(lua_State *L);
static int bank_gc(lua_State *L);
static int bank_cell_width(lua_State *L);
static int bank_cell_height(lua_State *L);
static int bank_blit(lua_State *L);

static const struct luaL_Reg _bank_functions[] = {
    { "new", bank_new },
    {"__gc", bank_gc },
    { "cell_width", bank_cell_width },
    { "cell_height", bank_cell_height },
    { "blit", bank_blit },
    { NULL, NULL }
};

static const luaX_Const _bank_constants[] = {
    { NULL }
};

int bank_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _bank_functions, _bank_constants, nup, BANK_MT);
}

static int bank_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING, LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);
    size_t cell_width = (size_t)lua_tointeger(L, 2);
    size_t cell_height = (size_t)lua_tointeger(L, 3);

    const File_System_t *file_system = (const File_System_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));
    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Sheet_t sheet;

    if (type == LUA_TSTRING) {
        const char *file = lua_tostring(L, 1);

        File_System_Chunk_t chunk = FS_load(file_system, file, FILE_SYSTEM_CHUNK_IMAGE);
        if (chunk.type == FILE_SYSTEM_CHUNK_NULL) {
            return luaL_error(L, "can't load file `%s`", file);
        }
        GL_sheet_fetch(&sheet, (GL_Image_t){ .width = chunk.var.image.width, .height = chunk.var.image.height, .data = chunk.var.image.pixels }, cell_width, cell_height, surface_callback_palette, (void *)&display->palette);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet `%s` loaded", file);
        FS_release(chunk);
    } else
    if (type == LUA_TUSERDATA) {
        const Surface_Class_t *instance = (const Surface_Class_t *)lua_touserdata(L, 1);

        GL_sheet_attach(&sheet, &instance->surface, cell_width, cell_height);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached", instance);
    }

    Bank_Class_t *instance = (Bank_Class_t *)lua_newuserdata(L, sizeof(Bank_Class_t));
    *instance = (Bank_Class_t){
            .sheet = sheet,
            .owned = type == LUA_TSTRING ? true : false
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank allocated as %p", instance);

    luaL_setmetatable(L, BANK_MT);

    return 1;
}

static int bank_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);

    if (instance->owned) {
        GL_sheet_delete(&instance->sheet);
    } else {
        GL_sheet_detach(&instance->sheet);
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p finalized", instance);

    return 0;
}

static int bank_cell_width(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->sheet.size.width);

    return 1;
}

static int bank_cell_height(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->sheet.size.height);

    return 1;
}

static int bank_blit4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);
    int cell_id = lua_tointeger(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);

    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y });

    return 0;
}

static int bank_blit5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);
    int cell_id = lua_tointeger(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    int rotation = lua_tointeger(L, 5);

    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit_sr(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, 1.0f, 1.0f, rotation, 0.5f, 0.5f);

    return 0;
}

static int bank_blit6(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 6)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);
    int cell_id = lua_tointeger(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    float scale_x = lua_tonumber(L, 5);
    float scale_y = lua_tonumber(L, 6);

    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit_s(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale_x, scale_y);

    return 0;
}

static int bank_blit7(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 7)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);
    int cell_id = lua_tointeger(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    float scale_x = lua_tonumber(L, 5);
    float scale_y = lua_tonumber(L, 6);
    int rotation = lua_tointeger(L, 7);

    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit_sr(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale_x, scale_y, rotation, 0.5f, 0.5f);

    return 0;
}

static int bank_blit9(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 9)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);
    int cell_id = lua_tointeger(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    float scale_x = lua_tonumber(L, 5);
    float scale_y = lua_tonumber(L, 6);
    int rotation = lua_tointeger(L, 7);
    float anchor_x = lua_tonumber(L, 8);
    float anchor_y = lua_tonumber(L, 9);

    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit_sr(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale_x, scale_y, rotation, anchor_x, anchor_y);

    return 0;
}

static int bank_blit(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(4, bank_blit4)
        LUAX_OVERLOAD_ARITY(5, bank_blit5)
        LUAX_OVERLOAD_ARITY(6, bank_blit6)
        LUAX_OVERLOAD_ARITY(7, bank_blit7)
        LUAX_OVERLOAD_ARITY(9, bank_blit9)
    LUAX_OVERLOAD_END
}
