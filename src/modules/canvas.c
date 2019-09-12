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

typedef struct _Canvas_Class_t {
} Canvas_Class_t;

static int canvas_width(lua_State *L);
static int canvas_height(lua_State *L);
static int canvas_palette(lua_State *L);
static int canvas_background(lua_State *L);
static int canvas_shift(lua_State *L);
static int canvas_transparent(lua_State *L);
static int canvas_shader(lua_State *L);
static int canvas_color(lua_State *L);
static int canvas_clear(lua_State *L);
static int canvas_screenshot(lua_State *L);
static int canvas_points(lua_State *L);
static int canvas_cluster(lua_State *L);
static int canvas_polyline(lua_State *L);
static int canvas_strip(lua_State *L);
static int canvas_fan(lua_State *L);

static const char _canvas_script[] =
    "local Canvas = {}\n"
    "\n"
    "function Canvas.point(x0, y0, color)\n"
    "  Canvas.points({ x0, y0 }, color)\n"
    "end\n"
    "\n"
    "function Canvas.line(x0, y0, x1, y1, color)\n"
    "  Canvas.polyline({ x0, y0, x1, y1, x0, y0 }, color)\n"
    "end\n"
    "\n"
    "function Canvas.triangle(mode, x0, y0, x1, y1, x2, y2, color)\n"
    "  if mode == \"line\" then\n"
    "    Canvas.polyline({ x0, y0, x1, y1, x2, y2, x0, y0 }, color)\n"
    "  else\n"
    "    Canvas.strip({ x0, y0, x1, y1, x2, y2 }, color)\n"
    "  end\n"
    "end\n"
    "\n"
    "function Canvas.rectangle(mode, x, y, width, height, color)\n"
    "  local offset = mode == \"line\" and 1 or 0\n"
    "  local x0 = x\n"
    "  local y0 = y\n"
    "  local x1 = x0 + width - offset\n"
    "  local y1= y0 + height - offset\n"
    "  if mode == \"line\" then\n"
    "    Canvas.polyline({ x0, y0, x0, y1, x1, y1, x1, y0, x0, y0 }, color)\n"
    "  else\n"
    "    Canvas.strip({ x0, y0, x0, y1, x1, y0, x1, y1 }, color)\n"
    "  end\n"
    "end\n"
    "\n"
    "function Canvas.square(mode, x, y, size, color)\n"
    "  Canvas.rectangle(mode, x, y, size, size, color)\n"
    "end\n"
    "\n"
    "function Canvas.circle(mode, cx, cy, radius, color, segments)\n"
    "  local radius_squared = radius * radius\n"
    "  local points = {}\n"
    "  if mode == \"line\" then\n"
    "    local steps = radius * math.cos(math.pi / 4.0);\n"
    "    for x = 0, steps do\n"
    "      local y = math.sqrt(radius_squared - (x * x))\n"
    "\n"
    "      table.insert(points, cx + x)\n"
    "      table.insert(points, cy + y)\n"
    "      table.insert(points, cx + x)\n"
    "      table.insert(points, cy - y)\n"
    "      table.insert(points, cx - x)\n"
    "      table.insert(points, cy + y)\n"
    "      table.insert(points, cx - x)\n"
    "      table.insert(points, cy - y)\n"
    "\n"
    "      table.insert(points, cx + y)\n"
    "      table.insert(points, cy + x)\n"
    "      table.insert(points, cx + y)\n"
    "      table.insert(points, cy - x)\n"
    "      table.insert(points, cx - y)\n"
    "      table.insert(points, cy + x)\n"
    "      table.insert(points, cx - y)\n"
    "      table.insert(points, cy - x)\n"
    "    end\n"
    "  else\n"
    "    for y = -radius, radius do\n"
    "      local y_squared = y * y\n"
    "      for x = -radius, radius do\n"
    "        local x_squared = x * x\n"
    "        if (x_squared + y_squared) <= radius_squared then\n"
    "          table.insert(points, cx + x)\n"
    "          table.insert(points, cy + y)\n"
    "        end\n"
    "      end\n"
    "    end\n"
    "  end\n"
    "  Canvas.points(points, color)\n"
    "end\n"
    "\n"
    "return Canvas\n"
;

static const struct luaL_Reg _canvas_functions[] = {
    { "width", canvas_width },
    { "height", canvas_height },
    { "palette", canvas_palette },
    { "background", canvas_background },
    { "shift", canvas_shift },
    { "transparent", canvas_transparent },
    { "shader", canvas_shader },
    { "color", canvas_color },
    { "clear", canvas_clear },
    { "screenshot", canvas_screenshot },
    { "points", canvas_points },
    { "cluster", canvas_cluster },
    { "polyline", canvas_polyline },
    { "strip", canvas_strip },
    { "fan", canvas_fan },
    { NULL, NULL }
};

static const luaX_Const _canvas_constants[] = {
    { NULL }
};

int canvas_loader(lua_State *L)
{
    luaX_pushupvalues(L, 2); // Duplicate the upvalues to pass it to the module.
    return luaX_newmodule(L, _canvas_script, _canvas_functions, _canvas_constants, 2, LUAX_CLASS(Canvas_Class_t));
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

    GL_Palette_t *palette = &display->gl.palette;

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

    GL_context_palette(&display->gl, &palette);

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
    int color = lua_tointeger(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.background(%d)", color);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_context_background(&display->gl, color);

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

static int canvas_color(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
    LUAX_SIGNATURE_END
    const char *argb = lua_tostring(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.color('%s')", argb);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Color_t color = GL_palette_parse_color(argb);
    const size_t index = GL_palette_find_nearest_color(&display->gl.palette, color);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "color '%s' mapped to index %d", argb, index);
#endif

    lua_pushinteger(L, index);

    return 1;
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

static int canvas_screenshot(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
    LUAX_SIGNATURE_END
    const char *file = lua_tostring(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.screenshot('%s')", file);
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));
    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    // TODO: auto numbering?

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file);

    GL_context_screenshot(&display->gl, pathfile);

    return 0;
}

static int canvas_points(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int vertices = lua_rawlen(L, 1);
    int color = lua_tointeger(L, 2);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.points(%d, %d)", vertices, color);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<CANVAS> point-sequence as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        GL_primitive_point(&display->gl, (GL_Point_t){ (int)x, (int)y }, color);
    }

    return 0;
}

static int canvas_cluster(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable)
    LUAX_SIGNATURE_END
    int amount = lua_rawlen(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.cluster(%d)", amount);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const size_t count = amount / 3;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<CANVAS> cluster-sequence as no elements");
        return 0;
    }

    double array[amount];
    luaX_getnumberarray(L, 1, array);

    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 3)];
        double y = array[(i * 3) + 1];
        double c = array[(i * 3) + 2];

        GL_primitive_point(&display->gl, (GL_Point_t){ (int)x, (int)y }, c);
    }

    return 0;
}

static int canvas_polyline(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int vertices = lua_rawlen(L, 1);
    int color = lua_tointeger(L, 2);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.polyline(%d, %d)", vertices, color);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<CANVAS> polyline as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        GL_primitive_point(&display->gl, (GL_Point_t){ (int)x, (int)y }, color);
    }

    return 0;
}

static int canvas_strip(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int vertices = lua_rawlen(L, 1);
    int color = lua_tointeger(L, 2);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.strip(%d, %d)", vertices, color);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<CANVAS> strip as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        GL_primitive_point(&display->gl, (GL_Point_t){ (int)x, (int)y }, color);
    }

    return 0;
}

static int canvas_fan(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable)
        LUAX_SIGNATURE_ARGUMENT(luaX_isinteger)
    LUAX_SIGNATURE_END
    int vertices = lua_rawlen(L, 1);
    int color = lua_tointeger(L, 2);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.fan(%d, %d)", vertices, color);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<CANVAS> fan as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        GL_primitive_point(&display->gl, (GL_Point_t){ (int)x, (int)y }, color);
    }

    return 0;
}
