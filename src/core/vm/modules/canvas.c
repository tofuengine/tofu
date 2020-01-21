/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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

#include "canvas.h"

#include <config.h>
#include <core/environment.h>
#include <core/io/display.h>
#include <core/vm/interpreter.h>
#include <libs/imath.h>
#include <libs/log.h>
#include <libs/gl/gl.h>
#include <libs/stb.h>

#include "callbacks.h"
#include "udt.h"
#include "resources/palettes.h"

#include <math.h>
#include <string.h>
#include <time.h>

#define LOG_CONTEXT "canvas"

#define CANVAS_MT        "Tofu_Canvas_mt"

static int canvas_new(lua_State *L);
static int canvas_gc(lua_State *L);
static int canvas_width(lua_State *L);
static int canvas_height(lua_State *L);
static int canvas_size(lua_State *L);
static int canvas_center(lua_State *L);
static int canvas_push(lua_State *L);
static int canvas_pop(lua_State *L);
static int canvas_reset(lua_State *L);
static int canvas_background(lua_State *L);
static int canvas_color(lua_State *L);
static int canvas_shift(lua_State *L);
static int canvas_transparent(lua_State *L);
static int canvas_clipping(lua_State *L);
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
static int canvas_process(lua_State *L);
//static int canvas_grab(lua_State *L);

// TODO: rename `Canvas` to `Context`?

static const struct luaL_Reg _canvas_functions[] = {
    { "new", canvas_new },
    { "__gc", canvas_gc },
    { "width", canvas_width },
    { "height", canvas_height },
    { "size", canvas_size },
    { "center", canvas_center },
    { "push", canvas_push },
    { "pop", canvas_pop },
    { "reset", canvas_reset },
    { "background", canvas_background },
    { "color", canvas_color },
    { "shift", canvas_shift },
    { "transparent", canvas_transparent },
    { "clipping", canvas_clipping },
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
    { "process", canvas_process },
//    { "grab", canvas_grab },
    { NULL, NULL }
};

static const uint8_t _canvas_lua[] = {
#include "canvas.inc"
};

static luaX_Script _canvas_script = { (const char *)_canvas_lua, sizeof(_canvas_lua), "@canvas.lua" }; // Trace as filename internally.

int canvas_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_canvas_script, _canvas_functions, NULL, nup, CANVAS_MT);
}

static int canvas_new0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 0)
    LUAX_SIGNATURE_END

    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    Canvas_Class_t *instance = (Canvas_Class_t *)lua_newuserdata(L, sizeof(Canvas_Class_t));
    *instance = (Canvas_Class_t){
            .context = display->context,
            .allocated = false
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas allocated as %p", instance);

    luaL_setmetatable(L, CANVAS_MT);

    return 1;
}

static int canvas_new1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *file = lua_tostring(L, 1);

    const File_System_t *file_system = (const File_System_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));
    const Display_t *display = (const Display_t *)lua_touserdata(L, lua_upvalueindex(USERDATA_DISPLAY));

    File_System_Chunk_t chunk = FSaux_load(file_system, file, FILE_SYSTEM_CHUNK_IMAGE);
    if (chunk.type == FILE_SYSTEM_CHUNK_NULL) {
        return luaL_error(L, "can't load file `%s`", file);
    }
    GL_Context_t context;
    bool result = GL_context_create(&context, chunk.var.image.width, chunk.var.image.height);
    if (!result) {
        return luaL_error(L, "can't create %dx%d canvas", chunk.var.image.width, chunk.var.image.height);
    }
    GL_surface_fetch(&context.surface, (GL_Image_t){ .width = chunk.var.image.width, .height = chunk.var.image.height, .data = chunk.var.image.pixels }, surface_callback_palette, (void *)&display->palette);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "file `%s` loaded into canvas", file);
    FSaux_release(chunk);

    Canvas_Class_t *instance = (Canvas_Class_t *)lua_newuserdata(L, sizeof(Canvas_Class_t));
    *instance = (Canvas_Class_t){
            .context = context,
            .allocated = true
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas allocated as %p", instance);

    luaL_setmetatable(L, CANVAS_MT);

    return 1;
}

static int canvas_new2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t width = (size_t)lua_tonumber(L, 1);
    size_t height = (size_t)lua_tonumber(L, 2);

    GL_Context_t context;
    bool result = GL_context_create(&context, width, height);
    if (!result) {
        return luaL_error(L, "can't create %dx%d canvas", width, height);
    }

    Canvas_Class_t *instance = (Canvas_Class_t *)lua_newuserdata(L, sizeof(Canvas_Class_t));
    *instance = (Canvas_Class_t){
            .context = context,
            .allocated = true
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas allocated as %p", instance);

    luaL_setmetatable(L, CANVAS_MT);

    return 1;
}

static int canvas_new(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_new0)
        LUAX_OVERLOAD_ARITY(1, canvas_new1)
        LUAX_OVERLOAD_ARITY(2, canvas_new2)
    LUAX_OVERLOAD_END
}

static int canvas_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    if (instance->allocated) {
        GL_context_delete(&instance->context);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context %p deleted", &instance->context);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p finalized", instance);

    return 0;
}

static int canvas_width(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    const GL_Context_t *context = &instance->context;

    lua_pushinteger(L, context->surface.width);

    return 1;
}

static int canvas_height(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    const GL_Context_t *context = &instance->context;

    lua_pushinteger(L, context->surface.height);

    return 1;
}

static int canvas_size(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    const GL_Context_t *context = &instance->context;

    lua_pushinteger(L, context->surface.width);
    lua_pushinteger(L, context->surface.height);

    return 2;
}

static int canvas_center(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    const GL_Context_t *context = &instance->context;

    lua_pushinteger(L, context->surface.width / 2);
    lua_pushinteger(L, context->surface.height / 2);

    return 2;
}

static int canvas_push(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    GL_Context_t *context = &instance->context;
    GL_context_push(context);

    return 0;
}

static int canvas_pop(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    GL_Context_t *context = &instance->context;
    GL_context_pop(context);

    return 0;
}

static int canvas_reset(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    GL_Context_t *context = &instance->context;
    GL_context_reset(context);

    return 0;
}

static int canvas_background(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 2);

    GL_Context_t *context = &instance->context;
    GL_context_background(context, index);

    return 0;
}

static int canvas_color(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 2);

    GL_Context_t *context = &instance->context;
    GL_context_color(context, index);

    return 0;
}

static int canvas_shift1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    GL_Context_t *context = &instance->context;
    GL_context_shifting(context, NULL, NULL, 0);

    return 0;
}

static int canvas_shift2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    size_t *from = NULL;
    size_t *to = NULL;
    size_t count = 0;

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        arrpush(from, lua_tointeger(L, -2));
        arrpush(to, lua_tointeger(L, -1));
        ++count;

        lua_pop(L, 1);
    }

    GL_Context_t *context = &instance->context;
    GL_context_shifting(context, from, to, count);

    arrfree(from);
    arrfree(to);

    return 0;
}

static int canvas_shift3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    size_t from = (size_t)lua_tointeger(L, 2);
    size_t to = (size_t)lua_tointeger(L, 3);

    GL_Context_t *context = &instance->context;
    GL_context_shifting(context, &from, &to, 1);

    return 0;
}

static int canvas_shift(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, canvas_shift1)
        LUAX_OVERLOAD_ARITY(2, canvas_shift2)
        LUAX_OVERLOAD_ARITY(3, canvas_shift3)
    LUAX_OVERLOAD_END
}

static int canvas_transparent1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    GL_Context_t *context = &instance->context;
    GL_context_transparent(context, NULL, NULL, 0);

    return 0;
}

static int canvas_transparent2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    GL_Pixel_t *indexes = NULL;
    GL_Bool_t *transparent = NULL;
    size_t count = 0;

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        arrpush(indexes, (GL_Pixel_t)lua_tointeger(L, -2));
        arrpush(transparent, lua_toboolean(L, -1) ? GL_BOOL_TRUE : GL_BOOL_FALSE);
        ++count;

        lua_pop(L, 1);
    }

    GL_Context_t *context = &instance->context;
    GL_context_transparent(context, indexes, transparent, count);

    arrfree(indexes);
    arrfree(transparent);

    return 0;
}

static int canvas_transparent3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 2);
    GL_Bool_t transparent = lua_toboolean(L, 3) ? GL_BOOL_TRUE : GL_BOOL_FALSE;

    GL_Context_t *context = &instance->context;
    GL_context_transparent(context, &index, &transparent, 1);

    return 0;
}

static int canvas_transparent(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, canvas_transparent1)
        LUAX_OVERLOAD_ARITY(2, canvas_transparent2)
        LUAX_OVERLOAD_ARITY(3, canvas_transparent3)
    LUAX_OVERLOAD_END
}

static int canvas_clipping1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    GL_Context_t *context = &instance->context;
    GL_context_clipping(context, NULL);

    return 0;
}

static int canvas_clipping5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    size_t width = (size_t)lua_tointeger(L, 4);
    size_t height = (size_t)lua_tointeger(L, 5);

    GL_Context_t *context = &instance->context;
    GL_context_clipping(context, &(GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height });

    return 0;
}

static int canvas_clipping(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, canvas_clipping1)
        LUAX_OVERLOAD_ARITY(5, canvas_clipping5)
    LUAX_OVERLOAD_END
}

#ifdef __GL_MASK_SUPPORT__
static int canvas_mask1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    GL_Context_t *context = &instance->context;
    GL_context_mask(context, NULL);

    return 0;
}

static int canvas_mask2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA, LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    int type = lua_type(L, 2);

    GL_Context_t *context = &instance->context;
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

static int canvas_mask3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    const Surface_Class_t *instance = (const Surface_Class_t *)lua_touserdata(L, 2);
    GL_Pixel_t index = lua_tointeger(L, 3);

    GL_Context_t *context = &instance->context;
    GL_context_mask(context, &(GL_Mask_t){ &instance->surface, index });

    return 0;
}

static int canvas_mask(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, canvas_mask1)
        LUAX_OVERLOAD_ARITY(2, canvas_mask2)
        LUAX_OVERLOAD_ARITY(3, canvas_mask3)
    LUAX_OVERLOAD_END
}
#endif

static int canvas_clear1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 1)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);

    const GL_Context_t *context = &instance->context;
    GL_context_clear(context, context->state.background); // FIXME: move around?

    return 0;
}

static int canvas_clear2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 2)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 2);

    const GL_Context_t *context = &instance->context;
    GL_context_clear(context, index);

    return 0;
}

static int canvas_clear(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, canvas_clear1)
        LUAX_OVERLOAD_ARITY(2, canvas_clear2)
    LUAX_OVERLOAD_END
}

static int canvas_point(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 4);

    const GL_Context_t *context = &instance->context;
    GL_primitive_point(context, (GL_Point_t){ .x = x, .y = y }, index);

    return 0;
}

static int canvas_hline(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    size_t width = (size_t)lua_tointeger(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 5);

    const GL_Context_t *context = &instance->context;
    GL_primitive_hline(context, (GL_Point_t){ .x = x, .y = y }, width, index);

    return 0;
}

static int canvas_vline(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    size_t height = (size_t)lua_tointeger(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 5);

    const GL_Context_t *context = &instance->context;
    GL_primitive_vline(context, (GL_Point_t){ .x = x, .y = y }, height, index);

    return 0;
}

static int canvas_line(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 6)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    int x0 = lua_tointeger(L, 2);
    int y0 = lua_tointeger(L, 3);
    int x1 = lua_tointeger(L, 4);
    int y1 = lua_tointeger(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 6);

    const GL_Context_t *context = &instance->context;
    GL_Point_t vertices[2] = {
            (GL_Point_t){ .x = x0, .y = y0 },
            (GL_Point_t){ .x = x1, .y = y1 }
        };
    GL_primitive_polyline(context, vertices, 2, index);

    return 0;
}

static int canvas_polyline(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TTABLE)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 3);

    GL_Point_t *vertices = NULL;
    size_t count = 0;
    int aux = 0;

    lua_pushnil(L);
    while (lua_next(L, 2)) {
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
        const GL_Context_t *context = &instance->context;
        GL_primitive_polyline(context, vertices, count / 2, index);
    } else {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "no enough points for polyline (%d)", count);
    }

    arrfree(vertices);

    return 0;
}

static int canvas_fill(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 4);

    const GL_Context_t *context = &instance->context;
    GL_context_fill(context, (GL_Point_t){ .x = x, .y = y }, index);

    return 0;
}

static int canvas_triangle(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 9)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    const char *mode = lua_tostring(L, 2);
    int x0 = lua_tointeger(L, 3);
    int y0 = lua_tointeger(L, 4);
    int x1 = lua_tointeger(L, 5);
    int y1 = lua_tointeger(L, 6);
    int x2 = lua_tointeger(L, 7);
    int y2 = lua_tointeger(L, 8);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 9);

    const GL_Context_t *context = &instance->context;
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
    LUAX_SIGNATURE_BEGIN(L, 7)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    const char *mode = lua_tostring(L, 2);
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    size_t width = (size_t)lua_tointeger(L, 5);
    size_t height = (size_t)lua_tointeger(L, 6);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 7);

    const GL_Context_t *context = &instance->context;
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
    LUAX_SIGNATURE_BEGIN(L, 6)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TSTRING)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    const char *mode = lua_tostring(L, 2);
    int cx = lua_tointeger(L, 3);
    int cy = lua_tointeger(L, 4);
    int radius = lua_tointeger(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 6);

    const GL_Context_t *context = &instance->context;
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
    LUAX_SIGNATURE_BEGIN(L, 3)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);

    const GL_Context_t *context = &instance->context;
    const GL_Surface_t *surface = &context->surface;
    GL_Pixel_t index = surface->data[y * surface->width + x];

    lua_pushinteger(L, index);

    return 1;
}

static int canvas_poke(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 4)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)lua_tointeger(L, 4);

    const GL_Context_t *context = &instance->context;
    const GL_Surface_t *surface = &context->surface;
    surface->data[y * surface->width + x] = index;

    return 0;
}

static int canvas_process(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L, 5)
        LUAX_SIGNATURE_ARGUMENT(LUA_TUSERDATA)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
        LUAX_SIGNATURE_ARGUMENT(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Class_t *instance = (Canvas_Class_t *)lua_touserdata(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    size_t width = (size_t)lua_tointeger(L, 4);
    size_t height = (size_t)lua_tointeger(L, 5);

    const GL_Context_t *context = &instance->context;
    GL_context_process(context, (GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height }); // TODO: pass pointers!!!

    return 0;
}
