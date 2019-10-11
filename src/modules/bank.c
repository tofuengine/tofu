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

#include "../core/luax.h"

#include "../config.h"
#include "../environment.h"
#include "../log.h"
#include "../gl/gl.h"

#include <math.h>
#include <string.h>

typedef struct _Bank_Class_t {
    // char pathfile[PATH_FILE_MAX];
    GL_Sheet_t sheet;
} Bank_Class_t;

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

#include "bank.inc"

int bank_loader(lua_State *L)
{
    luaX_Script script = { (const char *)_bank_lua, _bank_lua_len, "bank.lua" };
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, &script, _bank_functions, _bank_constants, nup, LUAX_CLASS(Bank_Class_t));
}

static void to_indexed_atlas_callback(void *parameters, GL_Surface_t *surface, const void *data)
{
    const GL_Palette_t *palette = (const GL_Palette_t *)parameters;

    const GL_Color_t *src = (const GL_Color_t *)data;
    GL_Pixel_t *dst = (GL_Pixel_t *)surface->data;

    for (int i = surface->data_size; i; --i) {
        GL_Color_t color = *(src++);
        *(dst++) = GL_palette_find_nearest_color(palette, color);
    }
}

static int bank_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    const char *file = lua_tostring(L, 1);
    int cell_width = lua_tointeger(L, 2);
    int cell_height = lua_tointeger(L, 3);

#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.new() -> %s, %d, %d", file, cell_width, cell_height);
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));
    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file);

    GL_Sheet_t sheet;
    GL_sheet_load(&sheet, pathfile, cell_width, cell_height, to_indexed_atlas_callback, (void *)&display->palette);
    Log_write(LOG_LEVELS_DEBUG, "<BANK> sheet '%s' loaded", pathfile);

    Bank_Class_t *instance = (Bank_Class_t *)lua_newuserdata(L, sizeof(Bank_Class_t));
    *instance = (Bank_Class_t){
            .sheet = sheet
        };
    Log_write(LOG_LEVELS_DEBUG, "<BANK> bank allocated as #%p", pathfile, instance);

    luaL_setmetatable(L, LUAX_CLASS(Bank_Class_t));

    return 1;
}

static int bank_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Bank_Class_t *instance = (Bank_Class_t *)lua_touserdata(L, 1);

    GL_sheet_delete(&instance->sheet);
    Log_write(LOG_LEVELS_DEBUG, "<BANK> bank #%p finalized", instance);

    *instance = (Bank_Class_t){};

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
    double x = (double)lua_tonumber(L, 3);
    double y = (double)lua_tonumber(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f", cell_id, x, y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = (int)x, .y = (int)y });

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
    double x = (double)lua_tonumber(L, 3);
    double y = (double)lua_tonumber(L, 4);
    double rotation = (double)lua_tonumber(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f, %.f", cell_id, x, y, rotation);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit_sr(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = (int)x, .y = (int)y }, 1.0f, 1.0f, rotation, 0.5f, 0.5f);

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
    double x = (double)lua_tonumber(L, 3);
    double y = (double)lua_tonumber(L, 4);
    double scale_x = (double)lua_tonumber(L, 5);
    double scale_y = (double)lua_tonumber(L, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f, %.f, %.f", cell_id, x, y, scale_x, scale_y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit_s(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = (int)x, .y = (int)y }, scale_x, scale_y);

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
    double x = (double)lua_tonumber(L, 3);
    double y = (double)lua_tonumber(L, 4);
    double scale_x = (double)lua_tonumber(L, 5);
    double scale_y = (double)lua_tonumber(L, 6);
    double rotation = (double)lua_tonumber(L, 7);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f, %.f, %.f, %.f", cell_id, x, y, scale_x, scale_y, rotation);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit_sr(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = (int)x, .y = (int)y }, scale_x, scale_y, rotation, 0.5f, 0.5f);

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
    double x = (double)lua_tonumber(L, 3);
    double y = (double)lua_tonumber(L, 4);
    double scale_x = (double)lua_tonumber(L, 5);
    double scale_y = (double)lua_tonumber(L, 6);
    double rotation = (double)lua_tonumber(L, 7);
    double anchor_x = (double)lua_tonumber(L, 8);
    double anchor_y = (double)lua_tonumber(L, 9);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f, %.f, %.f, %.f, %.f, %.f", cell_id, x, y, scale_x, scale_y, rotation, anchor_x, anchor_y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_context_blit_sr(context, &sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = (int)x, .y = (int)y }, scale_x, scale_y, rotation, anchor_x, anchor_y);

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
