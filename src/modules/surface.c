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
    // char full_path[PATH_FILE_MAX];
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
    luaX_Script script = { (const char *)_surface_lua, _surface_lua_len, "surface.lua" };
    int nup = luaX_unpackupvalues(L);
    return luaX_newmodule(L, &script, _surface_functions, _surface_constants, nup, LUAX_CLASS(Surface_Class_t));
}

static void to_indexed_atlas_callback(void *parameters, GL_Surface_t *surface, const void *data)
{
    const GL_Palette_t *palette = (const GL_Palette_t *)parameters;

    const GL_Color_t *src = (const GL_Color_t *)data;
    GL_Pixel_t *dst = surface->data;

    for (size_t i = surface->data_size; i; --i) {
        GL_Color_t color = *(src++);
        *(dst++) = GL_palette_find_nearest_color(palette, color);
    }
}

static GL_XForm_Registers_t string_to_register(const char *id)
{
    if (id[0] == 'h') {
        return GL_XFORM_REGISTER_H;
    } else
    if (id[0] == 'v') {
        return GL_XFORM_REGISTER_V;
    } else
    if (id[0] == 'a') {
        return GL_XFORM_REGISTER_A;
    } else
    if (id[0] == 'b') {
        return GL_XFORM_REGISTER_B;
    } else
    if (id[0] == 'c') {
        return GL_XFORM_REGISTER_C;
    } else
    if (id[0] == 'd') {
        return GL_XFORM_REGISTER_D;
    } else
    if (id[0] == 'x') {
        return GL_XFORM_REGISTER_X;
    } else
    if (id[0] == 'y') {
        return GL_XFORM_REGISTER_Y;
    }
    Log_write(LOG_LEVELS_WARNING, "<SURFACE> unknown register w/ id '%s'", id);
    return GL_XFORM_REGISTER_A;
}

static int surface_new1(lua_State *L)
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

    size_t buffer_size;
    void *buffer = FS_load_as_binary(&environment->fs, file, &buffer_size);
    if (!buffer) {
        return luaL_error(L, "<SURFACE> can't load file '%s'", file);
    }
    GL_Surface_t surface;
    GL_surface_decode(&surface, buffer, buffer_size, to_indexed_atlas_callback, (void *)&display->palette);
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface '%s' loaded", file);
    free(buffer);

    Surface_Class_t *instance = (Surface_Class_t *)lua_newuserdata(L, sizeof(Surface_Class_t));
    *instance = (Surface_Class_t){
            .surface = surface,
            .xform = (GL_XForm_t){
                    .state = (GL_XForm_State_t){
                        .h = 0.0f, .v = 0.0f,
                        .a = 1.0f, .b = 0.0f, .c = 1.0f, .d = 0.0f,
                        .x = 0.0f, .y = 0.0f,
                    },
                    .clamp = GL_XFORM_CLAMP_REPEAT,
                    .table = NULL
                }
        };
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface allocated as #%p", instance);

    luaL_setmetatable(L, LUAX_CLASS(Surface_Class_t));

    return 1;
}

static int surface_new2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    size_t width = (size_t)lua_tonumber(L, 1);
    size_t height = (size_t)lua_tonumber(L, 2);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.new() -> %s", file);
#endif

    GL_Surface_t surface;
    GL_surface_create(&surface, width, height);
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface %d x %d create", width, height);

    Surface_Class_t *instance = (Surface_Class_t *)lua_newuserdata(L, sizeof(Surface_Class_t));
    *instance = (Surface_Class_t){
            .surface = surface,
            .xform = (GL_XForm_t){
                    .state = (GL_XForm_State_t){
                        .h = 0.0f, .v = 0.0f,
                        .a = 1.0f, .b = 0.0f, .c = 1.0f, .d = 0.0f,
                        .x = 0.0f, .y = 0.0f,
                    },
                    .clamp = GL_XFORM_CLAMP_REPEAT,
                    .table = NULL
                }
        };
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface allocated as #%p", instance);

    luaL_setmetatable(L, LUAX_CLASS(Surface_Class_t));

    return 1;
}

static int surface_new(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, surface_new1) // file
        LUAX_OVERLOAD_ARITY(2, surface_new2) // width, height
    LUAX_OVERLOAD_END
}

static int surface_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    GL_Context_t *context = &display->gl;
    GL_context_sanitize(context, &instance->surface);
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface #%p sanitized from context", instance);

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

    const GL_Context_t *context = &display->gl;
    GL_context_to_surface(context, &instance->surface);

    return 0;
}

static int surface_blit1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.blit()");
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit(context, surface, (GL_Rectangle_t){ 0, 0, surface->width, surface->height }, (GL_Point_t){ (int)0, (int)0 });

    return 0;
}

static int surface_blit3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    float x = (float)lua_tonumber(L, 2);
    float y = (float)lua_tonumber(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.blit() -> %.f, %.f", x, y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit(context, surface, (GL_Rectangle_t){ 0, 0, surface->width, surface->height }, (GL_Point_t){ (int)x, (int)y });

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
    float x = (float)lua_tonumber(L, 2);
    float y = (float)lua_tonumber(L, 3);
    float rotation = (float)lua_tonumber(L, 4);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.blit() -> %.f, %.f, %.f", x, y, rotation);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_sr(context, surface, (GL_Rectangle_t){ 0, 0, surface->width, surface->height }, (GL_Point_t){ (int)x, (int)y }, 0.0f, 0.0f, rotation, 0.5, 0.5f);

    return 0;
}

static int surface_blit5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    float x = (float)lua_tonumber(L, 2);
    float y = (float)lua_tonumber(L, 3);
    float scale_x = (float)lua_tonumber(L, 4);
    float scale_y = (float)lua_tonumber(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.blit() -> %.f, %.f, %.f, %.f", x, y, scale_x, scale_y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_s(context, surface, (GL_Rectangle_t){ 0, 0, surface->width, surface->height }, (GL_Point_t){ (int)x, (int)y }, scale_x, scale_y);

    return 0;
}

static int surface_blit6(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 6)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    float x = (float)lua_tonumber(L, 2);
    float y = (float)lua_tonumber(L, 3);
    float scale_x = (float)lua_tonumber(L, 4);
    float scale_y = (float)lua_tonumber(L, 5);
    float rotation = (float)lua_tonumber(L, 6);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.blit() -> %.f, %.f, %.f, %.f, %.f", x, y, scale_x, scale_y, rotation);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_sr(context, surface, (GL_Rectangle_t){ 0, 0, surface->width, surface->height }, (GL_Point_t){ (int)x, (int)y }, scale_x, scale_y, rotation, 0.5, 0.5f);

    return 0;
}

static int surface_blit7(lua_State *L)
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
    float x = (float)lua_tonumber(L, 2);
    float y = (float)lua_tonumber(L, 3);
    float ox = (float)lua_tonumber(L, 4);
    float oy = (float)lua_tonumber(L, 5);
    float width = (float)lua_tonumber(L, 6);
    float height = (float)lua_tonumber(L, 7);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.blit() -> %.f, %.f, %.f, %.f, %.f, %.f", x, y, ox, oy, width, height);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit(context, surface, (GL_Rectangle_t){ (int)ox, (int)oy, (int)width, (int)height }, (GL_Point_t){ (int)x, (int)y });

    return 0;
}

static int surface_blit9(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 9)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    float x = (float)lua_tonumber(L, 2);
    float y = (float)lua_tonumber(L, 3);
    float ox = (float)lua_tonumber(L, 4);
    float oy = (float)lua_tonumber(L, 5);
    float width = (float)lua_tonumber(L, 6);
    float height = (float)lua_tonumber(L, 7);
    float scale_x = (float)lua_tonumber(L, 8);
    float scale_y = (float)lua_tonumber(L, 9);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.blit() -> %.f, %.f, %.f, %.f, %.f, %.f, %.f, %.f", x, y, ox, oy, width, height, scale_x, scale_y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_s(context, surface, (GL_Rectangle_t){ (int)ox, (int)oy, (int)width, (int)height }, (GL_Point_t){ (int)x, (int)y }, scale_x, scale_y);

    return 0;
}

static int surface_blit10(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 10)
        LUAX_SIGNATURE_ARGUMENT(luaX_isuserdata)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
        LUAX_SIGNATURE_ARGUMENT(luaX_isnumber)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    float x = (float)lua_tonumber(L, 2);
    float y = (float)lua_tonumber(L, 3);
    float ox = (float)lua_tonumber(L, 4);
    float oy = (float)lua_tonumber(L, 5);
    float width = (float)lua_tonumber(L, 6);
    float height = (float)lua_tonumber(L, 7);
    float scale_x = (float)lua_tonumber(L, 8);
    float scale_y = (float)lua_tonumber(L, 9);
    float rotation = (float)lua_tonumber(L, 10);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.blit() -> %.f, %.f, %.f, %.f, %.f, %.f, %.f, %.f, %.f", x, y, ox, oy, width, height, scale_x, scale_y, rotation);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_sr(context, surface, (GL_Rectangle_t){ (int)ox, (int)oy, (int)width, (int)height }, (GL_Point_t){ (int)x, (int)y }, scale_x, scale_y, rotation, 0.5, 0.5f);

    return 0;
}

// static int surface_blit1(lua_State *L)
// static int surface_blit3(lua_State *L) x y
// static int surface_blit4(lua_State *L) x y r
// static int surface_blit5(lua_State *L) x y sx sy
// static int surface_blit6(lua_State *L) x y sx sy r
// static int surface_blit7(lua_State *L) x y ox oy w h
// static int surface_blit9(lua_State *L) x y ox oy w h sx sy
// static int surface_blit10(lua_State *L) x y ox oy w h sx sy r
static int surface_blit(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, surface_blit1)
        LUAX_OVERLOAD_ARITY(3, surface_blit3)
        LUAX_OVERLOAD_ARITY(4, surface_blit4)
        LUAX_OVERLOAD_ARITY(5, surface_blit5)
        LUAX_OVERLOAD_ARITY(6, surface_blit6)
        LUAX_OVERLOAD_ARITY(7, surface_blit7)
        LUAX_OVERLOAD_ARITY(9, surface_blit9)
        LUAX_OVERLOAD_ARITY(10, surface_blit10)
    LUAX_OVERLOAD_END
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
    GL_context_blit_x(context, surface, (GL_Point_t){ 0, 0 }, &instance->xform);

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
    float x = (float)lua_tonumber(L, 2);
    float y = (float)lua_tonumber(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.xform() -> %.f, %.f", x, y);
#endif

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(2));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_x(context, surface, (GL_Point_t){ (int)x, (int)y }, &instance->xform);

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
    float h = (float)lua_tonumber(L, 2);
    float v = (float)lua_tonumber(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.offset() -> %.f, %.f", h, v);
#endif

    instance->xform.state.h = h;
    instance->xform.state.v = v;

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
    float x0 = (float)lua_tonumber(L, 2);
    float y0 = (float)lua_tonumber(L, 3);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.matrix() -> %.f, %.f", x0, y0);
#endif

    instance->xform.state.x = x0;
    instance->xform.state.y = y0;

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
    float a = (float)lua_tonumber(L, 2);
    float b = (float)lua_tonumber(L, 3);
    float c = (float)lua_tonumber(L, 4);
    float d = (float)lua_tonumber(L, 5);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.matrix() -> %.f, %.f, %.f, %.f", a, b, c, d);
#endif

    instance->xform.state.a = a;
    instance->xform.state.b = b;
    instance->xform.state.c = c;
    instance->xform.state.d = d;

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
    float a = (float)lua_tonumber(L, 2);
    float b = (float)lua_tonumber(L, 3);
    float c = (float)lua_tonumber(L, 4);
    float d = (float)lua_tonumber(L, 5);
    float x0 = (float)lua_tonumber(L, 6);
    float y0 = (float)lua_tonumber(L, 7);
#ifdef __DEBUG_API_CALLS__
    Log_write(LOG_LEVELS_DEBUG, "Surface.matrix() -> %.f, %.f, %.f, %.f, %.f, %.f", a, b, c, d, x0, y0);
#endif

    instance->xform.state.a = a;
    instance->xform.state.b = b;
    instance->xform.state.c = c;
    instance->xform.state.d = d;
    instance->xform.state.x = x0;
    instance->xform.state.y = y0;

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
        Log_write(LOG_LEVELS_DEBUG, "<SURFACE> scan-line table #%p deallocated", instance->xform.table);
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

    size_t count = luaX_count(L, 2) + 1; // Make room for the EOD marker.

    GL_XForm_Table_Entry_t *table = malloc(count * sizeof(GL_XForm_Table_Entry_t));
    if (!table) {
        return luaL_error(L, "<SURFACE> can't allocate memory");
    }

    lua_pushnil(L);
    for (size_t i = 0; lua_next(L, 2); ++i) {
        int index = (int)lua_tointeger(L, -2);
        table[i].scan_line = index; // The scan-line indicator is the array index.

        lua_pushnil(L);
        for (size_t j = 0; lua_next(L, -2); ++j) { // Scan the value, which is an array.
            if (j == GL_XFORM_TABLE_MAX_OPERATIONS) {
                Log_write(LOG_LEVELS_WARNING, "<SURFACE> too many operation for table entry #%d (id #%d)", i, index);
                lua_pop(L, 2);
                break;
            }
            table[i].count = j + 1;
            table[i].operations[j].id = lua_isstring(L, -2) ? string_to_register(lua_tostring(L, -2)) : (GL_XForm_Registers_t)lua_tointeger(L, -2);
            table[i].operations[j].value = (float)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
    }
    table[count - 1].scan_line = -1; // Set the end-of-data (safety) marker

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
