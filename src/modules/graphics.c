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

#include "graphics.h"

#include "../config.h"
#include "../environment.h"
#include "../log.h"
#include "../gl/gl.h"
#include "graphics/palettes.h"
#include "graphics/sheets.h"

#include <math.h>
#include <string.h>

typedef struct _Bank_Class_t {
    // char pathfile[PATH_FILE_MAX];
    GL_Sheet_t sheet;
} Bank_Class_t;

typedef struct _Canvas_Class_t {
} Canvas_Class_t;

typedef struct _Font_Class_t {
    // char pathfile[PATH_FILE_MAX];
    GL_Sheet_t sheet;
} Font_Class_t;

static const char *graphics_lua =
    "local graphics = require(\"tofu.graphics\")\n"
    "\n"
    "graphics.Font.default = function()\n"
    "  return graphics.Font.new(\"5x8\", 0, 0)\n"
    "end\n"
    "\n"
    "graphics.Canvas.point = function(x0, y0, color)\n"
    "  graphics.Canvas.points({ x0, y0 }, color)\n"
    "end\n"
    "\n"
    "graphics.Canvas.line = function(x0, y0, x1, y1, color)\n"
    "  graphics.Canvas.polyline({ x0, y0, x1, y1, x0, y0 }, color)\n"
    "end\n"
    "\n"
    "graphics.Canvas.triangle = function(mode, x0, y0, x1, y1, x2, y2, color)\n"
    "  if mode == \"line\" then\n"
    "    graphics.Canvas.polyline({ x0, y0, x1, y1, x2, y2, x0, y0 }, color)\n"
    "  else\n"
    "    graphics.Canvas.strip({ x0, y0, x1, y1, x2, y2 }, color)\n"
    "  end\n"
    "end\n"
    "\n"
    "graphics.Canvas.rectangle = function(mode, x, y, width, height, color)\n"
    "  local offset = mode == \"line\" and 1 or 0\n"
    "  local x0 = x\n"
    "  local y0 = y\n"
    "  local x1 = x0 + width - offset\n"
    "  local y1= y0 + height - offset\n"
    "  if mode == \"line\" then\n"
    "    graphics.Canvas.polyline({ x0, y0, x0, y1, x1, y1, x1, y0, x0, y0 }, color)\n"
    "  else\n"
    "    graphics.Canvas.strip({ x0, y0, x0, y1, x1, y0, x1, y1 }, color)\n"
    "  end\n"
    "end\n"
    "\n"
    "graphics.Canvas.square = function(mode, x, y, size, color)\n"
    "  graphics.Canvas.rectangle(mode, x, y, size, size, color)\n"
    "end\n"
    "\n"
    "graphics.Canvas.circle = function(mode, x, y, radius, color, segments)\n"
    "  segments = segments or 30\n"
    "  local step = (2 * math.PI) / segments\n"
    "  if mode == \"line\" then\n"
    "    local vertices = {}\n"
    "    for i = 1, segments do\n"
    "      local angle = step * i\n"
    "      table.insert(vertices, x + math.sin(angle) * radius)\n"
    "      table.insert(vertices, y + math.cos(angle) * radius)\n"
    "    end\n"
    "    graphics.Canvas.polyline(vertices, color)\n"
    "  else\n"
    "    local vertices = {}\n"
    "    vertices.insert(-1, x)\n"
    "    vertices.insert(-1, y)\n"
    "    for i = 1, segments do\n"
    "      local angle = step * i\n"
    "      table.insert(vertices, x + math.sin(angle) * radius)\n"
    "      table.insert(vertices, y + math.cos(angle) * radius)\n"
    "    end\n"
    "    graphics.Canvas.fan(vertices, color)\n"
    "  end\n"
    "end\n"
;

static int graphics_bank_new(lua_State *L);
static int graphics_bank_gc(lua_State *L);
static int graphics_bank_cell_width(lua_State *L);
static int graphics_bank_cell_height(lua_State *L);
static int graphics_bank_blit(lua_State *L);

static const struct luaL_Reg graphics_bank_f[] = {
    { "new", graphics_bank_new },
    { NULL, NULL }
};

static const struct luaL_Reg graphics_bank_m[] = {
    {"__gc", graphics_bank_gc },
    { "cell_width", graphics_bank_cell_width },
    { "cell_height", graphics_bank_cell_height },
    { "blit", graphics_bank_blit },
    { NULL, NULL }
};

static const luaX_Const graphics_bank_c[] = {
    { NULL }
};

static int graphics_canvas_width(lua_State *L);
static int graphics_canvas_height(lua_State *L);
static int graphics_canvas_palette(lua_State *L);
static int graphics_canvas_background(lua_State *L);
static int graphics_canvas_shader(lua_State *L);
static int graphics_canvas_color(lua_State *L);
static int graphics_canvas_points(lua_State *L);
static int graphics_canvas_polyline(lua_State *L);
static int graphics_canvas_strip(lua_State *L);
static int graphics_canvas_fan(lua_State *L);

static const struct luaL_Reg graphics_canvas_f[] = {
    { "width", graphics_canvas_width },
    { "height", graphics_canvas_height },
    { "palette", graphics_canvas_palette },
    { "background", graphics_canvas_background },
    { "shader", graphics_canvas_shader },
    { "color", graphics_canvas_color },
    { "points", graphics_canvas_points },
    { "polyline", graphics_canvas_polyline },
    { "strip", graphics_canvas_strip },
    { "fan", graphics_canvas_fan },
    { NULL, NULL }
};

static const struct luaL_Reg graphics_canvas_m[] = {
    { NULL, NULL }
};

static const luaX_Const graphics_canvas_c[] = {
    { NULL }
};

static int graphics_font_new(lua_State *L);
static int graphics_font_gc(lua_State *L);
static int graphics_font_write(lua_State *L);

static const struct luaL_Reg graphics_font_f[] = {
    { "new", graphics_font_new },
    { NULL, NULL }
};

static const struct luaL_Reg graphics_font_m[] = {
    {"__gc", graphics_font_gc },
    { "write", graphics_font_write },
    { NULL, NULL }
};

static const luaX_Const graphics_font_c[] = {
    { NULL }
};

static int luaopen_graphics(lua_State *L)
{
    lua_newtable(L);

    luaX_newclass(L, graphics_bank_f, graphics_bank_m, graphics_bank_c, LUAX_CLASS(Bank_Class_t));
    lua_setfield(L, -2, "Bank");

    luaX_newclass(L, graphics_canvas_f, graphics_canvas_m, graphics_canvas_c, LUAX_CLASS(Canvas_Class_t));
    lua_setfield(L, -2, "Canvas");

    luaX_newclass(L, graphics_font_f, graphics_font_m, graphics_font_c, LUAX_CLASS(Font_Class_t));
    lua_setfield(L, -2, "Font");

    return 1;
}

bool graphics_initialize(lua_State *L)
{
    luaX_preload(L, "tofu.graphics", luaopen_graphics);

    if (luaL_dostring(L, graphics_lua) != 0) {
        Log_write(LOG_LEVELS_FATAL, "<GRAPHICS> can't open script: %s", lua_tostring(L, -1));
        return false;
    }

    return true;
}

#define RED_WEIGHT      2.0f
#define GREEN_WEIGHT    4.0f
#define BLUE_WEIGHT     3.0f

// https://en.wikipedia.org/wiki/Color_difference
static size_t find_nearest_color(const GL_Palette_t *palette, const GL_Color_t color)
{
    size_t index = 0;
    double minimum = __DBL_MAX__;
    for (size_t i = 0; i < palette->count; ++i) {
        const GL_Color_t *current = &palette->colors[i];

        double delta_r = (double)(color.r - current->r);
        double delta_g = (double)(color.g - current->g);
        double delta_b = (double)(color.b - current->b);
#ifdef __FIND_NEAREST_COLOR_EUCLIDIAN__
        double distance = sqrt((delta_r * delta_r) * RED_WEIGHT
            + (delta_g * delta_g) * GREEN_WEIGHT
            + (delta_b * delta_b)) * BLUE_WEIGHT;
#else
        double distance = (delta_r * delta_r) * RED_WEIGHT
            + (delta_g * delta_g) * GREEN_WEIGHT
            + (delta_b * delta_b) * BLUE_WEIGHT; // Faster, no need to get the Euclidean distance.
#endif
        if (minimum > distance) {
            minimum = distance;
            index = i;
        }
    }
    return index;
}

static void to_indexed_atlas_callback(void *parameters, void *data, int width, int height) // TODO: convert image with a shader.
{
    const GL_Palette_t *palette = (const GL_Palette_t *)parameters;

    GL_Color_t *pixels = (GL_Color_t *)data;

    for (int y = 0; y < height; ++y) {
        int row_offset = width * y;
        for (int x = 0; x < width; ++x) {
            int offset = row_offset + x;

            GL_Color_t color = pixels[offset];
            if (color.a == 0) { // Skip transparent colors.
                continue;
            }

            size_t index = find_nearest_color(palette, color);
            pixels[offset] = (GL_Color_t){ index, index, index, color.a };
        }
    }
}

static void to_font_atlas_callback(void *parameters, void *data, int width, int height)
{
    GL_Color_t *pixels = (GL_Color_t *)data;

    for (int y = 0; y < height; ++y) {
        int row_offset = width * y;
        for (int x = 0; x < width; ++x) {
            int offset = row_offset + x;

            GL_Color_t color = pixels[offset];
            GLubyte index = color.a == 0 ? 0 : 255;
            pixels[offset] = (GL_Color_t){ index, index, index, color.a };
        }
    }
}

static int graphics_bank_new(lua_State *L)
{
    if (lua_gettop(L) != 3) {
        return luaL_error(L, "<GRAPHICS> bank constructor requires 3 arguments");
    }
    const char *file = luaL_checkstring(L, 1);
    int cell_width = luaL_checkinteger(L, 2);
    int cell_height = luaL_checkinteger(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.new() -> %s, %d, %d", file, cell_width, cell_height);
#endif
    Bank_Class_t *instance = (Bank_Class_t *)lua_newuserdata(L, sizeof(Bank_Class_t));

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file + 2);

    GL_Sheet_t sheet;
    GL_sheet_load(&sheet, pathfile, cell_width, cell_height, to_indexed_atlas_callback, (void *)&environment->display->palette);
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> bank '%s' allocated as #%p", pathfile, instance);

    *instance = (Bank_Class_t){
            .sheet = sheet
        };

    luaL_setmetatable(L, LUAX_CLASS(Bank_Class_t));

    return 1;
}

static int graphics_bank_gc(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<GRAPHICS> method requires 1 arguments");
    }
    Bank_Class_t *instance = (Bank_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Bank_Class_t));

    GL_sheet_delete(&instance->sheet);
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> bank #%p finalized", instance);

    *instance = (Bank_Class_t){};

    return 0;
}

static int graphics_bank_cell_width(lua_State *L)
{
    if (lua_gettop(L) != 0) {
        return luaL_error(L, "<GRAPHICS> method requires 0 arguments");
    }
    Bank_Class_t *instance = (Bank_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Bank_Class_t));

    lua_pushinteger(L, instance->sheet.quad.width);

    return 1;
}

static int graphics_bank_cell_height(lua_State *L)
{
    if (lua_gettop(L) != 0) {
        return luaL_error(L, "<GRAPHICS> method requires 0 arguments");
    }
    Bank_Class_t *instance = (Bank_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Bank_Class_t));

    lua_pushinteger(L, instance->sheet.quad.height);

    return 1;
}

static int graphics_bank_blit4(lua_State *L)
{
    if (lua_gettop(L) != 4) {
        return luaL_error(L, "<GRAPHICS> method requires 4 arguments");
    }
    Bank_Class_t *instance = (Bank_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Bank_Class_t));
    int cell_id = luaL_checkinteger(L, 2);
    double x = (double)luaL_checknumber(L, 3);
    double y = (double)luaL_checknumber(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f", cell_id, x, y);
#endif

    double dw = (double)instance->sheet.quad.width;
    double dh = (double)instance->sheet.quad.height;

    int dx = (int)x;
    int dy = (int)y;

    GL_Quad_t destination = (GL_Quad_t){ (GLfloat)dx, (GLfloat)dy, (GLfloat)dx + (GLfloat)dw, (GLfloat)dy + (GLfloat)dh };

    const GL_Sheet_t *sheet = &instance->sheet;
    GL_sheet_blit_fast(sheet, cell_id, destination, (GL_Color_t){ 255, 255, 255, 255 });

    return 0;
}

static int graphics_bank_blit5(lua_State *L)
{
    if (lua_gettop(L) != 5) {
        return luaL_error(L, "<GRAPHICS> method requires 5 arguments");
    }
    Bank_Class_t *instance = (Bank_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Bank_Class_t));
    int cell_id = luaL_checkinteger(L, 2);
    double x = (double)luaL_checknumber(L, 3);
    double y = (double)luaL_checknumber(L, 4);
    double rotation = (double)luaL_checknumber(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f, %.f", cell_id, x, y, rotation);
#endif

    double dw = (double)instance->sheet.quad.width;
    double dh = (double)instance->sheet.quad.height;

    int dx = (int)x;
    int dy = (int)y;

    GL_Quad_t destination = (GL_Quad_t){ (GLfloat)dx, (GLfloat)dy, (GLfloat)dx + (GLfloat)dw, (GLfloat)dy + (GLfloat)dh };

    const GL_Sheet_t *sheet = &instance->sheet;
    GL_sheet_blit(sheet, cell_id, destination, rotation, (GL_Color_t){ 255, 255, 255, 255 });

    return 0;
}

static int graphics_bank_blit6(lua_State *L)
{
    if (lua_gettop(L) != 6) {
        return luaL_error(L, "<GRAPHICS> method requires 6 arguments");
    }
    Bank_Class_t *instance = (Bank_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Bank_Class_t));
    int cell_id = luaL_checkinteger(L, 2);
    double x = (double)luaL_checknumber(L, 3);
    double y = (double)luaL_checknumber(L, 4);
    double scale_x = (double)luaL_checknumber(L, 5);
    double scale_y = (double)luaL_checknumber(L, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f, %.f, %.f", cell_id, x, y, scale_x, scale_y);
#endif

#ifdef __NO_MIRRORING__
    double dw = (double)instance->sheet.quad.width * fabs(scale_x);
    double dh = (double)instance->sheet.quad.height * fabs(scale_y);
#else
    double dw = (double)instance->sheet.quad.width * scale_x; // The sign controls the mirroring.
    double dh = (double)instance->sheet.quad.height * scale_y;
#endif

    int dx = (int)x;
    int dy = (int)y;

    GL_Quad_t destination = (GL_Quad_t){ (GLfloat)dx, (GLfloat)dy, (GLfloat)dx + (GLfloat)dw, (GLfloat)dy + (GLfloat)dh };

#ifndef __NO_MIRRORING__
    if (dw < 0.0) { // Compensate for mirroring!
        destination.x0 -= dw;
        destination.x1 -= dw;
    }
    if (dh < 0.0) {
        destination.y0 -= dh;
        destination.y1 -= dh;
    }
#endif

    const GL_Sheet_t *sheet = &instance->sheet;
    GL_sheet_blit_fast(sheet, cell_id, destination, (GL_Color_t){ 255, 255, 255, 255 });

    return 0;
}

static int graphics_bank_blit7(lua_State *L)
{
    if (lua_gettop(L) != 7) {
        return luaL_error(L, "<GRAPHICS> method requires 7 arguments");
    }
    Bank_Class_t *instance = (Bank_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Bank_Class_t));
    int cell_id = luaL_checkinteger(L, 2);
    double x = (double)luaL_checknumber(L, 3);
    double y = (double)luaL_checknumber(L, 4);
    double rotation = (double)luaL_checknumber(L, 5);
    double scale_x = (double)luaL_checknumber(L, 6);
    double scale_y = (double)luaL_checknumber(L, 7);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Bank.blit() -> %d, %.f, %.f, %.f, %.f, %.f", cell_id, x, y, rotation, scale_x, scale_y);
#endif

#ifdef __NO_MIRRORING__
    double dw = (double)instance->sheet.quad.width * fabs(scale_x);
    double dw = (double)instance->sheet.quad.height * fabs(scale_y);
#else
    double dw = (double)instance->sheet.quad.width * scale_x; // The sign controls the mirroring.
    double dh = (double)instance->sheet.quad.height * scale_y;
#endif

    int dx = (int)x;
    int dy = (int)y;

    GL_Quad_t destination = (GL_Quad_t){ (GLfloat)dx, (GLfloat)dy, (GLfloat)dx + (GLfloat)dw, (GLfloat)dy + (GLfloat)dh };

#ifndef __NO_MIRRORING__
    if (dw < 0.0) { // Compensate for mirroring!
        destination.x0 -= dw;
        destination.x1 -= dw;
    }
    if (dh < 0.0) {
        destination.y0 -= dh;
        destination.y1 -= dh;
    }
#endif

    const GL_Sheet_t *sheet = &instance->sheet;
    GL_sheet_blit(sheet, cell_id, destination, rotation, (GL_Color_t){ 255, 255, 255, 255 });

    return 0;
}

static int graphics_bank_blit(lua_State *L)
{
    if (lua_gettop(L) != 4) {
        return graphics_bank_blit4(L);
    } else
    if (lua_gettop(L) != 5) {
        return graphics_bank_blit5(L);
    } else
    if (lua_gettop(L) != 6) {
        return graphics_bank_blit6(L);
    } else
    if (lua_gettop(L) != 7) {
        return graphics_bank_blit7(L);
    } else {
        return luaL_error(L, "<GRAPHICS> method requires 1 arguments");
    }
}

static int graphics_canvas_width(lua_State *L)
{
    if (lua_gettop(L) != 0) {
        return luaL_error(L, "<GRAPHICS> canvas function 0 arguments");
    }
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.width()");
#endif

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    lua_pushinteger(L, environment->display->configuration.width);

    return 1;
}

static int graphics_canvas_height(lua_State *L)
{
    if (lua_gettop(L) != 0) {
        return luaL_error(L, "<GRAPHICS> canvas function 0 arguments");
    }
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.height()");
#endif

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    lua_pushinteger(L, environment->display->configuration.height);

    return 1;
}

static int graphics_canvas_palette0(lua_State *L)
{
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.palette()");
#endif

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

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

static int graphics_canvas_palette1(lua_State *L)
{
    int type = lua_type(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.palette(%d)", type);
#endif

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    GL_Palette_t palette = {};

    if (type == LUA_TSTRING) { // Predefined palette!
        const char *id = luaL_checkstring(L, 1);
        const GL_Palette_t *predefined_palette = graphics_palettes_find(id);
        if (predefined_palette != NULL) {
            palette = *predefined_palette;

            Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> setting predefined palette '%s' w/ %d color(s)", id, predefined_palette->count);
        } else {
            Log_write(LOG_LEVELS_WARNING, "<GRAPHICS> unknown predefined palette w/ id '%s'", id);
        }
    } else
    if (type == LUA_TTABLE) { // User supplied palette.
        palette.count = lua_rawlen(L, 1);
        Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> setting custom palette of #%d color(s)", palette.count);

        if (palette.count > GL_MAX_PALETTE_COLORS) {
            Log_write(LOG_LEVELS_WARNING, "<GRAPHICS> palette has too many colors (%d) - clamping", palette.count);
            palette.count = MAX_PALETTE_COLORS;
        }

        lua_pushnil(L); // first key
        int i = 0; // TODO: define a helper function
        while (lua_next(L, 1) != 0) {
#if 0
            const char *key_type = lua_typename(L, lua_type(L, -2)); // uses 'key' (at index -2) and 'value' (at index -1)
#endif
            const char *argb = lua_tostring(L, -1);
            palette.colors[i++] = GL_palette_parse_color(argb);

            lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
        }
    } else {
        Log_write(LOG_LEVELS_ERROR, "<GRAPHICS> wrong palette type, need to be string or list");
    }

    if (palette.count == 0) {
        return 0;
    }

    Display_palette(environment->display, &palette);

    return 0;
}

static int graphics_canvas_palette(lua_State *L)
{
    if (lua_gettop(L) == 0) {
        return graphics_canvas_palette0(L);
    } else
    if (lua_gettop(L) == 1) {
        return graphics_canvas_palette1(L);
    } else {
        return luaL_error(L, "<GRAPHICS> canvas function 0 arguments");
    }
}

static int graphics_canvas_background(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<GRAPHICS> canvas function takes 1 arguments");
    }
    int color = luaL_checkinteger(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.background(%d)", color);
#endif

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    Display_background(environment->display, color);

    return 0;
}

static int graphics_canvas_shader(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<GRAPHICS> canvas function takes 1 arguments");
    }
    const char *code = luaL_checkstring(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.shader('%s')", code);
#endif

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    Display_shader(environment->display, code);

    return 0;
}

static int graphics_canvas_color(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<GRAPHICS> canvas function takes 1 arguments");
    }
    const char *argb = luaL_checkstring(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.color('%s')", argb);
#endif

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    GL_Color_t color = GL_palette_parse_color(argb);
    size_t index = find_nearest_color(&environment->display->palette, color);
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
static int graphics_canvas_points(lua_State *L)
{
    if (lua_gettop(L) != 2) {
        return luaL_error(L, "<GRAPHICS> canvas function takes 2 arguments");
    }
    int vertices = lua_rawlen(L, 1);
    int color = luaL_checkinteger(L, 2);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.points(%d, %d)", vertices, color);
#endif

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<GRAPHICS> point-sequence as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        points[i] = (GL_Point_t){
                .x = (GLfloat)x + 0.375f, .y = (GLfloat)y + 0.375f
            };
    }

    GL_primitive_points(points, count, (GL_Color_t){ color, color, color, 255 });

    return 0;
}

static int graphics_canvas_polyline(lua_State *L)
{
    if (lua_gettop(L) != 2) {
        return luaL_error(L, "<GRAPHICS> canvas function takes 2 arguments");
    }
    int vertices = lua_rawlen(L, 1);
    int color = luaL_checkinteger(L, 2);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.polyline(%d, %d)", vertices, color);
#endif

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<GRAPHICS> polyline as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        points[i] = (GL_Point_t){
                .x = (GLfloat)x + 0.375f, .y = (GLfloat)y + 0.375f
            };
    }

    GL_primitive_polyline(points, count, (GL_Color_t){ color, color, color, 255 });

    return 0;
}

static int graphics_canvas_strip(lua_State *L)
{
    if (lua_gettop(L) != 2) {
        return luaL_error(L, "<GRAPHICS> canvas function takes 2 arguments");
    }
    int vertices = lua_rawlen(L, 1);
    int color = luaL_checkinteger(L, 2);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.strip(%d, %d)", vertices, color);
#endif

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<GRAPHICS> strip as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        points[i] = (GL_Point_t){
                .x = (GLfloat)x + 0.375f, .y = (GLfloat)y + 0.375f
            };
    }

    GL_primitive_strip(points, count, (GL_Color_t){ color, color, color, 255 });

    return 0;
}

static int graphics_canvas_fan(lua_State *L)
{
    if (lua_gettop(L) != 2) {
        return luaL_error(L, "<GRAPHICS> canvas function takes 2 arguments");
    }
    int vertices = lua_rawlen(L, 1);
    int color = luaL_checkinteger(L, 2);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Canvas.fan(%d, %d)", vertices, color);
#endif

    const size_t count = vertices / 2;
    if (count == 0) {
        Log_write(LOG_LEVELS_INFO, "<GRAPHICS> fan as no vertices");
        return 0;
    }

    double array[vertices];
    luaX_getnumberarray(L, 1, array);

    GL_Point_t points[count];
    for (size_t i = 0; i < count; ++i) {
        double x = array[(i * 2)];
        double y = array[(i * 2) + 1];

        points[i] = (GL_Point_t){
                .x = (GLfloat)x + 0.375f, .y = (GLfloat)y + 0.375f
            };
    }

    GL_primitive_fan(points, count, (GL_Color_t){ color, color, color, 255 });

    return 0;
}

static int graphics_font_new(lua_State *L)
{
    if (lua_gettop(L) != 3) {
        return luaL_error(L, "<GRAPHICS> bank constructor requires 3 arguments");
    }
    const char *file = luaL_checkstring(L, 1);
    int glyph_width = luaL_checkinteger(L, 2);
    int glyph_height = luaL_checkinteger(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.new() -> %s, %d, %d", file, glyph_width, glyph_height);
#endif
    Font_Class_t *instance = (Font_Class_t *)lua_newuserdata(L, sizeof(Font_Class_t));

    Environment_t *environment = (Environment_t *)luaX_getuserdata(L, "environment");

    const Sheet_Data_t *data = graphics_sheets_find(file);

    GL_Sheet_t sheet;
    if (data) {
        GL_sheet_decode(&sheet, data->buffer, data->size, data->quad_width, data->quad_height, to_font_atlas_callback, NULL);
        Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> default font %dx%d allocated", data->quad_width, data->quad_height);
    } else {
        char pathfile[PATH_FILE_MAX] = {};
        strcpy(pathfile, environment->base_path);
        strcat(pathfile, file + 2);

        GL_sheet_load(&sheet, pathfile, glyph_width, glyph_height, to_font_atlas_callback, NULL);
        Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> font '%s' allocated as #%p", pathfile, instance);
    }

    *instance = (Font_Class_t){
            .sheet = sheet
        };

    luaL_setmetatable(L, LUAX_CLASS(Font_Class_t));

    return 1;
}

static int graphics_font_gc(lua_State *L)
{
luaX_dump(L);
    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<FONT> method requires 1 arguments");
    }
    Font_Class_t *instance = (Font_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Font_Class_t));

    GL_sheet_delete(&instance->sheet);
    Log_write(LOG_LEVELS_DEBUG, "<GRAPHICS> font #%p finalized", instance);

    *instance = (Font_Class_t){};

    return 0;
}

static int graphics_font_write(lua_State *L)
{
    if (lua_gettop(L) != 7) {
        return luaL_error(L, "<FONT> method requires 7 arguments");
    }
    Font_Class_t *instance = (Font_Class_t *)luaL_checkudata(L, 1, LUAX_CLASS(Font_Class_t));
    const char *text = luaL_checkstring(L, 2);
    double x = luaL_checknumber(L, 3);
    double y = luaL_checknumber(L, 4);
    int color = luaL_checkinteger(L, 5);
    double scale = luaL_checknumber(L, 6);
    const char *align = luaL_checkstring(L, 7);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.write() -> %s, %d, %d, %d, %d, %s", text, x, y, color, scale, align);
#endif

    const GL_Sheet_t *sheet = &instance->sheet;

    double dw = sheet->quad.width * fabs(scale);
    double dh = sheet->quad.height * fabs(scale);

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
    if (strcasecmp(align, "left") == 0) {
        dx = (int)x;
        dy = (int)y;
    } else
    if (strcasecmp(align, "center") == 0) {
        dx = (int)(x - (width * 0.5f));
        dy = (int)y;
    } else
    if (strcasecmp(align, "right") == 0) {
        dx = (int)(x - width);
        dy = (int)y;
    } else {
        dx = (int)x;
        dy = (int)y;
    }
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Font.write() -> %d, %d, %d", width, dx, dy);
#endif

    GL_Quad_t destination = { .x0 = dx, .y0 = dy, .x1 = dx + dw, .y1 = dy + dh };
    for (const char *ptr = text; *ptr != '\0'; ++ptr) {
#ifndef __NO_LINEFEEDS__
        if (*ptr == '\n') { // Handle carriage-return
            destination.x0 = dx;
            destination.x1 = destination.x0 + dw;
            destination.y0 += dh;
            destination.y1 = destination.y0 + dh;
            continue;
        } else
#endif
        if (*ptr < ' ') {
            continue;
        }
        GL_sheet_blit_fast(sheet, *ptr - ' ', destination, (GL_Color_t){ color, color, color, 255 });
        destination.x0 += dw;
        destination.x1 = destination.x0 + dw;
    }

    return 0;
}
