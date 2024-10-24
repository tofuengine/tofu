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

#include "bank.h"

#include "internal/callbacks.h"
#include "internal/udt.h"

#include <core/config.h>
#define _LOG_TAG "bank"
#include <libs/log.h>
#include <systems/storage.h>

#include <math.h>

static int bank_new_v_1o(lua_State *L);
static int bank_gc_1o_0(lua_State *L);
static int bank_size_4onNN_2n(lua_State *L);

int bank_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", bank_new_v_1o },
            { "__gc", bank_gc_1o_0 },
            // -- accessors --
            { "size", bank_size_4onNN_2n },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { "NIL", LUA_CT_INTEGER, { .i = GL_CELL_NIL } },
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static int bank_new_2os_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Image_Object_t *atlas = (const Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);
    const char *cells_file = LUAX_STRING(L, 2);

    Storage_t *storage = (Storage_t *)udt_get_userdata(L, USERDATA_STORAGE);

    const Storage_Resource_t *cells = Storage_load(storage, cells_file, STORAGE_RESOURCE_BLOB);
    if (!cells) {
        return luaL_error(L, "can't load file `%s`", cells_file);
    }

    GL_Sheet_t *sheet = GL_sheet_create(atlas->surface, SR_BPTR(cells), SR_BSIZE(cells) / sizeof(GL_Rectangle32_t)); // Calculate the amount of entries on the fly.
    if (!sheet) {
        return luaL_error(L, "can't create sheet");
    }

    Bank_Object_t *self = (Bank_Object_t *)udt_newobject(L, sizeof(Bank_Object_t), &(Bank_Object_t){
            .atlas = {
                .instance = atlas,
                .reference = luaX_ref(L, 1),
            },
            .sheet = sheet
        }, OBJECT_TYPE_BANK);

    LOG_D("bank %p with cells file `%s` allocated w/ sheet %p for atlas %p w/ reference #%d",
        self, cells_file, sheet, atlas, self->atlas.reference);

    return 1;
}

static int bank_new_3oNN_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Image_Object_t *atlas = (const Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);
    size_t cell_width = LUAX_OPTIONAL_UNSIGNED(L, 2, atlas->surface->width);
    size_t cell_height = LUAX_OPTIONAL_UNSIGNED(L, 3, atlas->surface->height);

    GL_Sheet_t *sheet = GL_sheet_create_fixed(atlas->surface, (GL_Size_t){ .width = cell_width, .height = cell_height });
    if (!sheet) {
        return luaL_error(L, "can't create sheet");
    }

    Bank_Object_t *self = (Bank_Object_t *)udt_newobject(L, sizeof(Bank_Object_t), &(Bank_Object_t){
            .atlas = {
                .instance = atlas,
                .reference = luaX_ref(L, 1),
            },
            .sheet = sheet
        }, OBJECT_TYPE_BANK);

    LOG_D("bank %p with %dx%d cells allocated w/ sheet %p for atlas %p w/ reference #%d",
        self, cell_width, cell_height, sheet, atlas, self->atlas.reference);

    return 1;
}

static int bank_new_v_1o(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(bank_new_3oNN_1o, 1)
        LUAX_OVERLOAD_BY_ARITY(bank_new_2os_1o, 2)
        LUAX_OVERLOAD_BY_ARITY(bank_new_3oNN_1o, 3)
    LUAX_OVERLOAD_END
}

static int bank_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Bank_Object_t *self = (Bank_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BANK);

    GL_sheet_destroy(self->sheet);
    LOG_D("sheet %p destroyed", self->sheet);

    luaX_unref(L, self->atlas.reference);
    LOG_D("atlas reference #%d released", self->atlas.reference);

    LOG_D("bank %p finalized", self);

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
