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

#include "shapes.h"

#include <config.h>
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>
#include <systems/display.h>
#include <systems/interpreter.h>
#include <systems/storage.h>

#include "udt.h"

#define LOG_CONTEXT "shapes"
#define MODULE_NAME "tofu.graphics.shapes"

static int shapes_point_4onnn_0(lua_State *L);
static int shapes_hline_5onnnn_0(lua_State *L);
static int shapes_vline_5onnnn_0(lua_State *L);
static int shapes_line_6onnnnn_0(lua_State *L);
// static int shapes_tline_6onnnnonnnn_0(lua_State *L);
static int shapes_polyline_3otn_0(lua_State *L);
static int shapes_fill_4onnn_0(lua_State *L);
static int shapes_triangle_9osnnnnnnn_0(lua_State *L);
static int shapes_rectangle_7osnnnnn_0(lua_State *L);
static int shapes_circle_6osnnnn_0(lua_State *L);

int shapes_loader(lua_State *L)
{
    char file[PATH_MAX] = { 0 };
    path_lua_to_fs(file, MODULE_NAME);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));
    Storage_Resource_t *script = Storage_load(storage, file + 1, STORAGE_RESOURCE_STRING);

    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){
            .data = S_SCHARS(script),
            .size = S_SLENTGH(script),
            .name = file
        },
        (const struct luaL_Reg[]){
            { "point", shapes_point_4onnn_0 },
            { "hline", shapes_hline_5onnnn_0 },
            { "vline", shapes_vline_5onnnn_0 },
            { "line", shapes_line_6onnnnn_0 },
            { "polyline", shapes_polyline_3otn_0 },
            { "fill", shapes_fill_4onnn_0 },
            { "triangle", shapes_triangle_9osnnnnnnn_0 },
            { "rectangle", shapes_rectangle_7osnnnnn_0 },
            { "circle", shapes_circle_6osnnnn_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, NULL);
}

static int shapes_point_4onnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 4);

    GL_surface_point(target->surface, (GL_Point_t){ .x = x, .y = y }, index);

    return 0;
}

static int shapes_hline_5onnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    size_t width = (size_t)LUAX_INTEGER(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 5);

    GL_surface_hline(target->surface, (GL_Point_t){ .x = x, .y = y }, width, index);

    return 0;
}

static int shapes_vline_5onnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    size_t height = (size_t)LUAX_INTEGER(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 5);

    GL_surface_vline(target->surface, (GL_Point_t){ .x = x, .y = y }, height, index);

    return 0;
}

static int shapes_line_6onnnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x0 = LUAX_INTEGER(L, 2);
    int y0 = LUAX_INTEGER(L, 3);
    int x1 = LUAX_INTEGER(L, 4);
    int y1 = LUAX_INTEGER(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 6);

    GL_surface_polyline(target->surface, (GL_Point_t[]){
            (GL_Point_t){ .x = x0, .y = y0 },
            (GL_Point_t){ .x = x1, .y = y1 }
        }, 2, index);

    return 0;
}

static inline GL_Point_t *_fetch(lua_State *L, int idx)
{
    GL_Point_t *vertices = NULL;

    int coords[2] = { 0 };

    lua_pushnil(L);
    for (size_t index = 0; lua_next(L, idx); ++index) {
        coords[index % 2] = LUAX_INTEGER(L, -1);

        if (index % 2) { // On odd positions, push into the array.
            const GL_Point_t point = (GL_Point_t){ .x = coords[0], .y = coords[1] };
            arrpush(vertices, point); // Can't pass compound-literal to macro. :(
        }

        lua_pop(L, 1);
    }

    return vertices;
}

static int shapes_polyline_3otn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    // idx #2: LUA_TTABLE
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 3);

    GL_Point_t *vertices = _fetch(L, 2);

    size_t count = arrlenu(vertices);
    if (count < 2) {
        // TODO: free vertices!
        return luaL_error(L, "polyline requires al least 2 points (provided %d)", count);
    }

    GL_surface_polyline(target->surface, vertices, count, index);

    arrfree(vertices);

    return 0;
}

static int shapes_fill_4onnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 4);

    GL_surface_fill(target->surface, (GL_Point_t){ .x = x, .y = y }, index); // TODO: pass `GL_INDEX_COLOR` fake?

    return 0;
}

static int shapes_triangle_9osnnnnnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const char *mode = LUAX_STRING(L, 2);
    int x0 = LUAX_INTEGER(L, 3);
    int y0 = LUAX_INTEGER(L, 4);
    int x1 = LUAX_INTEGER(L, 5);
    int y1 = LUAX_INTEGER(L, 6);
    int x2 = LUAX_INTEGER(L, 7);
    int y2 = LUAX_INTEGER(L, 8);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 9);

    if (mode[0] == 'f') {
        GL_surface_filled_triangle(target->surface,
            (GL_Point_t){ .x = x0, .y = y0 }, (GL_Point_t){ .x = x1, .y = y1 }, (GL_Point_t){ .x = x2, .y = y2 },
            index);
    } else {
        GL_surface_polyline(target->surface, (GL_Point_t[4]){
                (GL_Point_t){ .x = x0, .y = y0 },
                (GL_Point_t){ .x = x1, .y = y1 },
                (GL_Point_t){ .x = x2, .y = y2 },
                (GL_Point_t){ .x = x0, .y = y0 }
            }, 4, index);
    }

    return 0;
}

static int shapes_rectangle_7osnnnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const char *mode = LUAX_STRING(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    size_t width = (size_t)LUAX_INTEGER(L, 5);
    size_t height = (size_t)LUAX_INTEGER(L, 6);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 7);

    if (mode[0] == 'f') {
        GL_surface_filled_rectangle(target->surface, (GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height }, index);
    } else {
        const int x0 = x;
        const int y0 = y;
        const int x1 = x0 + (int)width - 1;
        const int y1 = y0 + (int)height - 1;

        GL_surface_polyline(target->surface, (GL_Point_t[5]){
                (GL_Point_t){ .x = x0, .y = y0 },
                (GL_Point_t){ .x = x0, .y = y1 },
                (GL_Point_t){ .x = x1, .y = y1 },
                (GL_Point_t){ .x = x1, .y = y0 },
                (GL_Point_t){ .x = x0, .y = y0 }
            }, 5, index);
    }

    return 0;
}

static int shapes_circle_6osnnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const char *mode = LUAX_STRING(L, 2);
    int cx = LUAX_INTEGER(L, 3);
    int cy = LUAX_INTEGER(L, 4);
    size_t radius = (size_t)LUAX_INTEGER(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 6);

    if (radius < 1) { // Null radius, just a point regardless mode!
        GL_surface_point(target->surface, (GL_Point_t){ .x = cx, .y = cy }, index);
    } else
    if (mode[0] == 'f') {
        GL_surface_filled_circle(target->surface, (GL_Point_t){ .x = cx, .y = cy }, radius, index);
    } else {
        GL_surface_circle(target->surface, (GL_Point_t){ .x = cx, .y = cy }, radius, index);
    }

    return 0;
}
