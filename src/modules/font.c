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

#include "font.h"

#include "../core/luax.h"

#include "../config.h"
#include "../environment.h"
#include "../log.h"
#include "../gl/gl.h"

#include "graphics/palettes.h"
#include "graphics/sheets.h"

#include <math.h>
#include <string.h>

typedef struct _Font_Class_t {
    // char pathfile[PATH_FILE_MAX];
    GL_Sheet_t sheet;
} Font_Class_t;

static int font_new(lua_State *L);
static int font_gc(lua_State *L);
static int font_write(lua_State *L);

static const char _font_script[] =
    "local Font = {}\n"
    "\n"
    "--Font.__index = Font\n"
    "\n"
    "Font.default = function(background_color, foreground_color)\n"
    "  return Font.new(\"5x8\", 0, 0, background_color, foreground_color)\n"
    "end\n"
    "\n"
    "return Font\n"
;

static const struct luaL_Reg _font_functions[] = {
    { "new", font_new },
    {"__gc", font_gc },
    { "write", font_write },
    { NULL, NULL }
};

static const luaX_Const _font_constants[] = {
    { NULL }
};

int font_loader(lua_State *L)
{
    lua_pushvalue(L, lua_upvalueindex(1)); // Duplicate the upvalue to pass it to the module.
    return luaX_newmodule(L, _font_script, _font_functions, _font_constants, 1, LUAX_CLASS(Font_Class_t));
}

static void to_font_atlas_callback(void *parameters, GL_Surface_t *surface, const void *data)
{
    const GL_Pixel_t *colors = (const GL_Pixel_t *)parameters;

    const GL_Color_t *src = (const GL_Color_t *)data;
    GL_Pixel_t *dst = (GL_Pixel_t *)surface->data;

    for (size_t y = 0; y < surface->height; ++y) {
        int row_offset = surface->width * y;

        for (size_t x = 0; x < surface->width; ++x) {
            size_t offset = row_offset + x;

            GL_Color_t color = src[offset];
            dst[offset] = color.a == 0 ? colors[0] : colors[1]; // TODO: don't use alpha for transparency?
        }
    }
}

static int font_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    const char *file = lua_tostring(L, 1);
    int glyph_width = lua_tointeger(L, 2);
    int glyph_height = lua_tointeger(L, 3);
    size_t background_color = lua_tointeger(L, 4);
    size_t foreground_color = lua_tointeger(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.new() -> %s, %d, %d, %d, %d", file, glyph_width, glyph_height, background_color, foreground_color);
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    const Sheet_Data_t *data = graphics_sheets_find(file);

    const GL_Pixel_t colors[] = { background_color, foreground_color };
    GL_Sheet_t sheet;
    if (data) {
        GL_sheet_decode(&sheet, data->buffer, data->size, data->quad_width, data->quad_height, to_font_atlas_callback, (void *)colors);
        Log_write(LOG_LEVELS_DEBUG, "<FONT> sheet '%s' decoded", file);
    } else {
        char pathfile[PATH_FILE_MAX] = {};
        strcpy(pathfile, environment->base_path);
        strcat(pathfile, file);

        GL_sheet_load(&sheet, pathfile, glyph_width, glyph_height, to_font_atlas_callback, (void *)colors);
        Log_write(LOG_LEVELS_DEBUG, "<FONT> sheet '%s' loaded", pathfile);
    }

    Font_Class_t *instance = (Font_Class_t *)lua_newuserdata(L, sizeof(Font_Class_t));
    *instance = (Font_Class_t){
            .sheet = sheet
        };
    Log_write(LOG_LEVELS_DEBUG, "<FONT> font allocated as #%p", instance);

    luaL_setmetatable(L, LUAX_CLASS(Font_Class_t));

    return 1;
}

static int font_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);

    GL_sheet_delete(&instance->sheet);
    Log_write(LOG_LEVELS_DEBUG, "<FONT> font #%p finalized", instance);

    *instance = (Font_Class_t){};

    return 0;
}

static int font_write(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 6)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
    LUAX_SIGNATURE_END
    Font_Class_t *instance = (Font_Class_t *)lua_touserdata(L, 1);
    const char *text = lua_tostring(L, 2);
    double x = (double)lua_tonumber(L, 3);
    double y = (double)lua_tonumber(L, 4);
    double scale = (double)lua_tonumber(L, 5);
    const char *alignment = lua_tostring(L, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.write() -> %s, %d, %d, %f, %s", text, x, y, scale, alignment);
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    const GL_Context_t *context = &environment->display->gl.context;
    const GL_Sheet_t *sheet = &instance->sheet;
    GL_Surface_t *target = &environment->display->gl.surface;

    double dw = sheet->size.width * fabs(scale);
#ifndef __NO_LINEFEEDS__
    double dh = sheet->tile.height * fabs(scale);
#endif

#ifndef __NO_LINEFEEDS__
    size_t width = 0;
    size_t slen = strlen(text);
    size_t offset = 0;
    while (offset < slen) {
        const char *start = text + offset;
        const char *end = strchr(start, '\n');
        if (!end) {
            end = text + slen;
        }
        size_t length = end - start;
        if (width < length * dw) {
            width = length * dw;
        }
        offset += length + 1;
    }
#else
    size_t width = strlen(text) * dw;
#endif

    int dx, dy; // Always pixel-aligned positions.
    if (alignment[0] == 'l') {
        dx = (int)x;
        dy = (int)y;
    } else
    if (alignment[0] == 'c') {
        dx = (int)(x - (width * 0.5f));
        dy = (int)y;
    } else
    if (alignment[0] == 'r') {
        dx = (int)(x - width);
        dy = (int)y;
    } else {
        dx = (int)x;
        dy = (int)y;
    }
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.write() -> %d, %d, %d", width, dx, dy);
#endif

    GL_Point_t position = { .x = dx, .y = dy };
    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
#ifndef __NO_LINEFEEDS__
        if (*ptr == '\n') { // Handle carriage-return
            destination.x = dx;
            destination.y += dh;
            continue;
        } else
#endif
        if (*ptr < ' ') {
            continue;
        }
        GL_sheet_blit(context, sheet, *ptr - ' ', target, position, (float)scale, 0.0f);
        position.x += dw;
    }

    return 0;
}
