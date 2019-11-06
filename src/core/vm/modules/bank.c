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
#include <core/io/fs.h>
#include <libs/log.h>
#include <libs/gl/gl.h>

#include "udt.h"

#include <math.h>
#include <string.h>
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif

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
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, NULL, _bank_functions, _bank_constants, nup, BANK_MT);
}

static void to_indexed_atlas_callback(void *parameters, GL_Surface_t *surface, const void *data)
{
    const GL_Palette_t *palette = (const GL_Palette_t *)parameters;

    const GL_Color_t *src = (const GL_Color_t *)data;
    GL_Pixel_t *dst = (GL_Pixel_t *)surface->data;

    for (size_t i = surface->data_size; i; --i) {
        GL_Color_t color = *(src++);
        *(dst++) = GL_palette_find_nearest_color(palette, color);
    }
}

static int bank_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring, luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);
    size_t cell_width = (size_t)lua_tointeger(L, 2);
    size_t cell_height = (size_t)lua_tointeger(L, 3);

#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.new() -> %d, %d, %d", type, cell_width, cell_height);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(3));
    File_System_t *fs = (File_System_t *)lua_touserdata(L, lua_upvalueindex(5));

    GL_Sheet_t sheet;

    if (type == LUA_TSTRING) {
        const char *file = lua_tostring(L, 1);

        size_t buffer_size;
        void *buffer = FS_load_as_binary(fs, file, &buffer_size);
        if (!buffer) {
            return luaL_error(L, "<BANK> can't load file '%s'", file);
        }
        GL_sheet_decode(&sheet, buffer, buffer_size, cell_width, cell_height, to_indexed_atlas_callback, (void *)&display->palette);
        Log_write(LOG_LEVELS_DEBUG, "<BANK> sheet '%s' loaded", file);
        free(buffer);
    } else
    if (type == LUA_TUSERDATA) {
        const Surface_Class_t *instance = (const Surface_Class_t *)lua_touserdata(L, 1);

        GL_sheet_attach(&sheet, &instance->surface, cell_width, cell_height);
        Log_write(LOG_LEVELS_DEBUG, "<BANK> sheet %p attached", instance);
    }

    Bank_Class_t *instance = (Bank_Class_t *)lua_newuserdata(L, sizeof(Bank_Class_t));
    *instance = (Bank_Class_t){
            .sheet = sheet,
            .owned = type == LUA_TSTRING ? true : false
        };
    Log_write(LOG_LEVELS_DEBUG, "<BANK> bank allocated as #%p", instance);

    luaL_setmetatable(L, BANK_MT);

    return 1;
}

static int bank_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);

    if (instance->owned) {
        GL_sheet_delete(&instance->sheet);
    } else {
        GL_sheet_detach(&instance->sheet);
    }
    Log_write(LOG_LEVELS_DEBUG, "<BANK> bank #%p finalized", instance);

    *instance = (Bank_Class_t){ 0 };

    return 0;
}

static int bank_cell_width(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->sheet.size.width);

    return 1;
}

static int bank_cell_height(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->sheet.size.height);

    return 1;
}

static int bank_blit4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);
    int cell_id = lua_tointeger(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f", cell_id, x, y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(3));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y });

    return 0;
}

static int bank_blit5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);
    int cell_id = lua_tointeger(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    int rotation = lua_tointeger(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f, %d", cell_id, x, y, rotation);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(3));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit_sr(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, 1.0f, 1.0f, rotation, 0.5f, 0.5f);

    return 0;
}

static int bank_blit6(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 6)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);
    int cell_id = lua_tointeger(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    float scale_x = lua_tonumber(L, 5);
    float scale_y = lua_tonumber(L, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f, %.f, %.f", cell_id, x, y, scale_x, scale_y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(3));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit_s(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale_x, scale_y);

    return 0;
}

static int bank_blit7(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 7)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);
    int cell_id = lua_tointeger(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    float scale_x = lua_tonumber(L, 5);
    float scale_y = lua_tonumber(L, 6);
    int rotation = lua_tointeger(L, 7);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f, %.f, %.f, %d", cell_id, x, y, scale_x, scale_y, rotation);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(3));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit_sr(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale_x, scale_y, rotation, 0.5f, 0.5f);

    return 0;
}

static int bank_blit9(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 9)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
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
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f, %.f, %.f, %d, %.f, %.f", cell_id, x, y, scale_x, scale_y, rotation, anchor_x, anchor_y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(3));

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
