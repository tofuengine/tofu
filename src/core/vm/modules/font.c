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

#include "font.h"

#include <config.h>
#include <core/io/display.h>
#include <core/io/storage.h>
#include <libs/gl/gl.h>
#include <libs/log.h>
#include <libs/luax.h>

#include "udt.h"
#include "utils/callbacks.h"

#include <math.h>

#define LOG_CONTEXT "font"
#define META_TABLE  "Tofu_Graphics_Font_mt"
#define SCRIPT_NAME "@font.lua"

static int font_new_v_1o(lua_State *L);
static int font_gc_1o_0(lua_State *L);
static int font_size_4osNN_2n(lua_State *L);
static int font_blit_v_2nn(lua_State *L);

static const char _font_lua[] = {
#include "font.inc"
};

int font_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){
            .buffer = _font_lua,
            .size = sizeof(_font_lua) / sizeof(char),
            .name = SCRIPT_NAME
        },
        (const struct luaL_Reg[]){
            { "new", font_new_v_1o },
            { "__gc", font_gc_1o_0 },
            { "size", font_size_4osNN_2n },
            { "blit", font_blit_v_2nn },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static inline void _generate_alphabeth(GL_Cell_t glyphs[256], const char *alphabeth)
{
    if (alphabeth) {
        for (size_t i = 0; i < 256; ++i) {
            glyphs[i] = GL_CELL_NIL;
        }
        const uint8_t *ptr = (const uint8_t *)alphabeth; // Hack! Treat as unsigned! :)
        for (size_t i = 0; ptr[i] != '\0'; ++i) {
            glyphs[ptr[i]] = (GL_Cell_t)i;
        }
    } else {
        for (size_t i = 0; i < 256; ++i) {
            glyphs[i] = i < ' ' ? GL_CELL_NIL : (GL_Cell_t)(i - ' ');
        }
    }
}

static int font_new_3osS_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *atlas = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const char *cells_file = LUAX_STRING(L, 2);
    const char *alphabeth = LUAX_OPTIONAL_STRING(L, 3, NULL);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));

    const Storage_Resource_t *cells = Storage_load(storage, cells_file, STORAGE_RESOURCE_BLOB);
    if (!cells) {
        return luaL_error(L, "can't load file `%s`", cells_file);
    }

    GL_Sheet_t *sheet = GL_sheet_create(atlas->surface, S_BPTR(cells), S_BSIZE(cells) / sizeof(GL_Rectangle_u32_t)); // Calculate the amount of entries on the fly.
    if (!sheet) {
        return luaL_error(L, "can't create sheet");
    }

    Font_Object_t *self = (Font_Object_t *)luaX_newobject(L, sizeof(Font_Object_t), &(Font_Object_t){
            .atlas = {
                .instance = atlas,
                .reference = luaX_ref(L, 1)
            },
            .sheet = sheet,
            .glyphs = { 0 }
        }, OBJECT_TYPE_FONT, META_TABLE);
    _generate_alphabeth(self->glyphs, alphabeth);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p allocated w/ sheet %p for atlas %p w/ reference #%d",
        self, sheet, atlas, self->atlas.reference);

    return 1;
}

static int font_new_4onnS_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *atlas = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    size_t glyph_width = (size_t)LUAX_INTEGER(L, 2);
    size_t glyph_height = (size_t)LUAX_INTEGER(L, 3);
    const char *alphabeth = LUAX_OPTIONAL_STRING(L, 4, NULL);

    GL_Sheet_t *sheet = GL_sheet_create_fixed(atlas->surface, (GL_Size_t){ .width = glyph_width, .height = glyph_height });
    if (!sheet) {
        return luaL_error(L, "can't create sheet");
    }

    Font_Object_t *self = (Font_Object_t *)luaX_newobject(L, sizeof(Font_Object_t), &(Font_Object_t){
            .atlas = {
                .instance = atlas,
                .reference = luaX_ref(L, 1)
            },
            .sheet = sheet,
            .glyphs = { 0 }
        }, OBJECT_TYPE_FONT, META_TABLE);
    _generate_alphabeth(self->glyphs, alphabeth);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p allocated w/ sheet %p for atlas %p w/ reference #%d",
        self, sheet, atlas, self->atlas.reference);

    return 1;
}

static int font_new_v_1o(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_SIGNATURE(font_new_3osS_1o, LUA_TOBJECT, LUA_TSTRING, LUA_TSTRING)
        LUAX_OVERLOAD_SIGNATURE(font_new_4onnS_1o, LUA_TOBJECT, LUA_TNUMBER, LUA_TNUMBER)
        LUAX_OVERLOAD_ARITY(3, font_new_3osS_1o)
        LUAX_OVERLOAD_ARITY(4, font_new_4onnS_1o)
    LUAX_OVERLOAD_END
}

static int font_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Font_Object_t *self = (Font_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_FONT);

    GL_sheet_destroy(self->sheet);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p destroyed", self->sheet);

    luaX_unref(L, self->atlas.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "atlas reference #%d released", self->atlas.reference);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p finalized", self);

    return 0;
}

static GL_Size_t _size(const GL_Sheet_t *sheet, const char *text, const GL_Cell_t *glyphs, float scale_x, float scale_y)
{
    size_t width = 0, height = 0;
    for (const uint8_t *ptr = (const uint8_t *)text; *ptr != '\0'; ++ptr) { // The input string is always *not* null.
        uint8_t c = *ptr;
        const GL_Cell_t cell_id = glyphs[(size_t)c];
        if (cell_id == GL_CELL_NIL) {
            continue;
        }
        GL_Size_t size = GL_sheet_size(sheet, cell_id, scale_x, scale_y);
        width += size.width;
        if (height < size.height) {
            height = size.height;
        }
    }

    return (GL_Size_t){ .width = width, .height = height };
}

static int font_size_4osNN_2n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING, LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Font_Object_t *self = (const Font_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_FONT);
    const char *text = LUAX_STRING(L, 2);
    float scale_x = LUAX_OPTIONAL_NUMBER(L, 3, 1.0f);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 4, scale_x);

    const GL_Sheet_t *sheet = self->sheet;
    const GL_Cell_t *glyphs = self->glyphs;

    const GL_Size_t size = _size(sheet, text, glyphs, scale_x, scale_y);

    lua_pushinteger(L, (lua_Integer)size.width);
    lua_pushinteger(L, (lua_Integer)size.height);

    return 2;
}

static int font_blit_5oonns_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING, LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Font_Object_t *self = (const Font_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_FONT);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    const char *text = LUAX_STRING(L, 5);

    const GL_Surface_t *surface = canvas->surface;
    const GL_Sheet_t *sheet = self->sheet;
    const GL_Cell_t *glyphs = self->glyphs;

    size_t width = 0, height = 0;
    for (const uint8_t *ptr = (const uint8_t *)text; *ptr != '\0'; ++ptr) { // Hack! Treat as unsigned! :)
        uint8_t c = *ptr;
        const GL_Cell_t cell_id = glyphs[(size_t)c];
        if (cell_id == GL_CELL_NIL) {
            continue;
        }
        const GL_Size_t cell_size = GL_sheet_size(sheet, cell_id, 1.0f, 1.0f);
        GL_sheet_blit(sheet, surface, (GL_Point_t){ .x = x + width, .y = y }, cell_id);
        width += cell_size.width;
        if (height < cell_size.height) {
            height = cell_size.height;
        }
    }

    lua_pushinteger(L, (lua_Integer)width);
    lua_pushinteger(L, (lua_Integer)height);

    return 2;
}

static int font_blit_7oonnsnN_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING, LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Font_Object_t *self = (const Font_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_FONT);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    const char *text = LUAX_STRING(L, 5);
    float scale_x = LUAX_NUMBER(L, 6);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 7, scale_x);

    const GL_Surface_t *surface = canvas->surface;
    const GL_Sheet_t *sheet = self->sheet;
    const GL_Cell_t *glyphs = self->glyphs;

    size_t width = 0, height = 0;
    for (const uint8_t *ptr = (const uint8_t *)text; *ptr != '\0'; ++ptr) { // Hack! Treat as unsigned! :)
        uint8_t c = *ptr;
        // TODO: add support for tabs and escape-commands.
        const GL_Cell_t cell_id = glyphs[(size_t)c];
        if (cell_id == GL_CELL_NIL) {
            continue;
        }
        const GL_Size_t cell_size = GL_sheet_size(sheet, cell_id, scale_x, scale_y);
        GL_sheet_blit_s(sheet, surface, (GL_Point_t){ .x = x + width, .y = y }, cell_id, scale_x, scale_y);
        width += cell_size.width;
        if (height < cell_size.height) {
            height = cell_size.height;
        }
    }

    lua_pushinteger(L, (lua_Integer)width);
    lua_pushinteger(L, (lua_Integer)height);

    return 2;
}

static int font_blit_v_2nn(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(5, font_blit_5oonns_2nn)
        LUAX_OVERLOAD_ARITY(6, font_blit_7oonnsnN_2nn)
        LUAX_OVERLOAD_ARITY(7, font_blit_7oonnsnN_2nn)
    LUAX_OVERLOAD_END
}
