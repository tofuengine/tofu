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

#include "bank.h"

#include <config.h>
#include <core/io/display.h>
#include <core/io/storage.h>
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
static int bank_canvas(lua_State *L);
static int bank_blit(lua_State *L);

static const struct luaL_Reg _bank_functions[] = {
    { "new", bank_new },
    { "__gc", bank_gc },
    { "size", bank_size },
    { "canvas", bank_canvas },
    { "blit", bank_blit },
    { NULL, NULL }
};

static const luaX_Const _bank_constants[] = {
    { "NIL", LUA_CT_INTEGER, { .i = GL_CELL_NIL } },
    { NULL, LUA_CT_NIL, { 0 } }
};

int bank_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _bank_functions, _bank_constants, nup, META_TABLE);
}

static int bank_new3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_USERDATA(L, 1);
    const Canvas_Object_t *atlas = (const Canvas_Object_t *)LUAX_USERDATA(L, 2);
    const char *cells_file = LUAX_STRING(L, 3);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));

    const Storage_Resource_t *cells = Storage_load(storage, cells_file, STORAGE_RESOURCE_BLOB);
    if (!cells) {
        return luaL_error(L, "can't load file `%s`", cells_file);
    }

    GL_Sheet_t *sheet = GL_sheet_create(atlas->context->surface, S_BPTR(cells), S_BSIZE(cells) / sizeof(GL_Rectangle_u32_t)); // Calculate the amount of entries on the fly.
    if (!sheet) {
        return luaL_error(L, "can't create sheet");
    }

    Bank_Object_t *self = (Bank_Object_t *)lua_newuserdatauv(L, sizeof(Bank_Object_t), 1);
    *self = (Bank_Object_t){
            .canvas = {
                .instance = canvas,
                .reference = luaX_ref(L, 1)
            },
            .atlas = {
                .instance = atlas,
                .reference = luaX_ref(L, 2),
            },
            .sheet = sheet
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p allocated w/ sheet %p for canvas %p w/ reference #%d and atlas %p w/ reference #%d",
        self, sheet, canvas, self->canvas.reference, atlas, self->atlas.reference);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int bank_new4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_USERDATA(L, 1);
    const Canvas_Object_t *atlas = (const Canvas_Object_t *)LUAX_USERDATA(L, 2);
    size_t cell_width = (size_t)LUAX_INTEGER(L, 3);
    size_t cell_height = (size_t)LUAX_INTEGER(L, 4);

    GL_Sheet_t *sheet = GL_sheet_create_fixed(atlas->context->surface, (GL_Size_t ){ .width = cell_width, .height = cell_height });
    if (!sheet) {
        return luaL_error(L, "can't create sheet");
    }

    Bank_Object_t *self = (Bank_Object_t *)lua_newuserdatauv(L, sizeof(Bank_Object_t), 1);
    *self = (Bank_Object_t){
            .canvas = {
                .instance = canvas,
                .reference = luaX_ref(L, 1)
            },
            .atlas = {
                .instance = atlas,
                .reference = luaX_ref(L, 2),
            },
            .sheet = sheet
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p allocated w/ sheet %p for canvas %p w/ reference #%d and atlas %p w/ reference #%d",
        self, sheet, canvas, self->canvas.reference, atlas, self->atlas.reference);

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
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Bank_Object_t *self = (Bank_Object_t *)LUAX_USERDATA(L, 1);

    GL_sheet_destroy(self->sheet);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p destroyed", self->sheet);

    luaX_unref(L, self->atlas.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "atlas reference #%d released", self->atlas.reference);

    luaX_unref(L, self->canvas.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas reference #%d released", self->canvas.reference);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p finalized", self);

    return 0;
}

static int bank_size(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_USERDATA(L, 1);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 2);
    float scale_x = LUAX_OPTIONAL_NUMBER(L, 3, 1.0f);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 4, scale_x);

    const GL_Sheet_t *sheet = self->sheet;
    const GL_Rectangle_t *cell = cell_id == GL_CELL_NIL ? sheet->cells : &sheet->cells[cell_id]; // If `-1` pick the first one.
    lua_pushinteger(L, (lua_Integer)((float)cell->width * fabsf(scale_x)));
    lua_pushinteger(L, (lua_Integer)((float)cell->height * fabsf(scale_y)));

    return 2;
}

static int bank_canvas(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Bank_Object_t *self = (Bank_Object_t *)LUAX_USERDATA(L, 1);
    const Canvas_Object_t *canvas = (Canvas_Object_t *)LUAX_USERDATA(L, 2);

    luaX_unref(L, self->canvas.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas reference #%d released", self->canvas.reference);

    self->canvas.instance = canvas;
    self->canvas.reference = luaX_ref(L, 2);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p attached w/ reference #%d", canvas, self->canvas.reference);

    return 0;
}

// Bank.blit(self, cell_id, x, y)
// Bank.blit(self, cell_id, x, y, r)
// Bank.blit(self, cell_id, x, y, sx, sy)
// Bank.blit(self, cell_id, x, y, sx, sy, r)
// Bank.blit(self, cell_id, x, y, sx, sy, r, ax = 0.5, ay = 0.5)
static int bank_blit4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_USERDATA(L, 1);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);

    const GL_Context_t *context = self->canvas.instance->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_context_blit(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y });

    return 0;
}

static int bank_blit5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_USERDATA(L, 1);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    int rotation = LUAX_INTEGER(L, 5);

    const GL_Context_t *context = self->canvas.instance->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_context_blit_sr(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, 1.0f, 1.0f, rotation, 0.5f, 0.5f);

    return 0;
}

static int bank_blit6(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_USERDATA(L, 1);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    float scale_x = LUAX_NUMBER(L, 5);
    float scale_y = LUAX_NUMBER(L, 6);

    const GL_Context_t *context = self->canvas.instance->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_context_blit_s(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale_x, scale_y);

    return 0;
}

static int bank_blit7_8_9(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_USERDATA(L, 1);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    float scale_x = LUAX_NUMBER(L, 5);
    float scale_y = LUAX_NUMBER(L, 6);
    int rotation = LUAX_INTEGER(L, 7);
    float anchor_x = LUAX_OPTIONAL_NUMBER(L, 8, 0.5f);
    float anchor_y = LUAX_OPTIONAL_NUMBER(L, 9, anchor_x);

    const GL_Context_t *context = self->canvas.instance->context;
    const GL_Sheet_t *sheet = self->sheet;
    // TODO: implement `GL_sheet_blit_XXX()` functions, passing a cell-id as argumento.
    GL_context_blit_sr(context, sheet->atlas, sheet->cells[cell_id], (GL_Point_t){ .x = x, .y = y }, scale_x, scale_y, rotation, anchor_x, anchor_y);

    return 0;
}

static int bank_blit(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(4, bank_blit4)
        LUAX_OVERLOAD_ARITY(5, bank_blit5)
        LUAX_OVERLOAD_ARITY(6, bank_blit6)
        LUAX_OVERLOAD_ARITY(7, bank_blit7_8_9)
        LUAX_OVERLOAD_ARITY(8, bank_blit7_8_9)
        LUAX_OVERLOAD_ARITY(9, bank_blit7_8_9)
    LUAX_OVERLOAD_END
}
