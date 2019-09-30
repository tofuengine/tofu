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

#include "canvas.h"

#include "../core/luax.h"

#include "../config.h"
#include "../environment.h"
#include "../log.h"
#include "../gl/gl.h"

#include "graphics/palettes.h"
#include "graphics/sheets.h"

#include <math.h>
#include <string.h>
#include <time.h>

typedef struct _Canvas_Class_t {
} Canvas_Class_t;

static int canvas_color_to_index(lua_State *L);
static int canvas_screenshot(lua_State *L);
static int canvas_width(lua_State *L);
static int canvas_height(lua_State *L);
static int canvas_palette(lua_State *L);
static int canvas_background(lua_State *L);
static int canvas_color(lua_State *L);
static int canvas_pattern(lua_State *L);
static int canvas_shift(lua_State *L);
static int canvas_transparent(lua_State *L);
static int canvas_clipping(lua_State *L);
static int canvas_shader(lua_State *L);
static int canvas_clear(lua_State *L);
static int canvas_point(lua_State *L);
static int canvas_hline(lua_State *L);
static int canvas_vline(lua_State *L);
static int canvas_line(lua_State *L);
static int canvas_fill(lua_State *L);
static int canvas_triangle(lua_State *L);
static int canvas_rectangle(lua_State *L);

// TODO: color index is optional, if not present use the current (drawstate) pen color

static const struct luaL_Reg _canvas_functions[] = {
    { "color_to_index", canvas_color_to_index },
    { "screenshot", canvas_screenshot },
    { "width", canvas_width },
    { "height", canvas_height },
    { "palette", canvas_palette },
    { "background", canvas_background },
    { "color", canvas_color },
    { "pattern", canvas_pattern },
    { "shift", canvas_shift },
    { "transparent", canvas_transparent },
    { "clipping", canvas_clipping },
    { "shader", canvas_shader },
    { "clear", canvas_clear },
    { "point", canvas_point },
    { "hline", canvas_hline },
    { "vline", canvas_vline },
    { "line", canvas_line },
    { "fill", canvas_fill },
    { "triangle", canvas_triangle },
    { "rectangle", canvas_rectangle },
    { NULL, NULL }
};

static const luaX_Const _canvas_constants[] = {
    { NULL }
};

#include "canvas.inc"

int canvas_loader(lua_State *L)
{
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, (const char *)_canvas_lua, _canvas_functions, _canvas_constants, nup, LUAX_CLASS(Canvas_Class_t));
}

static int canvas_color_to_index(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
    LUAX_SIGNATURE_END
    const char *argb = lua_tostring(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.color_to_index('%s')", argb);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Color_t color = GL_palette_parse_color(argb);
    const GL_Pixel_t index = GL_palette_find_nearest_color(&display->palette, color);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "color '%s' mapped to index %d", argb, index);
#endif

    lua_pushinteger(L, index);

    return 1;
}

static int canvas_screenshot(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
    LUAX_SIGNATURE_END
    const char *prefix = lua_tostring(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.screenshot('%s')", file);
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));
    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    time_t marker = time(NULL); // Generate a timedate based name.
    struct tm *now = localtime(&marker);
    char file[32] = {};
    sprintf(file, "%s-%04d%02d%02d%02d%02d%02d.png", prefix,
        1900 + now->tm_year, now->tm_mon + 1, now->tm_mday,
        now->tm_hour, now->tm_min, now->tm_sec);

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file);

    GL_context_screenshot(&display->gl, pathfile);

    return 0;
}

static int canvas_width(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.width()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    lua_pushinteger(L, display->gl.width);

    return 1;
}

static int canvas_height(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.height()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    lua_pushinteger(L, display->gl.height);

    return 1;
}

static int canvas_palette0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.palette()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_Palette_t *palette = &display->palette;

    lua_newtable(L);
    for (size_t i = 0; i < palette->count; ++i) {
        char argb[12] = {};
        GL_palette_format_color(argb, palette->colors[i]);

        lua_pushstring(L, argb);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static int canvas_palette1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring, luaX_istable)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.palette(%d)", type);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_Palette_t palette = {};

    if (type == LUA_TSTRING) { // Predefined palette!
        const char *id = luaL_checkstring(L, 1);
        const GL_Palette_t *predefined_palette = graphics_palettes_find(id);
        if (predefined_palette != NULL) {
            palette = *predefined_palette;

            Log_write(LOG_LEVELS_DEBUG, "<CANVAS> setting predefined palette '%s' w/ %d color(s)", id, predefined_palette->count);
        } else {
            Log_write(LOG_LEVELS_WARNING, "<CANVAS> unknown predefined palette w/ id '%s'", id);
        }
    } else
    if (type == LUA_TTABLE) { // User supplied palette.
        palette.count = lua_rawlen(L, 1);
        Log_write(LOG_LEVELS_DEBUG, "<CANVAS> setting custom palette of #%d color(s)", palette.count);

        if (palette.count > GL_MAX_PALETTE_COLORS) {
            Log_write(LOG_LEVELS_WARNING, "<CANVAS> palette has too many colors (%d) - clamping", palette.count);
            palette.count = GL_MAX_PALETTE_COLORS;
        }

        lua_pushnil(L); // first key
        for (int i = 0; lua_next(L, 1); ++i) {
#if 0
            const char *key_type = lua_typename(L, lua_type(L, -2)); // uses 'key' (at index -2) and 'value' (at index -1)
#endif
            const char *argb = lua_tostring(L, -1);
            palette.colors[i] = GL_palette_parse_color(argb);

            lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
        }
    } else {
        Log_write(LOG_LEVELS_ERROR, "<CANVAS> wrong palette type, need to be string or list");
    }

    if (palette.count == 0) {
        return 0;
    }

    Display_palette(display, &palette);

    return 0;
}

static int canvas_palette(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_palette0)
        LUAX_OVERLOAD_ARITY(1, canvas_palette1)
    LUAX_OVERLOAD_END
}

static int canvas_background(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    GL_Pixel_t index = lua_tointeger(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.background(%d)", index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    GL_context_background(&display->gl, index);

    return 0;
}

static int canvas_color(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    GL_Pixel_t index = lua_tointeger(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.color(%d)", index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    GL_context_color(&display->gl, index);

    return 0;
}

static int canvas_pattern(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int mask = lua_tointeger(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.pattern(%08x)", mask);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_context_pattern(&display->gl, mask);

    return 0;
}

static int canvas_shift0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.shift()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_context_shifting(&display->gl, NULL, NULL, 0);

    return 0;
}

static int canvas_shift1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    int type = lua_type(L, 1);
    Log_write(LOG_LEVELS_DEBUG, "Canvas.shift(%d)", type);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    int count = luaX_count(L, 1);

    size_t from[count];
    size_t to[count];
    lua_pushnil(L); // first key
    for (size_t i = 0; lua_next(L, 1); ++i) {
        from[i] = lua_tointeger(L, -2);
        to[i] = lua_tointeger(L, -1);

        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }

    GL_context_shifting(&display->gl, from, to, count);

    return 0;
}

static int canvas_shift2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    size_t from = lua_tointeger(L, 1);
    size_t to = lua_tointeger(L, 2);
#ifdef __DEBUG_API_CALLS__
    int type = lua_type(L, 1);
    Log_write(LOG_LEVELS_DEBUG, "Canvas.shift(%d, %d)", from, to);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_context_shifting(&display->gl, &from, &to, 1);

    return 0;
}

static int canvas_shift(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_shift0)
        LUAX_OVERLOAD_ARITY(1, canvas_shift1)
        LUAX_OVERLOAD_ARITY(2, canvas_shift2)
    LUAX_OVERLOAD_END
}

static int canvas_transparent0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.transparent()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_context_transparent(&display->gl, NULL, NULL, 0);

    return 0;
}

static int canvas_transparent1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    int type = lua_type(L, 1);
    Log_write(LOG_LEVELS_DEBUG, "Canvas.transparent(%d)", type);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    int count = luaX_count(L, 1);

    GL_Pixel_t indexes[count];
    GL_Bool_t transparent[count];
    lua_pushnil(L); // first key
    for (size_t i = 0; lua_next(L, 1); ++i) {
        indexes[i] = lua_tointeger(L, -2);
        transparent[i] = lua_toboolean(L, -1) ? GL_BOOL_TRUE : GL_BOOL_FALSE;

        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }

    GL_context_transparent(&display->gl, indexes, transparent, count);

    return 0;
}

static int canvas_transparent2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isboolean)
    LUAX_SIGNATURE_END
    GL_Pixel_t index = lua_tointeger(L, 1);
    GL_Bool_t transparent = lua_toboolean(L, 2) ? GL_BOOL_TRUE : GL_BOOL_FALSE;
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.transparent(%d, %d)", color, transparent);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    GL_context_transparent(&display->gl, &index, &transparent, 1);

    return 0;
}

static int canvas_transparent(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_transparent0)
        LUAX_OVERLOAD_ARITY(1, canvas_transparent1)
        LUAX_OVERLOAD_ARITY(2, canvas_transparent2)
    LUAX_OVERLOAD_END
}

static int canvas_clipping0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.clipping()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_context_clipping(&display->gl, NULL);

    return 0;
}

static int canvas_clipping4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int x0 = lua_tointeger(L, 1);
    int y0 = lua_tointeger(L, 2);
    int x1 = lua_tointeger(L, 3);
    int y1 = lua_tointeger(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.clipping(%d, %d, %d, %d)", x0, y0, x1, y1);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_Quad_t clipping_region = (GL_Quad_t){
            .x0 = x0,
            .y0 = y0,
            .x1 = x1,
            .y1 = y1
        };

    GL_context_clipping(&display->gl, &clipping_region);

    return 0;
}

static int canvas_clipping(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_clipping0)
        LUAX_OVERLOAD_ARITY(4, canvas_clipping4)
    LUAX_OVERLOAD_END
}

static int canvas_shader(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
    LUAX_SIGNATURE_END
    const char *code = lua_tostring(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.shader('%s')", code);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    Display_shader(display, code);

    return 0;
}

static int canvas_clear(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.clear()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_context_clear(&display->gl);

    return 0;
}

static int canvas_point(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    double x = lua_tonumber(L, 1);
    double y = lua_tonumber(L, 2);
    GL_Pixel_t index = lua_tointeger(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.point(%f, %f, %d)", x, y, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    GL_primitive_point(&display->gl, (GL_Point_t){ (int)x, (int)y }, index);

    return 0;
}

static int canvas_hline(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    double x = lua_tonumber(L, 1);
    double y = lua_tonumber(L, 2);
    double width = lua_tonumber(L, 3);
    GL_Pixel_t index = lua_tointeger(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.hline(%f, %f, %f, %d)", x, y, width, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    GL_primitive_hline(&display->gl, (GL_Point_t){ (int)x, (int)y }, (size_t)width, index);

    return 0;
}

static int canvas_vline(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    double x = lua_tonumber(L, 1);
    double y = lua_tonumber(L, 2);
    double height = lua_tonumber(L, 3);
    GL_Pixel_t index = lua_tointeger(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.vline(%f, %f, %f, %d)", x, y, height, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    GL_primitive_vline(&display->gl, (GL_Point_t){ (int)x, (int)y }, (size_t)height, index);

    return 0;
}

static int canvas_line(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    double x0 = lua_tonumber(L, 1);
    double y0 = lua_tonumber(L, 2);
    double x1 = lua_tonumber(L, 3);
    double y1 = lua_tonumber(L, 4);
    GL_Pixel_t index = lua_tointeger(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.line(%f, %f, %f, %f, %d)", x0, y0, x1, y1, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    GL_primitive_line(&display->gl, (GL_Point_t){ (int)x0, (int)y0 }, (GL_Point_t){ (int)x1, (int)y1 }, index);

    return 0;
}

static int canvas_fill(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    double x = lua_tonumber(L, 1);
    double y = lua_tonumber(L, 2);
    GL_Pixel_t index = lua_tointeger(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.fill(%f, %f, %d)", x, y, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    GL_context_fill(&display->gl, (GL_Point_t){ (int)x, (int)y }, index);

    return 0;
}

static int canvas_triangle(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 8)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    const char *mode = lua_tostring(L, 1);
    double x0 = lua_tonumber(L, 2);
    double y0 = lua_tonumber(L, 3);
    double x1 = lua_tonumber(L, 4);
    double y1 = lua_tonumber(L, 5);
    double x2 = lua_tonumber(L, 6);
    double y2 = lua_tonumber(L, 7);
    GL_Pixel_t index = lua_tointeger(L, 8);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.triangle(%s, %f, %f, %f, %f, %f, %f, %d)", mode, x0, y0, x1, y1, x2, y2, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    if (mode[0] == 'l') {
        GL_primitive_line(&display->gl, (GL_Point_t){ (int)x0, (int)y0 }, (GL_Point_t){ (int)x1, (int)y1 }, index);
        GL_primitive_line(&display->gl, (GL_Point_t){ (int)x1, (int)y1 }, (GL_Point_t){ (int)x2, (int)y2 }, index);
        GL_primitive_line(&display->gl, (GL_Point_t){ (int)x2, (int)y2 }, (GL_Point_t){ (int)x0, (int)y0 }, index);
    } else {
        GL_primitive_triangle(&display->gl, (GL_Point_t){ (int)x0, (int)y0 }, (GL_Point_t){ (int)x1, (int)y1 }, (GL_Point_t){ (int)x2, (int)y2 }, index);
    }

    return 0;
}

static int canvas_rectangle(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 6)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    const char *mode = lua_tostring(L, 1);
    double x = lua_tonumber(L, 2);
    double y = lua_tonumber(L, 3);
    double width = lua_tonumber(L, 4);
    double height = lua_tonumber(L, 5);
    GL_Pixel_t index = lua_tointeger(L, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.rectangle(%s, %f, %f, %f, %f, %d)", mode, x, y, width, height, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    if (mode[0] == 'l') {
        double x0 = x;
        double y0 = y;
        double x1 = x0 + width - 1.0f;
        double y1 = y0 + height - 1.0f;

        GL_primitive_line(&display->gl, (GL_Point_t){ (int)x0, (int)y0 }, (GL_Point_t){ (int)x0, (int)y1 }, index);
        GL_primitive_line(&display->gl, (GL_Point_t){ (int)x0, (int)y1 }, (GL_Point_t){ (int)x1, (int)y1 }, index);
        GL_primitive_line(&display->gl, (GL_Point_t){ (int)x1, (int)y1 }, (GL_Point_t){ (int)x1, (int)y0 }, index);
        GL_primitive_line(&display->gl, (GL_Point_t){ (int)x1, (int)y0 }, (GL_Point_t){ (int)x0, (int)y0 }, index);
    } else {
        GL_primitive_rectangle(&display->gl, (GL_Rectangle_t){ (int)x, (int)y, (int)width, (int)height }, index);
    }

    return 0;
}
