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
static int canvas_push(lua_State *L);
static int canvas_pop(lua_State *L);
static int canvas_surface(lua_State *L);
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
static int canvas_polyline(lua_State *L);
static int canvas_fill(lua_State *L);
static int canvas_triangle(lua_State *L);
static int canvas_rectangle(lua_State *L);
static int canvas_circle(lua_State *L);

// TODO: color index is optional, if not present use the current (drawstate) pen color
// TODO: rename `Canvas` to `Context`?

static const struct luaL_Reg _canvas_functions[] = {
    { "color_to_index", canvas_color_to_index },
    { "screenshot", canvas_screenshot },
    { "width", canvas_width },
    { "height", canvas_height },
    { "push", canvas_push },
    { "pop", canvas_pop },
    { "surface", canvas_surface },
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
    { "polyline", canvas_polyline },
    { "fill", canvas_fill },
    { "triangle", canvas_triangle },
    { "rectangle", canvas_rectangle },
    { "circle", canvas_circle },
    { NULL, NULL }
};

static const luaX_Const _canvas_constants[] = {
    { NULL }
};

#include "canvas.inc"

int canvas_loader(lua_State *L)
{
    luaX_Script script = { (const char *)_canvas_lua, _canvas_lua_len, "canvas.lua" };
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, &script, _canvas_functions, _canvas_constants, nup, LUAX_CLASS(Canvas_Class_t));
}

// TODO: add a canvas constructor with overload (from file, from WxH, default one). Surface will become Canvas, in the end.

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

    const GL_Context_t *context = &display->gl;
    GL_context_screenshot(context, &display->palette, pathfile);

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

    const GL_Context_t *context = &display->gl;

    lua_pushinteger(L, context->state.surface->width);

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

    const GL_Context_t *context = &display->gl;

    lua_pushinteger(L, context->state.surface->height);

    return 1;
}

static int canvas_push(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.push()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_Context_t *context = &display->gl;
    GL_context_push(context);

    return 0;
}

static int canvas_pop(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.pop()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_Context_t *context = &display->gl;
    GL_context_pop(context);

    return 0;
}

static int canvas_surface0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.surface()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_Context_t *context = &display->gl;
    GL_context_surface(context, NULL);

    return 0;
}

// TODO: !!! MOVE THESE `*_Class_t` UDT to a separate header or move to header file.
typedef struct _Surface_Class_t {
    // char pathfile[PATH_FILE_MAX];
    GL_Surface_t surface;
    GL_XForm_t xform;
} Surface_Class_t;

static int canvas_surface1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Surface_Class_t *surface = (Surface_Class_t *)lua_touserdata(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.surface(%p)", type);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_Context_t *context = &display->gl;
    GL_context_surface(context, &surface->surface);

    return 0;
}

static int canvas_surface(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_surface0)
        LUAX_OVERLOAD_ARITY(1, canvas_surface1)
    LUAX_OVERLOAD_END
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
        for (size_t i = 0; lua_next(L, 1); ++i) {
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
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.background(%d)", index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    GL_Context_t *context = &display->gl;
    GL_context_background(context, index);

    return 0;
}

static int canvas_color(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.color(%d)", index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    GL_Context_t *context = &display->gl;
    GL_context_color(context, index);

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

    GL_Context_t *context = &display->gl;
    GL_context_pattern(context, mask);

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

    GL_Context_t *context = &display->gl;
    GL_context_shifting(context, NULL, NULL, 0);

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

    size_t count = luaX_count(L, 1);

    size_t from[count];
    size_t to[count];
    lua_pushnil(L); // first key
    for (size_t i = 0; lua_next(L, 1); ++i) {
        from[i] = lua_tointeger(L, -2);
        to[i] = lua_tointeger(L, -1);

        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }

    GL_Context_t *context = &display->gl;
    GL_context_shifting(context, from, to, count);

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

    GL_Context_t *context = &display->gl;
    GL_context_shifting(context, &from, &to, 1);

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

    GL_Context_t *context = &display->gl;
    GL_context_transparent(context, NULL, NULL, 0);

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

    size_t count = luaX_count(L, 1);

    GL_Pixel_t indexes[count]; // TOOD: use hashes over VLAs?
    GL_Bool_t transparent[count];
    lua_pushnil(L); // first key
    for (size_t i = 0; lua_next(L, 1); ++i) {
        indexes[i] = (GL_Pixel_t)lua_tointeger(L, -2);
        transparent[i] = lua_toboolean(L, -1) ? GL_BOOL_TRUE : GL_BOOL_FALSE;

        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }

    GL_Context_t *context = &display->gl;
    GL_context_transparent(context, indexes, transparent, count);

    return 0;
}

static int canvas_transparent2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isboolean)
    LUAX_SIGNATURE_END
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 1);
    GL_Bool_t transparent = lua_toboolean(L, 2) ? GL_BOOL_TRUE : GL_BOOL_FALSE;
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.transparent(%d, %d)", color, transparent);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    GL_Context_t *context = &display->gl;
    GL_context_transparent(context, &index, &transparent, 1);

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

    GL_Context_t *context = &display->gl;
    GL_context_clipping(context, NULL);

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

    GL_Context_t *context = &display->gl; // TODO: pass context and palette directly?
    GL_context_clipping(context, &clipping_region);

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

    const GL_Context_t *context = &display->gl;
    GL_context_clear(context);

    return 0;
}

static int canvas_point(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    float x = (float)lua_tonumber(L, 1);
    float y = (float)lua_tonumber(L, 2);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.point(%f, %f, %d)", x, y, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    GL_primitive_point(context, (GL_Point_t){ .x = (int)x, .y = (int)y }, index);

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
    float x = (float)lua_tonumber(L, 1);
    float y = (float)lua_tonumber(L, 2);
    float width = (float)lua_tonumber(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.hline(%f, %f, %f, %d)", x, y, width, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    GL_primitive_hline(context, (GL_Point_t){ .x = (int)x, .y = (int)y }, (size_t)width, index);

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
    float x = (float)lua_tonumber(L, 1);
    float y = (float)lua_tonumber(L, 2);
    float height = (float)lua_tonumber(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.vline(%f, %f, %f, %d)", x, y, height, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    GL_primitive_vline(context, (GL_Point_t){ .x = (int)x, .y = (int)y }, (size_t)height, index);

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
    float x0 = (float)lua_tonumber(L, 1);
    float y0 = (float)lua_tonumber(L, 2);
    float x1 = (float)lua_tonumber(L, 3);
    float y1 = (float)lua_tonumber(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.line(%f, %f, %f, %f, %d)", x0, y0, x1, y1, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    GL_Point_t vertices[2] = {
            (GL_Point_t){ .x = (int)x0, .y = (int)y0 },
            (GL_Point_t){ .x = (int)x1, .y = (int)y1 }
        };
    GL_primitive_polyline(context, vertices, 2, index);

    return 0;
}

static int canvas_polyline(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 2);
#ifdef __DEBUG_API_CALLS__
    int type = lua_type(L, 1);
    Log_write(LOG_LEVELS_DEBUG, "Canvas.polyline(%d, %d)", type, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    size_t count = luaX_count(L, 1) / 2;

    if (count > 1) {
        GL_Point_t vertices[count];
        for (size_t i = 0; i < count; ++i) {
            int base = i * 2;
            lua_rawgeti(L, 1, base + 1);
            lua_rawgeti(L, 1, base + 2);
            vertices[i] = (GL_Point_t){ .x = (int)lua_tonumber(L, -2), .y = (int)lua_tonumber(L, -1) };
            lua_pop(L, 2);
        }

        const GL_Context_t *context = &display->gl;
        GL_primitive_polyline(context, vertices, count, index);
    } else {
        Log_write(LOG_LEVELS_WARNING, "<CANVAS> no enough points for polyline (%d)", count);
    }

    return 0;
}

static int canvas_fill(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    float x = (float)lua_tonumber(L, 1);
    float y = (float)lua_tonumber(L, 2);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.fill(%f, %f, %d)", x, y, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    GL_context_fill(context, (GL_Point_t){ .x = (int)x, .y = (int)y }, index);

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
    float x0 = (float)lua_tonumber(L, 2);
    float y0 = (float)lua_tonumber(L, 3);
    float x1 = (float)lua_tonumber(L, 4);
    float y1 = (float)lua_tonumber(L, 5);
    float x2 = (float)lua_tonumber(L, 6);
    float y2 = (float)lua_tonumber(L, 7);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 8);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.triangle(%s, %f, %f, %f, %f, %f, %f, %d)", mode, x0, y0, x1, y1, x2, y2, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    if (mode[0] == 'f') {
        GL_primitive_filled_triangle(context, (GL_Point_t){ .x = (int)x0, (int)y0 }, (GL_Point_t){ .x = (int)x1, .y = (int)y1 }, (GL_Point_t){ .x = (int)x2, .y = (int)y2 }, index);
    } else {
        GL_Point_t vertices[4] = {
                (GL_Point_t){ .x = (int)x0, .y = (int)y0 },
                (GL_Point_t){ .x = (int)x1, .y = (int)y1 },
                (GL_Point_t){ .x = (int)x2, .y = (int)y2 },
                (GL_Point_t){ .x = (int)x0, .y = (int)y0 }
            };
        GL_primitive_polyline(context, vertices, 4, index);
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
    float x = (float)lua_tonumber(L, 2);
    float y = (float)lua_tonumber(L, 3);
    float width = (float)lua_tonumber(L, 4);
    float height = (float)lua_tonumber(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.rectangle(%s, %f, %f, %f, %f, %d)", mode, x, y, width, height, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    if (mode[0] == 'f') {
        GL_primitive_filled_rectangle(context, (GL_Rectangle_t){ .x = (int)x, .y = (int)y, .width = (int)width, .height = (int)height }, index);
    } else {
        float x0 = x;
        float y0 = y;
        float x1 = x0 + width - 1.0f;
        float y1 = y0 + height - 1.0f;

        GL_Point_t vertices[5] = {
                (GL_Point_t){ .x = (int)x0, .y = (int)y0 },
                (GL_Point_t){ .x = (int)x0, .y = (int)y1 },
                (GL_Point_t){ .x = (int)x1, .y = (int)y1 },
                (GL_Point_t){ .x = (int)x1, .y = (int)y0 },
                (GL_Point_t){ .x = (int)x0, .y = (int)y0 }
            };
        GL_primitive_polyline(context, vertices, 5, index);
/*
        GL_primitive_hline(context, (GL_Point_t){ .x = (int)x0, .y = (int)y0 }, width, index);
        GL_primitive_vline(context, (GL_Point_t){ .x = (int)x0, .y = (int)y0 }, height, index);
        GL_primitive_hline(context, (GL_Point_t){ .x = (int)x0, .y = (int)y1 }, width, index);
        GL_primitive_vline(context, (GL_Point_t){ .x = (int)x1, .y = (int)y0 }, height, index);
*/
    }

    return 0;
}

static int canvas_circle(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    const char *mode = lua_tostring(L, 1);
    float cx = (float)lua_tonumber(L, 2);
    float cy = (float)lua_tonumber(L, 3);
    float radius = (float)lua_tonumber(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.circle(%s, %f, %f, %f, %f, %d)", mode, cx, cy, radius);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;

    if (radius < 1.0f) { // Null radius, just a point regardless mode!
        GL_primitive_point(context, (GL_Point_t){ .x = (int)cx, .y = (int)cy }, index);
    } else
    if (mode[0] == 'f') {
        GL_primitive_filled_circle(context, (GL_Point_t){ .x = (int)cx, .y = (int)cy }, (int)radius, index);
    } else {
        GL_primitive_circle(context, (GL_Point_t){ .x = (int)cx, .y = (int)cy }, (int)radius, index);
    }

    return 0;
}
