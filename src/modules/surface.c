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

#include "surface.h"

#include "../core/luax.h"

#include "../config.h"
#include "../environment.h"
#include "../interpreter.h"
#include "../log.h"
#include "../gl/gl.h"

#include <math.h>
#include <string.h>
#ifdef DEBUG
  #include <stb/stb_leakcheck.h>
#endif

typedef struct _Surface_Class_t {
    // char pathfile[PATH_FILE_MAX];
    GL_Surface_t surface;
    GL_XForm_t xform;
} Surface_Class_t;

static int surface_new(lua_State *L);
static int surface_gc(lua_State *L);
static int surface_width(lua_State *L);
static int surface_height(lua_State *L);
static int surface_grab(lua_State *L);
static int surface_blit(lua_State *L);
static int surface_xform(lua_State *L);
static int surface_offset(lua_State *L);
static int surface_matrix(lua_State *L);
static int surface_clamp(lua_State *L);
static int surface_table(lua_State *L);

static const struct luaL_Reg _surface_functions[] = {
    { "new", surface_new },
    {"__gc", surface_gc },
    { "width", surface_width },
    { "height", surface_height },
    { "grab", surface_grab },
    { "blit", surface_blit },
    { "xform", surface_xform },
    { "offset", surface_offset },
    { "matrix", surface_matrix },
    { "clamp", surface_clamp },
    { "table", surface_table },
    { NULL, NULL }
};

static const luaX_Const _surface_constants[] = {
    { NULL }
};

#include "surface.inc"

int surface_loader(lua_State *L)
{
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, NULL, _surface_functions, _surface_constants, nup, LUAX_CLASS(Surface_Class_t));
}

static void to_indexed_atlas_callback(void *parameters, GL_Surface_t *surface, const void *data)
{
    const GL_Palette_t *palette = (const GL_Palette_t *)parameters;

    const GL_Color_t *src = (const GL_Color_t *)data;
    GL_Pixel_t *dst = surface->data;

    for (size_t y = 0; y < surface->height; ++y) {
        int row_offset = surface->width * y;

        for (size_t x = 0; x < surface->width; ++x) {
            size_t offset = row_offset + x;

            GL_Color_t color = src[offset];
            dst[offset] = GL_palette_find_nearest_color(palette, color);
        }
    }
}

static int surface_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
    LUAX_SIGNATURE_END
    const char *file = lua_tostring(L, 1);

#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.new() -> %s", file);
#endif

    Environment_t *environment = (Environment_t *)lua_touserdata(L, lua_upvalueindex(1));
    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    char pathfile[PATH_FILE_MAX] = {};
    strcpy(pathfile, environment->base_path);
    strcat(pathfile, file);

    GL_Surface_t surface;
    GL_surface_load(&surface, pathfile, to_indexed_atlas_callback, (void *)&display->palette);
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface '%s' loaded", pathfile);

    Surface_Class_t *instance = (Surface_Class_t *)lua_newuserdata(L, sizeof(Surface_Class_t));
    *instance = (Surface_Class_t){
            .surface = surface,
            .xform = (GL_XForm_t){
                    .h = 0.0f, .v = 0.0f,
                    .a = 1.0f, .b = 0.0f, .c = 1.0f, .d = 0.0f,
                    .x = 0.0f, .y = 0.0f,
                    .clamp = GL_XFORM_CLAMP_REPEAT,
                    .table = NULL
                }
        };
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface allocated as #%p", pathfile, instance);

    luaL_setmetatable(L, LUAX_CLASS(Surface_Class_t));

    return 1;
}

static int surface_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    if (instance->xform.table) {
        free(instance->xform.table);
        Log_write(LOG_LEVELS_DEBUG, "<SURFACE> scan-line table #%p deallocated", instance->xform.table);
    }

    GL_surface_delete(&instance->surface);
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface #%p finalized", instance);

    *instance = (Surface_Class_t){};

    return 0;
}

static int surface_width(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->surface.width);

    return 1;
}

static int surface_height(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->surface.height);

    return 1;
}

static int surface_grab(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_context_to_surface(&display->gl, &instance->surface);

    return 0;
}

// static int surface_blit1(lua_State *L)
// static int surface_blit3(lua_State *L) x y
// static int surface_blit5(lua_State *L) x y w h
// static int surface_blit7(lua_State *L) x y ox oy w h
// static int surface_blit9(lua_State *L) x y ox oy w h sx sy
// static int surface_blit10(lua_State *L) x y ox oy w h sx sy r
static int surface_blit(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    double x = (double)lua_tonumber(L, 2);
    double y = (double)lua_tonumber(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.blit() -> %.f, %.f", x, y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit(context, surface, (GL_Rectangle_t){ 0, 0, surface->width, surface->height }, (GL_Point_t){ (int)x, (int)y });

    return 0;
}

static int surface_xform1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.xform()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_x(context, surface, (GL_Point_t){ 0, 0 }, instance->xform);

    return 0;
}

static int surface_xform3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    double x = (double)lua_tonumber(L, 2);
    double y = (double)lua_tonumber(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.xform() -> %.f, %.f", x, y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_x(context, surface, (GL_Point_t){ (int)x, (int)y }, instance->xform);

    return 0;
}

static int surface_xform(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, surface_xform1)
        LUAX_OVERLOAD_ARITY(3, surface_xform3)
    LUAX_OVERLOAD_END
}

static int surface_offset(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    double h = (double)lua_tonumber(L, 2);
    double v = (double)lua_tonumber(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.offset() -> %.f, %.f", h, v);
#endif

    instance->xform.h = h;
    instance->xform.v = v;

    return 0;
}

static int surface_matrix3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    double x0 = (double)lua_tonumber(L, 2);
    double y0 = (double)lua_tonumber(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.matrix() -> %.f, %.f", x0, y0);
#endif

    instance->xform.x = x0;
    instance->xform.y = y0;

    return 0;
}

static int surface_matrix5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    double a = (double)lua_tonumber(L, 2);
    double b = (double)lua_tonumber(L, 3);
    double c = (double)lua_tonumber(L, 4);
    double d = (double)lua_tonumber(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.matrix() -> %.f, %.f, %.f, %.f", a, b, c, d);
#endif

    instance->xform.a = a;
    instance->xform.b = b;
    instance->xform.c = c;
    instance->xform.d = d;

    return 0;
}

static int surface_matrix7(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 7)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    double a = (double)lua_tonumber(L, 2);
    double b = (double)lua_tonumber(L, 3);
    double c = (double)lua_tonumber(L, 4);
    double d = (double)lua_tonumber(L, 5);
    double x0 = (double)lua_tonumber(L, 6);
    double y0 = (double)lua_tonumber(L, 7);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.matrix() -> %.f, %.f, %.f, %.f, %.f, %.f", a, b, c, d, x0, y0);
#endif

    instance->xform.a = a;
    instance->xform.b = b;
    instance->xform.c = c;
    instance->xform.d = d;
    instance->xform.x = x0;
    instance->xform.y = y0;

    return 0;
}

static int surface_matrix(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, surface_matrix3)
        LUAX_OVERLOAD_ARITY(5, surface_matrix5)
        LUAX_OVERLOAD_ARITY(7, surface_matrix7)
    LUAX_OVERLOAD_END
}

static int surface_clamp(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isstring)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    const char *clamp = lua_tostring(L, 2);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.clamp() -> %s", clamp);
#endif

    if (clamp[0] == 'e') {
        instance->xform.clamp = GL_XFORM_CLAMP_EDGE;
    } else
    if (clamp[0] == 'b') {
        instance->xform.clamp = GL_XFORM_CLAMP_BORDER;
    } else
    if (clamp[0] == 'r') {
        instance->xform.clamp = GL_XFORM_CLAMP_REPEAT;
    }

    return 0;
}

static int surface_table1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.table()");
#endif

    if (instance->xform.table) {
        free(instance->xform.table);
//        Log_write(LOG_LEVELS_DEBUG, "<SURFACE> scan-line table #%p deallocated", instance->xform.table);
    }
    instance->xform.table = NULL;

    return 0;
}

static int surface_table2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_istable)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
#ifdef __DEBUG_API_CALLS__
    int type = lua_type(L, 2);
    Log_write(LOG_LEVELS_DEBUG, "Surface.table(%d)", type);
#endif

    int count = luaX_count(L, 2) + 1; // Make room for the EOD marker.

    GL_XForm_Table_Entry_t *table = malloc(count * sizeof(GL_XForm_Table_Entry_t));
    if (!table) {
        return luaL_error(L, "<SURFACE> can't allocate memory");
    }

    lua_pushnil(L); // first key
    for (size_t i = 0; lua_next(L, 2); ++i) {
//        int index = lua_tointeger(L, -2);
        float YABCD[5];
        luaX_tonumberarray(L, -1, YABCD, 5);

        table[i].y = YABCD[0];
        table[i].a = YABCD[1]; table[i].b = YABCD[2];
        table[i].c = YABCD[3]; table[i].d = YABCD[4];

        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
    table[count].y = -1; // Set the end-of-data (safety) marker

    if (instance->xform.table) {
        free(instance->xform.table);
//        Log_write(LOG_LEVELS_TRACE, "<SURFACE> scan-line table #%p reallocated as #%p", instance->xform.table, table);
    }
    instance->xform.table = table;

    return 0;
}

static int surface_table(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, surface_table1)
        LUAX_OVERLOAD_ARITY(2, surface_table2)
    LUAX_OVERLOAD_END
}
