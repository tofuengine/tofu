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
static int font_width(lua_State *L);
static int font_height(lua_State *L);
static int font_size(lua_State *L);
static int font_write(lua_State *L);

static const struct luaL_Reg _font_functions[] = {
    { "new", font_new },
    {"__gc", font_gc },
    { "width", font_width },
    { "height", font_height },
    { "size", font_size },
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
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING, LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);
    size_t glyph_width = (size_t)lua_tointeger(L, 2);
    size_t glyph_height = (size_t)lua_tointeger(L, 3);

    const File_System_t *file_system = (const File_System_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));
    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    luaX_Reference reference = LUAX_REFERENCE_NIL;
    GL_Sheet_t *sheet;
    if (type == LUA_TSTRING) {
        const char *file = lua_tostring(L, 1);

        const Sheet_Data_t *data = resources_sheets_find(file);
        if (data) {
            sheet = GL_sheet_decode(data->data, data->size, data->cell_width, data->cell_height, surface_callback_palette, (void *)&display->palette);
            if (!sheet) {
                return luaL_error(L, "can't decode sheet `%s`", file);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet `%s` decoded", file);
        } else {
            File_System_Chunk_t chunk = FSaux_load(file_system, file, FILE_SYSTEM_CHUNK_BLOB);
            if (chunk.type == FILE_SYSTEM_CHUNK_NULL) {
                return luaL_error(L, "can't load file `%s`", file);
            }
            sheet = GL_sheet_decode(chunk.var.blob.ptr, chunk.var.blob.size, data->cell_width, data->cell_height, surface_callback_palette, (void *)&display->palette);
            FSaux_release(chunk);
            if (!sheet) {
                return luaL_error(L, "can't decode %d bytes sheet", chunk.var.blob.size);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p loaded from file `%s`", sheet, file);
        }
    } else
    if (type == LUA_TUSERDATA) {
        const Canvas_Class_t *canvas = (const Canvas_Class_t *)lua_touserdata(L, 1);
        reference = luaX_ref(L, 2);

        sheet = GL_sheet_attach(canvas->context->surface, glyph_width, glyph_height);
        if (!sheet) {
            return luaL_error(L, "can't attach sheet");
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached to canvas %p", sheet, canvas);
    }

    Font_Class_t *instance = (Font_Class_t *)lua_newuserdata(L, sizeof(Font_Class_t));
    *instance = (Font_Class_t){
            .context = display->context,
            .sheet = sheet,
            .reference = reference
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p allocated w/ sheet %p for default context", instance, sheet);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int font_new4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING, LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Class_t *canvas = (const Canvas_Class_t *)lua_touserdata(L, 1);
    int type = lua_type(L, 2);
    size_t glyph_width = (size_t)lua_tointeger(L, 3);
    size_t glyph_height = (size_t)lua_tointeger(L, 4);

    const File_System_t *file_system = (const File_System_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));
    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    luaX_Reference reference = LUAX_REFERENCE_NIL;
    GL_Sheet_t *sheet;
    if (type == LUA_TSTRING) {
        const char *file = lua_tostring(L, 2);

        const Sheet_Data_t *data = resources_sheets_find(file);
        if (data) {
            sheet = GL_sheet_decode(data->data, data->size, data->cell_width, data->cell_height, surface_callback_palette, (void *)&display->palette);
            if (!sheet) {
                return luaL_error(L, "can't decode sheet `%s`", file);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet `%s` decoded", file);
        } else {
            File_System_Chunk_t chunk = FSaux_load(file_system, file, FILE_SYSTEM_CHUNK_BLOB);
            if (chunk.type == FILE_SYSTEM_CHUNK_NULL) {
                return luaL_error(L, "can't load file `%s`", file);
            }
            sheet = GL_sheet_decode(chunk.var.blob.ptr, chunk.var.blob.size, glyph_width, glyph_height, surface_callback_palette, (void *)&display->palette);
            FSaux_release(chunk);
            if (!sheet) {
                return luaL_error(L, "can't decode %d bytes sheet", chunk.var.blob.size);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p loaded from file `%s`", sheet, file);
        }
    } else
    if (type == LUA_TUSERDATA) {
        const Canvas_Class_t *canvas = (const Canvas_Class_t *)lua_touserdata(L, 2);
        reference = luaX_ref(L, 2);

        sheet = GL_sheet_attach(canvas->context->surface, glyph_width, glyph_height);
        if (!sheet) {
            return luaL_error(L, "can't attach sheet");
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached to canvas %p", sheet, canvas);
    }

    Font_Class_t *instance = (Font_Class_t *)lua_newuserdata(L, sizeof(Font_Class_t));
    *instance = (Font_Class_t){
            .context = canvas->context,
            .sheet = sheet,
            .reference = reference
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p allocated w/ sheet %p for context %p", instance, sheet, canvas->context);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int font_new5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING, LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);
    size_t glyph_width = (size_t)lua_tointeger(L, 2);
    size_t glyph_height = (size_t)lua_tointeger(L, 3);
    GL_Pixel_t background_index = (GL_Pixel_t)lua_tointeger(L, 4);
    GL_Pixel_t foreground_index = (GL_Pixel_t)lua_tointeger(L, 5);

    const File_System_t *file_system = (const File_System_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));
    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    luaX_Reference reference = LUAX_REFERENCE_NIL;
    GL_Sheet_t *sheet;
    if (type == LUA_TSTRING) {
        const char *file = lua_tostring(L, 1);

        const GL_Pixel_t indexes[] = { background_index, foreground_index };

        const Sheet_Data_t *data = resources_sheets_find(file);
        if (data) {
            sheet = GL_sheet_decode(data->data, data->size, data->cell_width, data->cell_height, surface_callback_indexes, (void *)indexes);
            if (!sheet) {
                return luaL_error(L, "can't decode sheet `%s`", file);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet `%s` decoded", file);
        } else {
            File_System_Chunk_t chunk = FSaux_load(file_system, file, FILE_SYSTEM_CHUNK_BLOB);
            if (chunk.type == FILE_SYSTEM_CHUNK_NULL) {
                return luaL_error(L, "can't load file `%s`", file);
            }
            sheet = GL_sheet_decode(chunk.var.blob.ptr, chunk.var.blob.size, glyph_width, glyph_height, surface_callback_indexes, (void *)indexes);
            FSaux_release(chunk);
            if (!sheet) {
                return luaL_error(L, "can't decode %d bytes sheet", chunk.var.blob.size);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p loaded from file `%s`", sheet, file);
        }
    } else
    if (type == LUA_TUSERDATA) {
        const Canvas_Class_t *canvas = (const Canvas_Class_t *)lua_touserdata(L, 1);
        reference = luaX_ref(L, 2);

        sheet = GL_sheet_attach(canvas->context->surface, glyph_width, glyph_height);
        if (!sheet) {
            return luaL_error(L, "can't attach sheet");
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached to canvas %p", sheet, canvas);
    }

    Font_Class_t *instance = (Font_Class_t *)lua_newuserdata(L, sizeof(Font_Class_t));
    *instance = (Font_Class_t){
            .context = display->context,
            .sheet = sheet,
            .reference = reference
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p allocated w/ sheet %p for default context", instance, sheet);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int font_new6(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 6)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING, LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Class_t *canvas = (const Canvas_Class_t *)lua_touserdata(L, 1);
    int type = lua_type(L, 2);
    size_t glyph_width = (size_t)lua_tointeger(L, 3);
    size_t glyph_height = (size_t)lua_tointeger(L, 4);
    GL_Pixel_t background_index = (GL_Pixel_t)lua_tointeger(L, 5);
    GL_Pixel_t foreground_index = (GL_Pixel_t)lua_tointeger(L, 6);

    const File_System_t *file_system = (const File_System_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));

    luaX_Reference reference = LUAX_REFERENCE_NIL;
    GL_Sheet_t *sheet;
    if (type == LUA_TSTRING) {
        const char *file = lua_tostring(L, 2);

        const GL_Pixel_t indexes[] = { background_index, foreground_index };

        const Sheet_Data_t *data = resources_sheets_find(file);
        if (data) {
            sheet = GL_sheet_decode(data->data, data->size, data->cell_width, data->cell_height, surface_callback_indexes, (void *)indexes);
            if (!sheet) {
                return luaL_error(L, "can't decode sheet `%s`", file);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet `%s` decoded", file);
        } else {
            File_System_Chunk_t chunk = FSaux_load(file_system, file, FILE_SYSTEM_CHUNK_BLOB);
            if (chunk.type == FILE_SYSTEM_CHUNK_NULL) {
                return luaL_error(L, "can't load file `%s`", file);
            }
            sheet = GL_sheet_decode(chunk.var.blob.ptr, chunk.var.blob.size, glyph_width, glyph_height, surface_callback_indexes, (void *)indexes);
            FSaux_release(chunk);
            if (!sheet) {
                return luaL_error(L, "can't decode %d bytes sheet", chunk.var.blob.size);
            }
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p loaded from file `%s`", sheet, file);
        }
    } else
    if (type == LUA_TUSERDATA) {
        const Canvas_Class_t *canvas = (const Canvas_Class_t *)lua_touserdata(L, 2);
        reference = luaX_ref(L, 2);

        sheet = GL_sheet_attach(canvas->context->surface, glyph_width, glyph_height);
        if (!sheet) {
            return luaL_error(L, "can't attach sheet");
        }
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p attached to canvas %p", sheet, canvas);
    }

    Font_Class_t *instance = (Font_Class_t *)lua_newuserdata(L, sizeof(Font_Class_t));
    *instance = (Font_Class_t){
            .context = canvas->context,
            .sheet = sheet,
            .reference = reference
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p allocated w/ sheet %p for context %p", instance, sheet, canvas->context);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int font_new(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, font_new3)
        LUAX_OVERLOAD_ARITY(4, font_new4)
        LUAX_OVERLOAD_ARITY(5, font_new5)
        LUAX_OVERLOAD_ARITY(6, font_new6)
    LUAX_OVERLOAD_END
}

static int font_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);

    if (instance->reference != LUAX_REFERENCE_NIL) {
        luaX_unref(L, instance->reference);
        GL_sheet_detach(instance->sheet);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p detached", instance->sheet);
    } else {
        GL_sheet_destroy(instance->sheet);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "sheet %p destroyed", instance->sheet);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font %p finalized", instance);

    return 0;
}

static void _size(const char *text, int dw, int dh, int *w, int *h)
{
    if (text[0] == '\0') {
        *w = *h = 0;
        return;
    }

    *h  = dh;
    int max_length =0, length = 0;
    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
        char c = *ptr;
#ifndef __NO_LINEFEEDS__
        if (c == '\n') {
            *h += dh;
            if (max_length < length) {
                max_length = length;
            }
            length = 0;
            continue;
        } else
#endif
        if (c < ' ') {
            continue;
        }
        length += 1;
    }
    if (max_length < length) {
        max_length = length;
    }
    *w = max_length * dw;
}

static int font_width1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->sheet->size.width);

    return 1;
}

static int font_width2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);
    const char *text = lua_tostring(L, 2);

    int dw = instance->sheet->size.width;

    int width, height;
    _size(text, dw, 0, &width, &height);

    lua_pushinteger(L, width);

    return 1;
}

static int font_width3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);
    const char *text = lua_tostring(L, 2);
    float scale = lua_tonumber(L, 3);

    int dw = (int)(instance->sheet->size.width * fabsf(scale));

    int width, height;
    _size(text, dw, 0, &width, &height);

    lua_pushinteger(L, width);

    return 1;
}

static int font_width(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, font_width1)
        LUAX_OVERLOAD_ARITY(2, font_width2)
        LUAX_OVERLOAD_ARITY(3, font_width3)
    LUAX_OVERLOAD_END
}

static int font_height1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->sheet->size.height);

    return 1;
}

static int font_height2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);
    const char *text = lua_tostring(L, 2);

    int dh = instance->sheet->size.height;

    int width, height;
    _size(text, 0, dh, &width, &height);

    lua_pushinteger(L, height);

    return 1;
}

static int font_height3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);
    const char *text = lua_tostring(L, 2);
    float scale = lua_tonumber(L, 3);

    int dh = (int)(instance->sheet->size.height * fabsf(scale));

    int width, height;
    _size(text, 0, dh, &width, &height);

    lua_pushinteger(L, height);

    return 1;
}

static int font_height(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, font_height1)
        LUAX_OVERLOAD_ARITY(2, font_height2)
        LUAX_OVERLOAD_ARITY(3, font_height3)
    LUAX_OVERLOAD_END
}

static int font_size1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->sheet->size.width);
    lua_pushinteger(L, instance->sheet->size.height);

    return 2;
}

static int font_size2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);
    const char *text = lua_tostring(L, 2);

    int dw = instance->sheet->size.width;
    int dh = instance->sheet->size.height;

    int width, height;
    _size(text, dw, dh, &width, &height);

    lua_pushinteger(L, width);
    lua_pushinteger(L, height);

    return 2;
}

static int font_size3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);
    const char *text = lua_tostring(L, 2);
    float scale = lua_tonumber(L, 3);

    int dw = (int)(instance->sheet->size.width * fabsf(scale));
    int dh = (int)(instance->sheet->size.height * fabsf(scale));

    int width, height;
    _size(text, dw, dh, &width, &height);

    lua_pushinteger(L, width);
    lua_pushinteger(L, height);

    return 2;
}

static int font_size4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);
    const char *text = lua_tostring(L, 2);
    float scale_x = lua_tonumber(L, 3);
    float scale_y = lua_tonumber(L, 4);

    int dw = (int)(instance->sheet->size.width * fabsf(scale_x));
    int dh = (int)(instance->sheet->size.height * fabsf(scale_y));

    int width, height;
    _size(text, dw, dh, &width, &height);

    lua_pushinteger(L, width);
    lua_pushinteger(L, height);

    return 2;
}

static int font_size(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, font_size1)
        LUAX_OVERLOAD_ARITY(2, font_size2)
        LUAX_OVERLOAD_ARITY(3, font_size3)
        LUAX_OVERLOAD_ARITY(4, font_size4)
    LUAX_OVERLOAD_END
}

static int font_write4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);
    const char *text = lua_tostring(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);

    const GL_Context_t *context = instance->context;
    const GL_Sheet_t *sheet = instance->sheet;

    int dw = sheet->size.width;
#ifndef __NO_LINEFEEDS__
    int dh = sheet->size.height;
#endif

    int dx = x, dy = y;
    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
        char c = *ptr;
#ifndef __NO_LINEFEEDS__
        if (c == '\n') { // Handle carriage-return
            dx = x;
            dy += dh;
            continue;
        } else
#endif
        if (c < ' ') {
            continue;
        }
        GL_context_blit(context, sheet->atlas, sheet->cells[c - ' '], (GL_Point_t){ .x = dx, .y = dy });
        dx += dw;
    }

    return 0;
}

static int font_write5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);
    const char *text = lua_tostring(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    float scale = lua_tonumber(L, 5);

    const GL_Context_t *context = instance->context;
    const GL_Sheet_t *sheet = instance->sheet;

    int dw = (int)(sheet->size.width * fabsf(scale));
#ifndef __NO_LINEFEEDS__
    int dh = (int)(sheet->size.height * fabsf(scale));
#endif

    int dx = x, dy = y;
    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
        char c = *ptr;
#ifndef __NO_LINEFEEDS__
        if (c == '\n') { // Handle carriage-return
            dx = x;
            dy += dh;
            continue;
        } else
#endif
        if (c < ' ') {
            continue;
        }
        GL_context_blit_s(context, sheet->atlas, sheet->cells[c - ' '], (GL_Point_t){ .x = dx, .y = dy }, scale, scale);
        dx += dw;
    }

    return 0;
}

static int font_write6(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 6)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);
    const char *text = lua_tostring(L, 2);
    int x = lua_tointeger(L, 3); // TODO: make all arguments const?
    int y = lua_tointeger(L, 4);
    float scale_x = lua_tonumber(L, 5);
    float scale_y = lua_tonumber(L, 6);

    const GL_Context_t *context = instance->context;
    const GL_Sheet_t *sheet = instance->sheet;

    int dw = (int)(sheet->size.width * fabsf(scale_x));
#ifndef __NO_LINEFEEDS__
    int dh = (int)(sheet->size.height * fabsf(scale_y));
#endif

    float dx = x, dy = y;
    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
        char c = *ptr;
#ifndef __NO_LINEFEEDS__
        if (c == '\n') { // Handle carriage-return
            dx = x;
            dy += dh;
            continue;
        } else
#endif
        if (c < ' ') {
            continue;
        }
        GL_context_blit_s(context, sheet->atlas, sheet->cells[c - ' '], (GL_Point_t){ .x = dx, .y = dy }, scale_x, scale_y);
        dx += dw;
    }

    return 0;
}

static int font_write(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(4, font_write4)
        LUAX_OVERLOAD_ARITY(5, font_write5)
        LUAX_OVERLOAD_ARITY(6, font_write6)
    LUAX_OVERLOAD_END
}
