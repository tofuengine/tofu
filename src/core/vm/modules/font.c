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

#include "callbacks.h"
#include "udt.h"

#include <math.h>

#define LOG_CONTEXT "font"
#define META_TABLE  "Tofu_Graphics_Font_mt"

static int font_new_4uunn_1u(lua_State *L);
static int font_gc_1u_0(lua_State *L);
static int font_size_4uSNN_2n(lua_State *L);
static int font_canvas_2uu_0(lua_State *L);
static int font_write(lua_State *L);

static const struct luaL_Reg _font_functions[] = {
    { "new", font_new_4uunn_1u },
    { "__gc", font_gc_1u_0 },
    { "size", font_size_4uSNN_2n },
    { "canvas", font_canvas_2uu_0 },
    { "write", font_write },
    { NULL, NULL }
};

static const unsigned char _font_lua[] = {
#include "font.inc"
};

// TODO: implement font in Lua as a bank.

static luaX_Script _font_script = { (const char *)_font_lua, sizeof(_font_lua), "@font.lua" }; // Trace as filename internally.

int font_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_font_script, _font_functions, NULL, nup, META_TABLE);
}

static int font_new_4uunn_1u(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_USERDATA(L, 1);
    const Canvas_Object_t *atlas = (const Canvas_Object_t *)LUAX_USERDATA(L, 2);
    size_t glyph_width = (size_t)LUAX_INTEGER(L, 3);
    size_t glyph_height = (size_t)LUAX_INTEGER(L, 4);

    GL_Sheet_t *sheet = GL_sheet_create_fixed(atlas->context->surface, (GL_Size_t ){ .width = glyph_width, .height = glyph_height });
    if (!sheet) {
        return luaL_error(L, "can't create sheet");
    }

    Font_Object_t *self = (Font_Object_t *)lua_newuserdatauv(L, sizeof(Font_Object_t), 1);
    *self = (Font_Object_t){
            .canvas = {
                .instance = canvas,
                .reference = luaX_ref(L, 1)
            },
            .atlas = {
                .instance = atlas,
                .reference = luaX_ref(L, 2)
            },
            .sheet = sheet,
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p allocated w/ sheet %p for canvas %p w/ reference #%d and atlas %p w/ reference #%d",
        self, sheet, canvas, self->canvas.reference, atlas, self->atlas.reference);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int font_gc_1u_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Font_Object_t *self = (Font_Object_t *)LUAX_USERDATA(L, 1);

    GL_sheet_destroy(self->sheet);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p destroyed", self->sheet);

    luaX_unref(L, self->atlas.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "atlas reference #%d released", self->atlas.reference);

    luaX_unref(L, self->canvas.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas reference #%d released", self->canvas.reference);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p finalized", self);

    return 0;
}

static void _size(const char *text, const GL_Rectangle_t *cells, float scale_x, float scale_y, size_t *w, size_t *h)
{
    if (!text || text[0] == '\0') {
        const GL_Rectangle_t *cell = &cells[0]; // Font is non-proportional, use the first glyph.
        *w = cell->width;
        *h = cell->height;
        return;
    }

    *w = *h = 0;

    size_t max_width = 0, width = 0;
    size_t height = 0;
    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
        char c = *ptr;
#ifndef __NO_LINEFEEDS__
        if (c == '\n') {
            *h += height;
            if (max_width < width) {
                max_width = width;
            }
            width = 0;
            height = 0;
            continue;
        } else
#endif
        if (c < ' ') {
            continue;
        }

        const GL_Rectangle_t *cell = &cells[c - ' '];

        const size_t cw = (size_t)((float)cell->width * fabs(scale_x));
        const size_t ch = (size_t)((float)cell->height * fabs(scale_y));

        width += cw;
        if (height < ch) {
            height = ch;
        }
    }
    if (max_width < width) {
        max_width = width;
    }
    *w = max_width;
    *h += height;
}

static int font_size_4uSNN_2n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Font_Object_t *self = (const Font_Object_t *)LUAX_USERDATA(L, 1);
    const char *text = LUAX_OPTIONAL_STRING(L, 2, NULL);
    float scale_x = LUAX_OPTIONAL_NUMBER(L, 3, 1.0f);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 4, scale_x);

    size_t width, height;
    _size(text, self->sheet->cells, scale_x, scale_y, &width, &height);

    lua_pushinteger(L, (lua_Integer)width);
    lua_pushinteger(L, (lua_Integer)height);

    return 2;
}

static int font_canvas_2uu_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Font_Object_t *self = (Font_Object_t *)LUAX_USERDATA(L, 1);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_USERDATA(L, 2);

    luaX_unref(L, self->canvas.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas reference #%d released", self->canvas.reference);

    self->canvas.instance = canvas;
    self->canvas.reference = luaX_ref(L, 2);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p attached w/ reference #%d", canvas, self->canvas.reference);

    return 0;
}

static int font_write_4usnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Font_Object_t *self = (const Font_Object_t *)LUAX_USERDATA(L, 1);
    const char *text = LUAX_STRING(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);

    const GL_Context_t *context = self->canvas.instance->context;
    const GL_Sheet_t *sheet = self->sheet;
    const GL_Rectangle_t *cells = sheet->cells;

    int dx = x, dy = y;
    size_t height = 0;
    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
        char c = *ptr;
#ifndef __NO_LINEFEEDS__
        if (c == '\n') { // Handle carriage-return
            dx = x;
            dy += height;
            height = 0;
            continue;
        } else
#endif
        if (c < ' ') {
            continue;
        }
        const GL_Rectangle_t *cell = &cells[c - ' '];
        GL_context_blit(context, sheet->atlas, *cell, (GL_Point_t){ .x = dx, .y = dy });
        dx += cell->width;
        if (height < cell->height) {
            height = cell->height;
        }
    }

    return 0;
}

static int font_write_6usnnnN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Font_Object_t *self = (const Font_Object_t *)LUAX_USERDATA(L, 1);
    const char *text = LUAX_STRING(L, 2);
    int x = LUAX_INTEGER(L, 3); // TODO: make all arguments const?
    int y = LUAX_INTEGER(L, 4);
    float scale_x = LUAX_NUMBER(L, 5);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 6, scale_x);

    const GL_Context_t *context = self->canvas.instance->context;
    const GL_Sheet_t *sheet = self->sheet;
    const GL_Rectangle_t *cells = sheet->cells;

    int dx = x, dy = y;
    size_t height = 0;
    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
        char c = *ptr;
#ifndef __NO_LINEFEEDS__
        if (c == '\n') { // Handle carriage-return
            dx = x;
            dy += height;
            height = 0;
            continue;
        } else
#endif
        if (c < ' ') {
            continue;
        }
        const GL_Rectangle_t *cell = &cells[c - ' '];
        const size_t cw = (size_t)(cell->width * fabs(scale_x));
        const size_t ch = (size_t)(cell->height * fabs(scale_y));
        GL_context_blit_s(context, sheet->atlas, *cell, (GL_Point_t){ .x = dx, .y = dy }, scale_x, scale_y);
        dx += cw;
        if (height < ch) {
            height = ch;
        }
    }

    return 0;
}

static int font_write(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(4, font_write_4usnn_0)
        LUAX_OVERLOAD_ARITY(5, font_write_6usnnnN_0)
        LUAX_OVERLOAD_ARITY(6, font_write_6usnnnN_0)
    LUAX_OVERLOAD_END
}
