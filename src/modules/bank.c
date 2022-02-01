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
#include <libs/log.h>
#include <systems/storage.h>

#include "udt.h"
#include "utils/callbacks.h"

#include <math.h>

#define LOG_CONTEXT "bank"
#define META_TABLE  "Tofu_Graphics_Bank_mt"

static int bank_new_v_1o(lua_State *L);
static int bank_gc_1o_0(lua_State *L);
static int bank_size_4onNN_2n(lua_State *L);
static int bank_blit_v_0(lua_State *L);
static int bank_tile_v_0(lua_State *L);

int bank_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", bank_new_v_1o },
            { "__gc", bank_gc_1o_0 },
            { "size", bank_size_4onNN_2n },
            { "blit", bank_blit_v_0 },
            { "tile", bank_tile_v_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { "NIL", LUA_CT_INTEGER, { .i = GL_CELL_NIL } },
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int bank_new_2os_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Image_Object_t *atlas = (const Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);
    const char *cells_file = LUAX_STRING(L, 2);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));

    const Storage_Resource_t *cells = Storage_load(storage, cells_file, STORAGE_RESOURCE_BLOB);
    if (!cells) {
        return luaL_error(L, "can't load file `%s`", cells_file);
    }

    GL_Sheet_t *sheet = GL_sheet_create(atlas->surface, S_BPTR(cells), S_BSIZE(cells) / sizeof(GL_Rectangle_u32_t)); // Calculate the amount of entries on the fly.
    if (!sheet) {
        return luaL_error(L, "can't create sheet");
    }

    Bank_Object_t *self = (Bank_Object_t *)luaX_newobject(L, sizeof(Bank_Object_t), &(Bank_Object_t){
            .atlas = {
                .instance = atlas,
                .reference = luaX_ref(L, 1),
            },
            .sheet = sheet
        }, OBJECT_TYPE_BANK, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p allocated w/ sheet %p for atlas %p w/ reference #%d",
        self, sheet, atlas, self->atlas.reference);

    return 1;
}

static int bank_new_3onn_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Image_Object_t *atlas = (const Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);
    size_t cell_width = LUAX_UNSIGNED(L, 2);
    size_t cell_height = LUAX_UNSIGNED(L, 3);

    GL_Sheet_t *sheet = GL_sheet_create_fixed(atlas->surface, (GL_Size_t){ .width = cell_width, .height = cell_height });
    if (!sheet) {
        return luaL_error(L, "can't create sheet");
    }

    Bank_Object_t *self = (Bank_Object_t *)luaX_newobject(L, sizeof(Bank_Object_t), &(Bank_Object_t){
            .atlas = {
                .instance = atlas,
                .reference = luaX_ref(L, 1),
            },
            .sheet = sheet
        }, OBJECT_TYPE_BANK, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p allocated w/ sheet %p for atlas %p w/ reference #%d",
        self, sheet, atlas, self->atlas.reference);

    return 1;
}

static int bank_new_v_1o(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, bank_new_2os_1o)
        LUAX_OVERLOAD_ARITY(3, bank_new_3onn_1o)
    LUAX_OVERLOAD_END
}

static int bank_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Bank_Object_t *self = (Bank_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BANK);

    GL_sheet_destroy(self->sheet);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p destroyed", self->sheet);

    luaX_unref(L, self->atlas.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "atlas reference #%d released", self->atlas.reference);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank %p finalized", self);

    return 0;
}

static int bank_size_4onNN_2n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BANK);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 2);
    float scale_x = LUAX_OPTIONAL_NUMBER(L, 3, 1.0f);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 4, scale_x);

    const GL_Sheet_t *sheet = self->sheet;
    const GL_Rectangle_t *cell = cell_id == GL_CELL_NIL ? sheet->cells : &sheet->cells[cell_id]; // If `GL_CELL_NIL` pick the first one.
    lua_pushinteger(L, (lua_Integer)((float)cell->width * fabsf(scale_x)));
    lua_pushinteger(L, (lua_Integer)((float)cell->height * fabsf(scale_y)));

    return 2;
}

static int bank_blit_5oonnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BANK);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);

    const GL_Context_t *context = canvas->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_sheet_blit(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id);

    return 0;
}

static int bank_blit_6oonnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BANK);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);
    int rotation = LUAX_INTEGER(L, 6);

    const GL_Context_t *context = canvas->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_sheet_blit_sr(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id, 1.0f, 1.0f, rotation, 0.5f, 0.5f);

    return 0;
}

static int bank_blit_7oonnnnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BANK);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);
    float scale_x = LUAX_NUMBER(L, 6);
    float scale_y = LUAX_NUMBER(L, 7);

    const GL_Context_t *context = canvas->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_sheet_blit_s(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id, scale_x, scale_y);

    return 0;
}

static int bank_blit_10oonnnnnnNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BANK);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);
    float scale_x = LUAX_NUMBER(L, 6);
    float scale_y = LUAX_NUMBER(L, 7);
    int rotation = LUAX_INTEGER(L, 8);
    float anchor_x = LUAX_OPTIONAL_NUMBER(L, 9, 0.5f);
    float anchor_y = LUAX_OPTIONAL_NUMBER(L, 10, anchor_x);

    const GL_Context_t *context = canvas->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_sheet_blit_sr(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id, scale_x, scale_y, rotation, anchor_x, anchor_y);

    return 0;
}

static int bank_blit_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(5, bank_blit_5oonnn_0)
        LUAX_OVERLOAD_ARITY(6, bank_blit_6oonnnn_0)
        LUAX_OVERLOAD_ARITY(7, bank_blit_7oonnnnnn_0)
        LUAX_OVERLOAD_ARITY(8, bank_blit_10oonnnnnnNN_0)
        LUAX_OVERLOAD_ARITY(9, bank_blit_10oonnnnnnNN_0)
        LUAX_OVERLOAD_ARITY(10, bank_blit_10oonnnnnnNN_0)
    LUAX_OVERLOAD_END
}

// FIXME: fix the consts!!!
static int bank_tile_7oonnnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BANK);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);
    int offset_x = LUAX_INTEGER(L, 6);
    int offset_y = LUAX_INTEGER(L, 7);

    const GL_Context_t *context = canvas->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_sheet_tile(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id, (GL_Point_t){ .x = offset_x, .y = offset_y });

    return 0;
}

static int bank_tile_9oonnnnnnN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *self = (const Bank_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BANK);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);
    int offset_x = LUAX_INTEGER(L, 6);
    int offset_y = LUAX_INTEGER(L, 7);
    int scale_x = LUAX_INTEGER(L, 8);
    int scale_y = LUAX_OPTIONAL_INTEGER(L, 9, scale_x);

    const GL_Context_t *context = canvas->context;
    const GL_Sheet_t *sheet = self->sheet;
    GL_sheet_tile_s(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id, (GL_Point_t){ .x = offset_x, .y = offset_y }, scale_x, scale_y);

    return 0;
}

static int bank_tile_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(7, bank_tile_7oonnnnn_0)
        LUAX_OVERLOAD_ARITY(8, bank_tile_9oonnnnnnN_0)
        LUAX_OVERLOAD_ARITY(9, bank_tile_9oonnnnnnN_0)
    LUAX_OVERLOAD_END
}
