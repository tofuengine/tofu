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

#include <config.h>
#include <core/io/display.h>
#include <core/vm/interpreter.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "udt.h"
#include "callbacks.h"

#include <math.h>
#include <string.h>

#define SURFACE_MT      "Tofu_Surface_mt"

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

int surface_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _surface_functions, _surface_constants, nup, SURFACE_MT);
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
    Log_write(LOG_LEVELS_WARNING, "<SURFACE> unknown register w/ id `%s`", id);
    return GL_XFORM_REGISTER_A;
}

static int surface_new1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *file = lua_tostring(L, 1);

    File_System_t *file_system = (File_System_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));
    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    size_t buffer_size;
    void *buffer = FS_load_as_binary(file_system, file, &buffer_size);
    if (!buffer) {
        return luaL_error(L, "<SURFACE> can't load file `%s`", file);
    }
    GL_Surface_t surface;
    GL_surface_decode(&surface, buffer, buffer_size, surface_callback_palette, (void *)&display->palette);
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface `%s` loaded", file);
    free(buffer);

    Surface_Class_t *instance = (Surface_Class_t *)lua_newuserdata(L, sizeof(Surface_Class_t));
    *instance = (Surface_Class_t){
            .surface = surface,
            .xform = (GL_XForm_t){
                    .registers = {
                        0.0f, 0.0f, // No offset
                        1.0f, 0.0f, 1.0f, 0.0f, // Identity matrix.
                        0.0f, 0.0f, // No offset
                    },
                    .clamp = GL_XFORM_CLAMP_REPEAT,
                    .table = NULL
                }
        };
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface allocated as #%p", instance);

    luaL_setmetatable(L, SURFACE_MT);

    return 1;
}

static int surface_new2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t width = (size_t)lua_tonumber(L, 1);
    size_t height = (size_t)lua_tonumber(L, 2);

    GL_Surface_t surface;
    GL_surface_create(&surface, width, height);
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface %dx%d created", width, height);

    Surface_Class_t *instance = (Surface_Class_t *)lua_newuserdata(L, sizeof(Surface_Class_t));
    *instance = (Surface_Class_t){
            .surface = surface,
            .xform = (GL_XForm_t){
                    .registers = {
                        0.0f, 0.0f, // No offset
                        1.0f, 0.0f, 1.0f, 0.0f, // Identity matrix.
                        0.0f, 0.0f, // No offset
                    },
                    .clamp = GL_XFORM_CLAMP_REPEAT,
                    .table = NULL
                }
        };
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface allocated as #%p", instance);

    luaL_setmetatable(L, SURFACE_MT);

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
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Context_t *context = &display->gl;
    GL_context_sanitize(context, &instance->surface);
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface #%p sanitized from context", instance);

    if (instance->xform.table) {
        arrfree(instance->xform.table);
        Log_write(LOG_LEVELS_DEBUG, "<SURFACE> scan-line table #%p deallocated", instance->xform.table);
    }

    GL_surface_delete(&instance->surface);
    Log_write(LOG_LEVELS_DEBUG, "<SURFACE> surface #%p finalized", instance);

    return 0;
}

static int surface_width(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->surface.width);

    return 1;
}

static int surface_height(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    lua_pushinteger(L, instance->surface.height);

    return 1;
}

static int surface_grab(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    GL_context_to_surface(context, &instance->surface);

    return 0;
}

static int surface_blit1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit(context, surface, (GL_Rectangle_t){ 0, 0, surface->width, surface->height }, (GL_Point_t){ .x = 0, .y = 0 });

    return 0;
}

static int surface_blit3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit(context, surface, (GL_Rectangle_t){ 0, 0, surface->width, surface->height }, (GL_Point_t){ .x = x, .y = y });

    return 0;
}

static int surface_blit4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    int rotation = lua_tointeger(L, 4);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface; // TODO: fix explicit struct elements
    GL_context_blit_sr(context, surface, (GL_Rectangle_t){ 0, 0, surface->width, surface->height }, (GL_Point_t){ .x = x, .y = y }, 0.0f, 0.0f, rotation, 0.5, 0.5f);

    return 0;
}

static int surface_blit5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    float scale_x = lua_tonumber(L, 4);
    float scale_y = lua_tonumber(L, 5);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_s(context, surface, (GL_Rectangle_t){ 0, 0, surface->width, surface->height }, (GL_Point_t){ x, y }, scale_x, scale_y);

    return 0;
}

static int surface_blit6(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 6)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    float scale_x = lua_tonumber(L, 4);
    float scale_y = lua_tonumber(L, 5);
    int rotation = lua_tointeger(L, 6);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_sr(context, surface, (GL_Rectangle_t){ 0, 0, surface->width, surface->height }, (GL_Point_t){ x, y }, scale_x, scale_y, rotation, 0.5, 0.5f);

    return 0;
}

static int surface_blit7(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 7)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    int ox = lua_tointeger(L, 4);
    int oy = lua_tointeger(L, 5);
    size_t width = (size_t)lua_tointeger(L, 6);
    size_t height = (size_t)lua_tointeger(L, 7);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit(context, surface, (GL_Rectangle_t){ ox, oy, width, height }, (GL_Point_t){ x, y });

    return 0;
}

static int surface_blit9(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 9)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    int ox = lua_tointeger(L, 4);
    int oy = lua_tointeger(L, 5);
    size_t width = (size_t)lua_tointeger(L, 6);
    size_t height = (size_t)lua_tointeger(L, 7);
    float scale_x = lua_tonumber(L, 8);
    float scale_y = lua_tonumber(L, 9);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_s(context, surface, (GL_Rectangle_t){ ox, oy, width, height }, (GL_Point_t){ x, y }, scale_x, scale_y);

    return 0;
}

static int surface_blit10(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 10)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    int ox = lua_tointeger(L, 4);
    int oy = lua_tointeger(L, 5);
    size_t width = (size_t)lua_tointeger(L, 6);
    size_t height = (size_t)lua_tointeger(L, 7);
    float scale_x = lua_tonumber(L, 8);
    float scale_y = lua_tonumber(L, 9);
    int rotation = lua_tointeger(L, 10);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_sr(context, surface, (GL_Rectangle_t){ ox, oy, width, height }, (GL_Point_t){ .x = x, .y = y }, scale_x, scale_y, rotation, 0.5f, 0.5f);

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
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_x(context, surface, (GL_Point_t){ .x = 0, .y = 0 }, &instance->xform);

    return 0;
}

static int surface_xform3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);

    Display_t *display = (Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Context_t *context = &display->gl;
    const GL_Surface_t *surface = &instance->surface;
    GL_context_blit_x(context, surface, (GL_Point_t){ .x = x, .y = y }, &instance->xform);

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
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    float h = lua_tonumber(L, 2);
    float v = lua_tonumber(L, 3);

    instance->xform.registers[GL_XFORM_REGISTER_H] = h;
    instance->xform.registers[GL_XFORM_REGISTER_V] = v;

    return 0;
}

static int surface_matrix3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    float x0 = lua_tonumber(L, 2);
    float y0 = lua_tonumber(L, 3);

    instance->xform.registers[GL_XFORM_REGISTER_X] = x0;
    instance->xform.registers[GL_XFORM_REGISTER_Y] = y0;

    return 0;
}

static int surface_matrix5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    float a = lua_tonumber(L, 2);
    float b = lua_tonumber(L, 3);
    float c = lua_tonumber(L, 4);
    float d = lua_tonumber(L, 5);

    instance->xform.registers[GL_XFORM_REGISTER_A] = a;
    instance->xform.registers[GL_XFORM_REGISTER_B] = b;
    instance->xform.registers[GL_XFORM_REGISTER_C] = c;
    instance->xform.registers[GL_XFORM_REGISTER_D] = d;

    return 0;
}

static int surface_matrix7(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 7)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    float a = lua_tonumber(L, 2);
    float b = lua_tonumber(L, 3);
    float c = lua_tonumber(L, 4);
    float d = lua_tonumber(L, 5);
    float x0 = lua_tonumber(L, 6);
    float y0 = lua_tonumber(L, 7);

    instance->xform.registers[GL_XFORM_REGISTER_A] = a;
    instance->xform.registers[GL_XFORM_REGISTER_B] = b;
    instance->xform.registers[GL_XFORM_REGISTER_C] = c;
    instance->xform.registers[GL_XFORM_REGISTER_D] = d;
    instance->xform.registers[GL_XFORM_REGISTER_X] = x0;
    instance->xform.registers[GL_XFORM_REGISTER_Y] = y0;

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
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);
    const char *clamp = lua_tostring(L, 2);

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
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    if (instance->xform.table) {
        arrfree(instance->xform.table);
        Log_write(LOG_LEVELS_DEBUG, "<SURFACE> scan-line table #%p deallocated", instance->xform.table);
    }
    instance->xform.table = NULL;

    return 0;
}

static int surface_table2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Surface_Class_t *instance = (Surface_Class_t *)lua_touserdata(L, 1);

    GL_XForm_Table_Entry_t *table = NULL;

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        int index = lua_tointeger(L, -2);
        GL_XForm_Table_Entry_t entry = { 0 };
        entry.scan_line = index - 1; // The scan-line indicator is the array index (minus one).

        lua_pushnil(L);
        for (size_t i = 0; lua_next(L, -2); ++i) { // Scan the value, which is an array.
            if (i == GL_XForm_Registers_t_CountOf) {
                Log_write(LOG_LEVELS_WARNING, "<SURFACE> too many operation for table entry w/ id #%d", index);
                lua_pop(L, 2);
                break;
            }
            entry.count = i + 1;
            entry.operations[i].id = lua_isstring(L, -2) ? string_to_register(lua_tostring(L, -2)) : (GL_XForm_Registers_t)lua_tointeger(L, -2);
            entry.operations[i].value = (float)lua_tonumber(L, -1);

            lua_pop(L, 1);
        }

        arrpush(table, entry);

        lua_pop(L, 1);
    }
    arrpush(table, (GL_XForm_Table_Entry_t){ .scan_line = -1 }); // Set the end-of-data (safety) marker

    if (instance->xform.table) {
        arrfree(instance->xform.table);
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
