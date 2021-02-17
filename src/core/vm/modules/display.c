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

#include "display.h"

#include <config.h>
#include <core/io/display.h>
#include <libs/log.h>
#include <libs/stb.h>
#include <resources/palettes.h>

#include "udt.h"

#include <stdlib.h>

#define LOG_CONTEXT "graphics"

static int display_offset(lua_State *L);
static int display_palette(lua_State *L);
static int display_color_to_index(lua_State *L);
static int display_index_to_color(lua_State *L);
static int display_copperlist(lua_State *L);
static int display_shader(lua_State *L);
static int display_send(lua_State *L);

static const struct luaL_Reg _display_functions[] = {
    { "palette", display_palette },
    { "color_to_index", display_color_to_index },
    { "index_to_color", display_index_to_color },
    { "offset", display_offset },
    { "copperlist", display_copperlist },
    { "shader", display_shader },
    { "send", display_send },
    { NULL, NULL }
};

static const luaX_Const _display_constants[] = {
    { NULL, LUA_CT_NIL, { 0 } }
};

int display_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _display_functions, _display_constants, nup, NULL);
}

static int display_palette0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Palette_t *palette = Display_get_palette(display);

    lua_createtable(L, (int)palette->count, 0);
    for (size_t i = 0; i < palette->count; ++i) {
        unsigned int argb = GL_palette_pack_color(palette->colors[i]);

        lua_pushinteger(L, (lua_Integer)argb);
        lua_rawseti(L, -2, (lua_Integer)(i + 1));
    }

    return 1;
}

static int display_palette1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING, LUA_TTABLE)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Palette_t palette = { 0 };
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
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "setting custom palette of %d color(s)", palette.count);

        if (palette.count > GL_MAX_PALETTE_COLORS) {
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "palette has too many colors (%d) - clamping", palette.count);
            palette.count = GL_MAX_PALETTE_COLORS;
        }

        lua_pushnil(L);
        for (size_t i = 0; lua_next(L, 1); ++i) {
            uint32_t argb = (uint32_t)LUAX_INTEGER(L, -1);
            GL_Color_t color = GL_palette_unpack_color(argb);
            palette.colors[i] = color;

            lua_pop(L, 1);
        }
    }

    if (palette.count == 0) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "palette has no colors - skipping");
        return 0;
    }

    Display_set_palette(display, &palette);

    return 0;
}

static int display_palette(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, display_palette0)
        LUAX_OVERLOAD_ARITY(1, display_palette1)
    LUAX_OVERLOAD_END
}

static int display_color_to_index1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    uint32_t argb = (uint32_t)LUAX_INTEGER(L, 1);

    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Color_t color = GL_palette_unpack_color(argb);
    const GL_Pixel_t index = GL_palette_find_nearest_color(Display_get_palette(display), color);

    lua_pushinteger(L, (lua_Integer)index);

    return 1;
}

static int display_color_to_index3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    uint8_t r = (uint8_t)LUAX_INTEGER(L, 1);
    uint8_t g = (uint8_t)LUAX_INTEGER(L, 2);
    uint8_t b = (uint8_t)LUAX_INTEGER(L, 3);

    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    uint32_t argb = GL_palette_pack_color((GL_Color_t){  .a = 255, .r = r, .g = g, .b = b });
    GL_Color_t color = GL_palette_unpack_color(argb);
    const GL_Pixel_t index = GL_palette_find_nearest_color(Display_get_palette(display), color);

    lua_pushinteger(L, (lua_Integer)index);

    return 1;
}

static int display_color_to_index(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, display_color_to_index1)
        LUAX_OVERLOAD_ARITY(3, display_color_to_index3)
    LUAX_OVERLOAD_END
}

static int display_index_to_color(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 1);

    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Palette_t *palette = Display_get_palette(display);
    const GL_Color_t color = palette->colors[index];

    lua_pushinteger(L, (lua_Integer)color.r);
    lua_pushinteger(L, (lua_Integer)color.g);
    lua_pushinteger(L, (lua_Integer)color.b);

    return 3;
}

static int display_offset0_2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int x = LUAX_OPTIONAL_INTEGER(L, 1, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 2, 0);

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_set_offset(display, (GL_Point_t){ .x = x, .y = y });

    return 0;
}

static int display_offset(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, display_offset0_2)
        LUAX_OVERLOAD_ARITY(2, display_offset0_2)
    LUAX_OVERLOAD_END
}

static int display_copperlist0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_set_copperlist(display, NULL, 0);

    return 0;
}

static int display_copperlist1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_CopperList_Entry_t *copperlist = NULL;

    lua_pushnil(L);
    for (size_t i = 0; lua_next(L, 1); ++i) {
        lua_pushnil(L);
        lua_next(L, -2); const char *command = LUAX_STRING(L, -1); lua_pop(L, 1);

        if (command[0] == 'w') {
            lua_next(L, -2); const size_t x = (size_t)LUAX_INTEGER(L, -1); lua_pop(L, 1);
            lua_next(L, -2); const size_t y = (size_t)LUAX_INTEGER(L, -1); lua_pop(L, 1);

            arrpush(copperlist, (Display_CopperList_Entry_t){ .command = WAIT });
            arrpush(copperlist, (Display_CopperList_Entry_t){ .size = x });
            arrpush(copperlist, (Display_CopperList_Entry_t){ .size = y });
        } else
        if (command[0] == 'c') {
            lua_next(L, -2); const size_t index = (size_t)LUAX_INTEGER(L, -1); lua_pop(L, 1);
            lua_next(L, -2); const uint32_t argb = (uint32_t)LUAX_INTEGER(L, -1); lua_pop(L, 1);

            arrpush(copperlist, (Display_CopperList_Entry_t){ .command = COLOR });
            arrpush(copperlist, (Display_CopperList_Entry_t){ .integer = index });
            arrpush(copperlist, (Display_CopperList_Entry_t){ .color = GL_palette_unpack_color(argb) });
        } else
        if (command[0] == 'm') {
            lua_next(L, -2); const int amount = LUAX_INTEGER(L, -1); lua_pop(L, 1);

            arrpush(copperlist, (Display_CopperList_Entry_t){ .command = MODULO });
            arrpush(copperlist, (Display_CopperList_Entry_t){ .integer = amount });
        } else
        if (command[0] == 'o') {
            lua_next(L, -2); const int amount = LUAX_INTEGER(L, -1); lua_pop(L, 1);

            arrpush(copperlist, (Display_CopperList_Entry_t){ .command = OFFSET });
            arrpush(copperlist, (Display_CopperList_Entry_t){ .integer = amount });
        } else {
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "unrecognized command `%s` for copperlist", command);
        }

        lua_pop(L, 2);
    }

    arrpush(copperlist, (Display_CopperList_Entry_t){ .command = WAIT }); // Force an unreachable WAIT as optimization!
    arrpush(copperlist, (Display_CopperList_Entry_t){ .size = 9999 });
    arrpush(copperlist, (Display_CopperList_Entry_t){ .size = 9999 });

    Display_set_copperlist(display, copperlist, arrlen(copperlist));

    arrfree(copperlist);

    return 0;
}

static int display_copperlist(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, display_copperlist0)
        LUAX_OVERLOAD_ARITY(1, display_copperlist1)
    LUAX_OVERLOAD_END
}

static int display_shader(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *code = LUAX_STRING(L, 1);

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_set_shader(display, code);

    return 0;
}

static int display_send(lua_State *L)
{
/*
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER, LUA_TTABLE)
    LUAX_SIGNATURE_END
    const char *uniform = LUAX_STRING(L, 1);
    int type = lua_type(L, 2);

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    if (lua_isinteger(L, 2)) {
        Display_send(display, uniform, PROGRAM_UNIFORM_INT, 1, &value);
    } else
    if (lua_isnumber(L, 2)) {
        Display_send(display, uniform, PROGRAM_UNIFORM_FLOAT, 1, &value);
    } else
    if (lua_istable(L, 2)) {
        Display_send(display, uniform, PROGRAM_UNIFORM_FLOAT, 1, &value);
    }
*/
    return 0;
}
