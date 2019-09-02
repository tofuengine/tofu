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

#define OPENGL_PIXEL_OFFSET     0.375f

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
static int canvas_points(lua_State *L);
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
    "  segments = segments or 128\n"
    "  local step = (2 * math.pi) / segments\n"
    "  if mode == \"line\" then\n"
    "    local angle = 0\n"
    "    local vertices = {}\n"
    "    for i = 0, segments do\n"
    "      angle = angle + step\n"
    "      y = math.sin(angle) * radius\n"
    "      x = math.cos(angle) * radius\n"
    "      table.insert(vertices, cx + x)\n"
    "      table.insert(vertices, cy + y)\n"
    "    end\n"
    "    Canvas.polyline(vertices, color)\n"
    "  else\n"
    "    local angle = 0\n"
    "    local vertices = {}\n"
    "    table.insert(vertices, cx)\n"
    "    table.insert(vertices, cy)\n"
    "    for i = 0, segments do\n"
    "      angle = angle + step\n"
    "      y = math.sin(angle) * radius\n"
    "      x = math.cos(angle) * radius\n"
    "      table.insert(vertices, cx + x)\n"
    "      table.insert(vertices, cy + y)\n"
    "    end\n"
    "    Canvas.fan(vertices, color)\n"
    "  end\n"
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
    { "points", canvas_points },
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
    lua_pushvalue(L, lua_upvalueindex(1)); // Duplicate the upvalue to pass it to the module.
    return luaX_newmodule(L, _canvas_script, _canvas_functions, _canvas_constants, 1, LUAX_CLASS(Canvas_Class_t));
}

static int canvas_width(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.width()");
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    lua_pushinteger(L, environment->display->configuration.width);

    return 1;
}

static int canvas_height(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.height()");
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    lua_pushinteger(L, environment->display->configuration.height);

    return 1;
}

static int canvas_palette0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.palette()");
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    GL_Palette_t *palette = &environment->display->palette;

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

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

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
            palette.count = MAX_PALETTE_COLORS;
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

    Display_palette(environment->display, &palette);

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

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    Display_background(environment->display, color);

    return 0;
}

static int canvas_shift0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.shift()");
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    Display_shift(environment->display, NULL, NULL, 0);

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

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    int count = luaX_count(L, 1);

    size_t from[count];
    size_t to[count];
    lua_pushnil(L); // first key
    for (size_t i = 0; lua_next(L, 1); ++i) {
        from[i] = lua_tointeger(L, -2);
        to[i] = lua_tointeger(L, -1);

        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }

    Display_shift(environment->display, from, to, count);

    return 0;
}

static int canvas_shift(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_shift0)
        LUAX_OVERLOAD_ARITY(1, canvas_shift1)
    LUAX_OVERLOAD_END
}

static int canvas_transparent0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.transparent()");
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    Display_transparent(environment->display, NULL, NULL, 0);

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

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    int count = luaX_count(L, 1);

    size_t color[count];
    bool is_transparent[count];
    lua_pushnil(L); // first key
    for (size_t i = 0; lua_next(L, 1); ++i) {
        color[i] = lua_tointeger(L, -2);
        is_transparent[i] = lua_toboolean(L, -1) ? true : false;

        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }

    Display_transparent(environment->display, color, is_transparent, count);

    return 0;
}

static int canvas_transparent(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_transparent0)
        LUAX_OVERLOAD_ARITY(1, canvas_transparent1)
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

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    Display_shader(environment->display, code);

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

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));

    GL_Color_t color = GL_palette_parse_color(argb);
    size_t index = GL_palette_find_nearest_color(&environment->display->palette, color);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "color '%s' mapped to index %d", argb, index);
#endif

    lua_pushinteger(L, index);

    return 1;
}

// When drawing points and lines we need to ensure to be in mid-pixel coordinates, according to
// the "diamond exit rule" in OpenGL's rasterization. Also, the ending pixel of a line segment
// is not drawn to avoid lighting a pixel twice in a loop.
//
// http://glprogramming.com/red/appendixg.html#name1
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

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<CANVAS> point-sequence as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        points[i] = (GL_Point_t){
                .x = (GLfloat)x + OPENGL_PIXEL_OFFSET, .y = (GLfloat)y + OPENGL_PIXEL_OFFSET
            };
    }

    GL_primitive_points(points, count, (GL_Color_t){ color, color, color, 255 });

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

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<CANVAS> polyline as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        points[i] = (GL_Point_t){
                .x = (GLfloat)x + OPENGL_PIXEL_OFFSET, .y = (GLfloat)y + OPENGL_PIXEL_OFFSET
            };
    }

    GL_primitive_polyline(points, count, (GL_Color_t){ color, color, color, 255 });

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

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<CANVAS> strip as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        points[i] = (GL_Point_t){
                .x = (GLfloat)x + OPENGL_PIXEL_OFFSET, .y = (GLfloat)y + OPENGL_PIXEL_OFFSET
            };
    }

    GL_primitive_strip(points, count, (GL_Color_t){ color, color, color, 255 });

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

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<CANVAS> fan as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        points[i] = (GL_Point_t){
                .x = (GLfloat)x + OPENGL_PIXEL_OFFSET, .y = (GLfloat)y + OPENGL_PIXEL_OFFSET
            };
    }

    GL_primitive_fan(points, count, (GL_Color_t){ color, color, color, 255 });

    return 0;
}
