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
#include "../log.h"
#include "../gl/gl.h"

#include <math.h>
#include <string.h>

typedef struct _Surface_Class_t {
    // char pathfile[PATH_FILE_MAX];
    GL_Surface_t surface;
    GL_Transformation_t transformation;
} Surface_Class_t;

static int surface_new(lua_State *L);
static int surface_gc(lua_State *L);
static int surface_width(lua_State *L);
static int surface_height(lua_State *L);
static int surface_blit(lua_State *L);
static int surface_transformation(lua_State *L);

static const struct luaL_Reg _surface_functions[] = {
    { "new", surface_new },
    {"__gc", surface_gc },
    { "width", surface_width },
    { "height", surface_height },
    { "blit", surface_blit },
    { "transformation", surface_transformation },
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
    GL_Pixel_t *dst = (GL_Pixel_t *)surface->data;

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
            .transformation = (GL_Transformation_t){
                    .x0 = 0.0f, .y0 = 0.0f,
                    .a = 1.0f, .b = 0.0f,
                    .c = 0.0f, .d = 1.0f,
                    .clamp = GL_CLAMP_MODE_REPEAT,
                    .perspective = true,
                    .elevation = 0.5f,
                    .horizon = 0.25f
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

static int surface_blit3(lua_State *L)
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
    Log_write(LOG_LEVELS_DEBUG, "Surface.blit() -> %.f, %.f", h, v);
#endif

    instance->transformation.h = h;
    instance->transformation.v = v;
    instance->transformation.perspective = false;

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_x(context, surface, (GL_Point_t){ 0, 0 }, instance->transformation);

    return 0;
}

static int surface_blit4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    double h = (double)lua_tonumber(L, 2);
    double v = (double)lua_tonumber(L, 3);
    double elevation = (double)lua_tonumber(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.blit() -> %.f, %.f, %.f", h, v, elevation);
#endif

    instance->transformation.h = h;
    instance->transformation.v = v;
    instance->transformation.perspective = true;
    instance->transformation.elevation = elevation;

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_x(context, surface, (GL_Point_t){ 0, 0 }, instance->transformation);

    return 0;
}

static int surface_blit(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, surface_blit3)
        LUAX_OVERLOAD_ARITY(4, surface_blit4)
    LUAX_OVERLOAD_END
}

static int surface_transformation3(lua_State *L)
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
    Log_write(LOG_LEVELS_DEBUG, "Surface.transformation() -> %.f, %.f", x0, y0);
#endif

    instance->transformation.x0 = x0;
    instance->transformation.y0 = y0;

    return 0;
}

static int surface_transformation5(lua_State *L)
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
    Log_write(LOG_LEVELS_DEBUG, "Surface.transformation() -> %.f, %.f, %.f, %.f", a, b, c, d);
#endif

    instance->transformation.a = a;
    instance->transformation.b = b;
    instance->transformation.c = c;
    instance->transformation.d = d;

    return 0;
}

static int surface_transformation7(lua_State *L)
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
    double x0 = (double)lua_tonumber(L, 2);
    double y0 = (double)lua_tonumber(L, 3);
    double a = (double)lua_tonumber(L, 4);
    double b = (double)lua_tonumber(L, 5);
    double c = (double)lua_tonumber(L, 6);
    double d = (double)lua_tonumber(L, 7);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.transformation() -> %.f, %.f, %.f, %.f, %.f, %.f", x0, y0, a, b, c, d);
#endif

    instance->transformation.x0 = x0;
    instance->transformation.y0 = y0;
    instance->transformation.a = a;
    instance->transformation.b = b;
    instance->transformation.c = c;
    instance->transformation.d = d;

    return 0;
}

static int surface_transformation(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, surface_transformation3)
        LUAX_OVERLOAD_ARITY(5, surface_transformation5)
        LUAX_OVERLOAD_ARITY(7, surface_transformation7)
    LUAX_OVERLOAD_END
}
