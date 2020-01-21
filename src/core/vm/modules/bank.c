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

#include "bank.h"

#include <config.h>
#include <core/io/display.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "callbacks.h"
#include "udt.h"

#include <math.h>

#define LOG_CONTEXT "bank"
#define META_TABLE  "Tofu_Graphics_Bank_mt"

static int bank_new(lua_State *L);
static int bank_gc(lua_State *L);
static int bank_size(lua_State *L);
static int bank_blit(lua_State *L);

static const struct luaL_Reg _bank_functions[] = {
    { "new", bank_new },
    {"__gc", bank_gc },
    { "size", bank_size },
    { "blit", bank_blit },
    { NULL, NULL }
};

int bank_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _bank_functions, NULL, nup, META_TABLE);
}

static int bank_new3(lua_State *L)
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

    GL_Sheet_t *sheet = NULL;
    if (type == LUA_TSTRING) {
        const char *file = lua_tostring(L, 1);

        File_System_Chunk_t chunk = FSaux_load(file_system, file, FILE_SYSTEM_CHUNK_BLOB);
        if (chunk.type == FILE_SYSTEM_CHUNK_NULL) {
            return luaL_error(L, "can't load file `%s`", file);
        }
        sheet = GL_sheet_decode(chunk.var.blob.ptr, chunk.var.blob.size, cell_width, cell_height, surface_callback_palette, (void *)&display->palette);
        FSaux_release(chunk);
        if (!sheet) {
            return luaL_error(L, "can't decode %d bytes sheet", chunk.var.blob.size);
        }
    } else
    if (type == LUA_TUSERDATA) {
        const Canvas_Class_t *canvas = (const Canvas_Class_t *)lua_touserdata(L, 1);

        sheet = GL_sheet_fetch(canvas->context->surface, cell_width, cell_height);
        if (!sheet) {
            return luaL_error(L, "can't fetch sheet");
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p fetched from canvas %p", sheet, canvas);
    }

    Bank_Class_t *instance = (Bank_Class_t *)lua_newuserdata(L, sizeof(Bank_Class_t));
    *instance = (Bank_Class_t){
            .context = display->context,
            .sheet = sheet,
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p allocated w/ sheet %p for default context", instance, sheet);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int bank_new4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING, LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Class_t *canvas = (const Canvas_Class_t *)lua_touserdata(L, 1);
    int type = lua_type(L, 2);
    size_t cell_width = (size_t)lua_tointeger(L, 3);
    size_t cell_height = (size_t)lua_tointeger(L, 4);

    const File_System_t *file_system = (const File_System_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));
    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Sheet_t *sheet = NULL;
    if (type == LUA_TSTRING) {
        const char *file = lua_tostring(L, 2);

        File_System_Chunk_t chunk = FSaux_load(file_system, file, FILE_SYSTEM_CHUNK_BLOB);
        if (chunk.type == FILE_SYSTEM_CHUNK_NULL) {
            return luaL_error(L, "can't load file `%s`", file);
        }
        sheet = GL_sheet_decode(chunk.var.blob.ptr, chunk.var.blob.size, cell_width, cell_height, surface_callback_palette, (void *)&display->palette);
        FSaux_release(chunk);
        if (!sheet) {
            return luaL_error(L, "can't decode %d bytes sheet", chunk.var.blob.size);
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p loaded from file `%s`", sheet, file);
    } else
    if (type == LUA_TUSERDATA) {
        const Canvas_Class_t *canvas = (const Canvas_Class_t *)lua_touserdata(L, 2);

        sheet = GL_sheet_fetch(canvas->context->surface, cell_width, cell_height);
        if (!sheet) {
            return luaL_error(L, "can't fetch sheet");
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p fetched from canvas %p", sheet, canvas);
    }

    Bank_Class_t *instance = (Bank_Class_t *)lua_newuserdata(L, sizeof(Bank_Class_t));
    *instance = (Bank_Class_t){
            .context = canvas->context, // TODO: should lock reference to canvas?
            .sheet = sheet
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p allocated w/ sheet %p for context %p", instance, sheet, canvas->context);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int bank_new(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, bank_new3)
        LUAX_OVERLOAD_ARITY(4, bank_new4)
    LUAX_OVERLOAD_END
}

static int bank_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);

    GL_sheet_destroy(instance->sheet);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p destroyed", instance->sheet);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p finalized", instance);

    return 0;
}

static int bank_size1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);

    const GL_Sheet_t *sheet = instance->sheet;
    lua_pushinteger(L, sheet->size.width);
    lua_pushinteger(L, sheet->size.height);

    return 2;
}

static int bank_size2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);
    float scale = lua_tonumber(L, 2);

    const GL_Sheet_t *sheet = instance->sheet;
    lua_pushinteger(L, (int)(sheet->size.width * fabsf(scale)));
    lua_pushinteger(L, (int)(sheet->size.height * fabsf(scale)));

    return 2;
}

static int bank_size3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);
    float scale_x = lua_tonumber(L, 2);
    float scale_y = lua_tonumber(L, 3);

    const GL_Sheet_t *sheet = instance->sheet;
    lua_pushinteger(L, (int)(sheet->size.width * fabsf(scale_x)));
    lua_pushinteger(L, (int)(sheet->size.height * fabsf(scale_y)));

    return 2;
}

static int bank_size(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, bank_size1)
        LUAX_OVERLOAD_ARITY(2, bank_size2)
        LUAX_OVERLOAD_ARITY(3, bank_size3)
    LUAX_OVERLOAD_END
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

    const GL_Context_t *context = instance->context;
    const GL_Sheet_t *sheet = instance->sheet;
    GL_context_blit(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y });

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
    float scale = lua_tonumber(L, 5);

    const GL_Context_t *context = instance->context;
    const GL_Sheet_t *sheet = instance->sheet;
    GL_context_blit_s(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale, scale);

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
    float scale = lua_tonumber(L, 5);
    int rotation = lua_tointeger(L, 6);

    const GL_Context_t *context = instance->context;
    const GL_Sheet_t *sheet = instance->sheet;
    GL_context_blit_sr(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale, scale, rotation, 0.5f, 0.5f);

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

    const GL_Context_t *context = instance->context;
    const GL_Sheet_t *sheet = instance->sheet;
    GL_context_blit_sr(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale_x, scale_y, rotation, 0.5f, 0.5f);

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

    const GL_Context_t *context = instance->context;
    const GL_Sheet_t *sheet = instance->sheet;
    GL_context_blit_sr(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale_x, scale_y, rotation, anchor_x, anchor_y);

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
