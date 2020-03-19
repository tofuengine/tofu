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

#include "font.h"

#include <config.h>
#include <core/io/display.h>
#include <core/vm/interpreter.h>
#include <libs/fs/fsaux.h>
#include <libs/gl/gl.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "callbacks.h"
#include "udt.h"
#include "resources/sheets.h"

#include <math.h>
#include <string.h>

#define LOG_CONTEXT "font"
#define META_TABLE  "Tofu_Graphics_Font_mt"

static int font_new(lua_State *L);
static int font_gc(lua_State *L);
static int font_size(lua_State *L);
static int font_canvas(lua_State *L);
static int font_write(lua_State *L);

static const struct luaL_Reg _font_functions[] = {
    { "new", font_new },
    {"__gc", font_gc },
    { "size", font_size },
    { "canvas", font_canvas },
    { "write", font_write },
    { NULL, NULL }
};

static const unsigned char _font_lua[] = {
#include "font.inc"
};

static luaX_Script _font_script = { (const char *)_font_lua, sizeof(_font_lua), "@font.lua" }; // Trace as filename internally.

int font_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_font_script, _font_functions, NULL, nup, META_TABLE);
}

static int font_new3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING, LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);
    size_t glyph_width = (size_t)LUAX_INTEGER(L, 2);
    size_t glyph_height = (size_t)LUAX_INTEGER(L, 3);

    const File_System_t *file_system = (const File_System_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));
    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Sheet_t *sheet;
    if (type == LUA_TSTRING) {
        const char *file = LUAX_STRING(L, 1);

        const Sheet_Data_t *data = resources_sheets_find(file);
        if (data) {
            sheet = GL_sheet_decode_rect(data->width, data->height, data->pixels, data->cell_width, data->cell_height, surface_callback_palette, (void *)&display->palette);
            if (!sheet) {
                return luaL_error(L, "can't decode sheet `%s`", file);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet `%s` decoded", file);
        } else {
            File_System_Resource_t *resource = FSaux_load(file_system, file, FILE_SYSTEM_RESOURCE_IMAGE);
            if (!resource) {
                return luaL_error(L, "can't load file `%s`", file);
            }
            sheet = GL_sheet_decode_rect(resource->var.image.width, resource->var.image.height, resource->var.image.pixels, data->cell_width, data->cell_height, surface_callback_palette, (void *)&display->palette);
            FSaux_release(resource);
            if (!sheet) {
                return luaL_error(L, "can't decode %dx%d image", resource->var.image.width, resource->var.image.height);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p loaded from file `%s`", sheet, file);
        }
    } else
    if (type == LUA_TUSERDATA) {
        const Canvas_Class_t *canvas = (const Canvas_Class_t *)LUAX_USERDATA(L, 1);

        sheet = GL_sheet_attach_rect(canvas->context->surface, glyph_width, glyph_height);
        if (!sheet) {
            return luaL_error(L, "can't attach sheet");
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached to canvas %p", sheet, canvas);
    } else {
        return luaL_error(L, "invalid argument");
    }

    Font_Class_t *self = (Font_Class_t *)lua_newuserdata(L, sizeof(Font_Class_t));
    *self = (Font_Class_t){
            .context = display->context,
            .context_reference = LUAX_REFERENCE_NIL,
            .sheet = sheet,
            .sheet_reference = type == LUA_TUSERDATA ? luaX_ref(L, 1) : LUAX_REFERENCE_NIL
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p allocated w/ sheet %p for default context", self, sheet);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int font_new5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING, LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);
    size_t glyph_width = (size_t)LUAX_INTEGER(L, 2);
    size_t glyph_height = (size_t)LUAX_INTEGER(L, 3);
    GL_Pixel_t background_index = (GL_Pixel_t)LUAX_INTEGER(L, 4);
    GL_Pixel_t foreground_index = (GL_Pixel_t)LUAX_INTEGER(L, 5);

    const File_System_t *file_system = (const File_System_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));
    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Sheet_t *sheet;
    if (type == LUA_TSTRING) {
        const char *file = LUAX_STRING(L, 1);

        const GL_Pixel_t indexes[] = { background_index, foreground_index };

        const Sheet_Data_t *data = resources_sheets_find(file);
        if (data) {
            sheet = GL_sheet_decode_rect(data->width, data->height, data->pixels, data->cell_width, data->cell_height, surface_callback_indexes, (void *)indexes);
            if (!sheet) {
                return luaL_error(L, "can't decode sheet `%s`", file);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet `%s` decoded", file);
        } else {
            File_System_Resource_t *resource = FSaux_load(file_system, file, FILE_SYSTEM_RESOURCE_IMAGE);
            if (!resource) {
                return luaL_error(L, "can't load file `%s`", file);
            }
            sheet = GL_sheet_decode_rect(resource->var.image.width, resource->var.image.height, resource->var.image.pixels, glyph_width, glyph_height, surface_callback_indexes, (void *)indexes);
            FSaux_release(resource);
            if (!sheet) {
                return luaL_error(L, "can't decode %dx%d image", resource->var.image.width, resource->var.image.height);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p loaded from file `%s`", sheet, file);
        }
    } else
    if (type == LUA_TUSERDATA) {
        const Canvas_Class_t *canvas = (const Canvas_Class_t *)LUAX_USERDATA(L, 1);

        sheet = GL_sheet_attach_rect(canvas->context->surface, glyph_width, glyph_height);
        if (!sheet) {
            return luaL_error(L, "can't attach sheet");
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached to canvas %p", sheet, canvas);
    } else {
        return luaL_error(L, "invalid argument");
    }

    Font_Class_t *self = (Font_Class_t *)lua_newuserdata(L, sizeof(Font_Class_t));
    *self = (Font_Class_t){
            .context = display->context,
            .context_reference = LUAX_REFERENCE_NIL,
            .sheet = sheet,
            .sheet_reference = type == LUA_TUSERDATA ? luaX_ref(L, 1) : LUAX_REFERENCE_NIL
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p allocated w/ sheet %p for default context", self, sheet);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int font_new(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, font_new3)
        LUAX_OVERLOAD_ARITY(5, font_new5)
    LUAX_OVERLOAD_END
}

static int font_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Font_Class_t *self = (Font_Class_t *)LUAX_USERDATA(L, 1);

    if (self->sheet_reference != LUAX_REFERENCE_NIL) {
        luaX_unref(L, self->sheet_reference);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet reference #%d released", self->sheet_reference);
        GL_sheet_detach(self->sheet);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p detached", self->sheet);
    } else {
        GL_sheet_destroy(self->sheet);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p destroyed", self->sheet);
    }

    if (self->context_reference != LUAX_REFERENCE_NIL) {
        luaX_unref(L, self->context_reference);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context reference #%d released", self->context_reference);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p finalized", self);

    return 0;
}

static void _size(const char *text, const GL_Rectangle_t *cells, float scale_x, float scale_y, int *w, int *h)
{
    *w = *h = 0;

    if (!text || text[0] == '\0') {
        return;
    }

    int max_width =0, width = 0;
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

        const size_t cw = (int)(cell->width * fabs(scale_x));
        const size_t ch = (int)(cell->height * fabs(scale_y));

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

static int font_size(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Font_Class_t *self = (Font_Class_t *)LUAX_USERDATA(L, 1);
    const char *text = LUAX_OPTIONAL_STRING(L, 2, NULL);
    float scale_x = LUAX_OPTIONAL_NUMBER(L, 3, 1.0f);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 4, scale_x);

    int width, height;
    _size(text, self->sheet->cells, scale_x, scale_y, &width, &height);

    lua_pushinteger(L, width);
    lua_pushinteger(L, height);

    return 2;
}

static int font_canvas(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Font_Class_t *self = (Font_Class_t *)LUAX_USERDATA(L, 1);
    const Canvas_Class_t *canvas = (Canvas_Class_t *)LUAX_OPTIONAL_USERDATA(L, 2, NULL);

    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    if (self->context_reference != LUAX_REFERENCE_NIL) {
        luaX_unref(L, self->context_reference);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context reference #%d released", self->context_reference);
    }

    if (canvas) {
        self->context = canvas->context;
        self->context_reference = luaX_ref(L, 2);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context %p attached w/ reference #%d", self->context, self->context_reference);
    } else {
        self->context = display->context;
        self->context_reference = LUAX_REFERENCE_NIL;
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "default context attached");
    }

    return 0;
}

static int font_write4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Font_Class_t *self = (Font_Class_t *)LUAX_USERDATA(L, 1);
    const char *text = LUAX_STRING(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = self->sheet;

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
        const GL_Rectangle_t *cell = &sheet->cells[c - ' '];
        GL_context_blit(context, sheet->atlas, *cell, (GL_Point_t){ .x = dx, .y = dy });
        dx += cell->width;
        if (height < cell->height) {
            height = cell->height;
        }
    }

    return 0;
}

static int font_write5_6(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Font_Class_t *self = (Font_Class_t *)LUAX_USERDATA(L, 1);
    const char *text = LUAX_STRING(L, 2);
    int x = LUAX_INTEGER(L, 3); // TODO: make all arguments const?
    int y = LUAX_INTEGER(L, 4);
    float scale_x = LUAX_NUMBER(L, 5);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 6, scale_x);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = self->sheet;

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
        const GL_Rectangle_t *cell = &sheet->cells[c - ' '];
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
        LUAX_OVERLOAD_ARITY(4, font_write4)
        LUAX_OVERLOAD_ARITY(5, font_write5_6)
        LUAX_OVERLOAD_ARITY(6, font_write5_6)
    LUAX_OVERLOAD_END
}
