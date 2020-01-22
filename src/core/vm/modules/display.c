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

#include "canvas.h"

#include <config.h>
#include <core/io/display.h>
#include <libs/imath.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "udt.h"
#include "resources/palettes.h"

#include <math.h>
#include <string.h>
#include <time.h>

#define LOG_CONTEXT "graphics"
#define META_TABLE  "Tofu_Graphics_Display_mt"

static int display_offset(lua_State *L);
static int display_palette(lua_State *L);
static int display_color_to_index(lua_State *L);
static int display_shader(lua_State *L);

static const struct luaL_Reg _display_functions[] = {
    { "palette", display_palette },
    { "color_to_index", display_color_to_index },
    { "offset", display_offset },
    { "shader", display_shader },
    { NULL, NULL }
};

int display_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _display_functions, NULL, nup, META_TABLE);
}

static int display_palette0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END

    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Palette_t *palette = &display->palette;

    lua_createtable(L, palette->count, 0);
    for (size_t i = 0; i < palette->count; ++i) {
        unsigned int argb = GL_palette_pack_color(palette->colors[i]);

        lua_pushinteger(L, argb);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static int display_palette1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING, LUA_TTABLE)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Palette_t palette;
    if (type == LUA_TSTRING) { // Predefined palette!
        const char *id = luaL_checkstring(L, 1);
        const GL_Palette_t *predefined_palette = resources_palettes_find(id);
        if (predefined_palette != NULL) {
            palette = *predefined_palette;

            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "setting predefined palette `%s` w/ %d color(s)", id, predefined_palette->count);
        } else {
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "unknown predefined palette w/ id `%s`", id);
        }
    } else
    if (type == LUA_TTABLE) { // User supplied palette.
        palette.count = lua_rawlen(L, 1);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "setting custom palette of #%d color(s)", palette.count);

        if (palette.count > GL_MAX_PALETTE_COLORS) {
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "palette has too many colors (%d) - clamping", palette.count);
            palette.count = GL_MAX_PALETTE_COLORS;
        }

        lua_pushnil(L);
        for (size_t i = 0; lua_next(L, 1); ++i) {
            uint32_t argb = (uint32_t)lua_tointeger(L, -1);
            GL_Color_t color = GL_palette_unpack_color(argb);
            palette.colors[i] = color;

            lua_pop(L, 1);
        }
    }

    if (palette.count == 0) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "palette has no colors - skipping");
        return 0;
    }

    Display_palette(display, &palette);

    return 0;
}

static int display_palette(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, display_palette0)
        LUAX_OVERLOAD_ARITY(1, display_palette1)
    LUAX_OVERLOAD_END
}

static int display_color_to_index(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    uint32_t argb = (uint32_t)lua_tointeger(L, 1);

    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Color_t color = GL_palette_unpack_color(argb);
    const GL_Pixel_t index = GL_palette_find_nearest_color(&display->palette, color);

    lua_pushinteger(L, index);

    return 1;
}

static int display_offset0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_offset(display, (GL_Point_t){ .x = 0, .y = 0 });

    return 0;
}

static int display_offset2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_offset(display, (GL_Point_t){ .x = x, .y = y });

    return 0;
}

static int display_offset(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, display_offset0)
        LUAX_OVERLOAD_ARITY(2, display_offset2)
    LUAX_OVERLOAD_END
}

static int display_shader(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *code = lua_tostring(L, 1);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_shader(display, code);

    return 0;
}
