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

#include <config.h>
#include <core/environment.h>
#include <core/io/display.h>
#include <libs/log.h>
#include <libs/gl/gl.h>

#include "udt.h"
#include "graphics/palettes.h"
#include "graphics/sheets.h"

#include <math.h>
#include <string.h>
#include <time.h>
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif
#include <stb/stb_ds.h>

#define CANVAS_MT        "Tofu_Canvas_mt"

static int canvas_color_to_index(lua_State *L);
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
#ifdef __GL_MASK_SUPPORT__
static int canvas_mask(lua_State *L);
#endif
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
static int canvas_peek(lua_State *L);
static int canvas_poke(lua_State *L);

// TODO: color index is optional, if not present use the current (drawstate) pen color
// TODO: rename `Canvas` to `Context`?

static const struct luaL_Reg _canvas_functions[] = {
    { "color_to_index", canvas_color_to_index },
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
#ifdef __GL_MASK_SUPPORT__
    { "mask", canvas_mask },
#endif
    { "point", canvas_point },
    { "hline", canvas_hline },
    { "vline", canvas_vline },
    { "line", canvas_line },
    { "polyline", canvas_polyline },
    { "fill", canvas_fill },
    { "triangle", canvas_triangle },
    { "rectangle", canvas_rectangle },
    { "circle", canvas_circle },
    { "peek", canvas_peek },
    { "poke", canvas_poke },
    { NULL, NULL }
};

static const luaX_Const _canvas_constants[] = {
    { NULL }
};

static const uint8_t _canvas_lua[] = {
#include "canvas.inc"
};

static luaX_Script _canvas_script = { (const char *)_canvas_lua, sizeof(_canvas_lua), "canvas.lua" };

int canvas_loader(lua_State *L)
{
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, &_canvas_script, _canvas_functions, _canvas_constants, nup, CANVAS_MT);
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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));


    const GL_Color_t color = GL_palette_parse_color(argb);
    const GL_Pixel_t index = GL_palette_find_nearest_color(&display->palette, color);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "color '%s' mapped to index %d", argb, index);
#endif

    lua_pushinteger(L, index);

    return 1;
}

static int canvas_width(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.width()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Context_t *context = &display->gl;
    GL_context_surface(context, NULL);

    return 0;
}

static int canvas_surface1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Surface_Class_t *surface = (Surface_Class_t *)lua_touserdata(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.surface(%p)", type);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Palette_t *palette = &display->palette;

    lua_newtable(L);
    for (size_t i = 0; i < palette->count; ++i) {
        char argb[12] = { 0 };
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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Palette_t palette = { 0 };

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

        lua_pushnil(L);
        for (size_t i = 0; lua_next(L, 1); ++i) {
#if 0
            const char *key_type = lua_typename(L, lua_type(L, -2)); // uses 'key' (at index -2) and 'value' (at index -1)
#endif
            const char *argb = lua_tostring(L, -1);
            palette.colors[i] = GL_palette_parse_color(argb);

            lua_pop(L, 1);
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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    size_t *from = NULL;
    size_t *to = NULL;
    size_t count = 0;

    lua_pushnil(L);
    while (lua_next(L, 1)) {
        arrpush(from, lua_tointeger(L, -2));
        arrpush(to, lua_tointeger(L, -1));
        ++count;

        lua_pop(L, 1);
    }

    GL_Context_t *context = &display->gl;
    GL_context_shifting(context, from, to, count);

    arrfree(from);
    arrfree(to);

    return 0;
}

static int canvas_shift2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    size_t from = (size_t)lua_tointeger(L, 1);
    size_t to = (size_t)lua_tointeger(L, 2);
#ifdef __DEBUG_API_CALLS__
    int type = lua_type(L, 1);
    Log_write(LOG_LEVELS_DEBUG, "Canvas.shift(%d, %d)", from, to);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Pixel_t *indexes = NULL;
    GL_Bool_t *transparent = NULL;
    size_t count = 0;

    lua_pushnil(L);
    while (lua_next(L, 1)) {
        arrpush(indexes, (GL_Pixel_t)lua_tointeger(L, -2));
        arrpush(transparent, lua_toboolean(L, -1) ? GL_BOOL_TRUE : GL_BOOL_FALSE);
        ++count;

        lua_pop(L, 1);
    }

    GL_Context_t *context = &display->gl;
    GL_context_transparent(context, indexes, transparent, count);

    arrfree(indexes);
    arrfree(transparent);

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Context_t *context = &display->gl;
    GL_context_clipping(context, NULL);

    return 0;
}

static int canvas_clipping4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    int x0 = lua_tointeger(L, 1);
    int y0 = lua_tointeger(L, 2);
    int x1 = lua_tointeger(L, 3);
    int y1 = lua_tointeger(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.clipping(%d, %d, %d, %d)", x0, y0, x1, y1);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Context_t *context = &display->gl; // TODO: pass context and palette directly?
    GL_context_clipping(context, &(GL_Quad_t){ .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1 });

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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_shader(display, code);

    return 0;
}

#ifdef __GL_MASK_SUPPORT__
static int canvas_mask0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Context_t *context = &display->gl;
    GL_context_mask(context, NULL);

    return 0;
}

static int canvas_mask1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata, luaX_isinteger)
    LUAX_SIGNATURE_END
    int type = lua_type(L, 1);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Context_t *context = &display->gl;
    GL_Mask_t mask = context->state.mask;
    if (type == LUA_TUSERDATA) {
        const Surface_Class_t *instance = (const Surface_Class_t *)lua_touserdata(L, 1);

        mask.stencil = &instance->surface;
    } else
    if (type == LUA_TNUMBER) {
        GL_Pixel_t index = lua_tointeger(L, 1);

        mask.threshold = index;
    }
    GL_context_mask(context, &mask);

    return 0;
}

static int canvas_mask2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    const Surface_Class_t *instance = (const Surface_Class_t *)lua_touserdata(L, 1);
    GL_Pixel_t index = lua_tointeger(L, 2);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Context_t *context = &display->gl;
    GL_context_mask(context, &(GL_Mask_t){ &instance->surface, index });

    return 0;
}

static int canvas_mask(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_mask0)
        LUAX_OVERLOAD_ARITY(1, canvas_mask1)
        LUAX_OVERLOAD_ARITY(2, canvas_mask2)
    LUAX_OVERLOAD_END
}
#endif

static int canvas_clear(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.clear()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

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
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.point(%d, %d, %d)", x, y, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    GL_primitive_point(context, (GL_Point_t){ .x = x, .y = y }, index);

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
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);
    int width = lua_tointeger(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.hline(%d, %d, %d, %d)", x, y, width, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    GL_primitive_hline(context, (GL_Point_t){ .x = x, .y = y }, (size_t)width, index);

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
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);
    size_t height = (size_t)lua_tointeger(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.vline(%d, %d, %d, %d)", x, y, height, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    GL_primitive_vline(context, (GL_Point_t){ .x = x, .y = y }, height, index);

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
    int x0 = lua_tointeger(L, 1);
    int y0 = lua_tointeger(L, 2);
    int x1 = lua_tointeger(L, 3);
    int y1 = lua_tointeger(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.line(%f, %f, %f, %f, %d)", x0, y0, x1, y1, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    GL_Point_t vertices[2] = {
            (GL_Point_t){ .x = x0, .y = y0 },
            (GL_Point_t){ .x = x1, .y = y1 }
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

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    index %= display->palette.count;

    GL_Point_t *vertices = NULL;
    size_t count = 0;
    int aux = 0;

    lua_pushnil(L);
    while (lua_next(L, 1)) {
        int value = lua_tointeger(L, -1);
        ++count;
        if (count > 0 && (count % 2) == 0) {
            GL_Point_t point = (GL_Point_t){ .x = aux, .y = value }; // Can't pass compound-literal to macro. :(
            arrpush(vertices, point);
        } else {
            aux = value;
        }
        lua_pop(L, 1);
    }

    if (count > 1) {
        const GL_Context_t *context = &display->gl;
        GL_primitive_polyline(context, vertices, count / 2, index);
    } else {
        Log_write(LOG_LEVELS_WARNING, "<CANVAS> no enough points for polyline (%d)", count);
    }

    arrfree(vertices);

    return 0;
}

static int canvas_fill(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.fill(%d, %d, %d)", x, y, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    GL_context_fill(context, (GL_Point_t){ .x = x, .y = y }, index);

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
    int x0 = lua_tointeger(L, 2);
    int y0 = lua_tointeger(L, 3);
    int x1 = lua_tointeger(L, 4);
    int y1 = lua_tointeger(L, 5);
    int x2 = lua_tointeger(L, 6);
    int y2 = lua_tointeger(L, 7);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 8);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.triangle(%s, %d, %d, %d, %d, %d, %d, %d)", mode, x0, y0, x1, y1, x2, y2, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    if (mode[0] == 'f') {
        GL_primitive_filled_triangle(context, (GL_Point_t){ .x = x0, y0 }, (GL_Point_t){ .x = x1, .y = y1 }, (GL_Point_t){ .x = x2, .y = y2 }, index);
    } else {
        GL_Point_t vertices[4] = {
                (GL_Point_t){ .x = x0, .y = y0 },
                (GL_Point_t){ .x = x1, .y = y1 },
                (GL_Point_t){ .x = x2, .y = y2 },
                (GL_Point_t){ .x = x0, .y = y0 }
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
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    int width = lua_tointeger(L, 4);
    int height = lua_tointeger(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.rectangle(%s, %d, %d, %d, %d, %d)", mode, x, y, width, height, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    if (mode[0] == 'f') {
        // TODO: move to pointers for compound literals, too.
        GL_primitive_filled_rectangle(context, (GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height }, index);
    } else {
        int x0 = x;
        int y0 = y;
        int x1 = x0 + width - 1;
        int y1 = y0 + height - 1;

        GL_Point_t vertices[5] = {
                (GL_Point_t){ .x = x0, .y = y0 },
                (GL_Point_t){ .x = x0, .y = y1 },
                (GL_Point_t){ .x = x1, .y = y1 },
                (GL_Point_t){ .x = x1, .y = y0 },
                (GL_Point_t){ .x = x0, .y = y0 }
            };
        GL_primitive_polyline(context, vertices, 5, index);
/*
        GL_primitive_hline(context, (GL_Point_t){ .x = x0, .y = y0 }, width, index);
        GL_primitive_vline(context, (GL_Point_t){ .x = x0, .y = y0 }, height, index);
        GL_primitive_hline(context, (GL_Point_t){ .x = x0, .y = y1 }, width, index);
        GL_primitive_vline(context, (GL_Point_t){ .x = x1, .y = y0 }, height, index);
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
    int cx = lua_tointeger(L, 2);
    int cy = lua_tointeger(L, 3);
    int radius = lua_tointeger(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.circle(%s, %f, %f, %f, %f, %d)", mode, cx, cy, radius);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;

    if (radius < 1.0f) { // Null radius, just a point regardless mode!
        GL_primitive_point(context, (GL_Point_t){ .x = cx, .y = cy }, index);
    } else
    if (mode[0] == 'f') {
        GL_primitive_filled_circle(context, (GL_Point_t){ .x = cx, .y = cy }, radius, index);
    } else {
        GL_primitive_circle(context, (GL_Point_t){ .x = cx, .y = cy }, radius, index);
    }

    return 0;
}

static int canvas_peek(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.peek(%d, %d)", x, y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = context->state.surface;
    GL_Pixel_t index = surface->data_rows[y % surface->height][x % surface->width];

    lua_pushinteger(L, index);

    return 1;
}

static int canvas_poke(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.poke(%d, %d, %d)", x, y, index);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    index %= display->palette.count;

    const GL_Context_t *context = &display->gl;
    GL_Surface_t *surface = context->state.surface;
    surface->data_rows[y % surface->height][x % surface->width] = index;

    return 0;
}