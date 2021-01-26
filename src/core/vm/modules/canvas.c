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

#include "canvas.h"

#include <config.h>
#include <core/environment.h>
#include <core/io/display.h>
#include <core/io/storage.h>
#include <core/vm/interpreter.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "callbacks.h"
#include "udt.h"

#define LOG_CONTEXT "canvas"
#define META_TABLE  "Tofu_Graphics_Canvas_mt"

static int canvas_new(lua_State *L);
static int canvas_gc(lua_State *L);
static int canvas_size(lua_State *L);
static int canvas_center(lua_State *L);
static int canvas_push(lua_State *L);
static int canvas_pop(lua_State *L);
static int canvas_reset(lua_State *L);
static int canvas_background(lua_State *L);
static int canvas_color(lua_State *L);
static int canvas_pattern(lua_State *L);
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
static int canvas_copy(lua_State *L);
//static int canvas_grab(lua_State *L);

// TODO: rename `Canvas` to `Context`?

static const struct luaL_Reg _canvas_functions[] = {
    { "new", canvas_new },
    { "__gc", canvas_gc },
    { "size", canvas_size },
    { "center", canvas_center },
    { "push", canvas_push },
    { "pop", canvas_pop },
    { "reset", canvas_reset },
    { "background", canvas_background },
    { "color", canvas_color },
    { "pattern", canvas_pattern },
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
    { "copy", canvas_copy },
    { NULL, NULL }
};

static const uint8_t _canvas_lua[] = {
#include "canvas.inc"
};

static luaX_Script _canvas_script = { (const char *)_canvas_lua, sizeof(_canvas_lua), "@canvas.lua" }; // Trace as filename internally.

int canvas_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_canvas_script, _canvas_functions, NULL, nup, META_TABLE);
}

static int canvas_new0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Canvas_Object_t *self = (Canvas_Object_t *)lua_newuserdatauv(L, sizeof(Canvas_Object_t), 1);
    *self = (Canvas_Object_t){
            .context = Display_get_context(display),
            .allocated = false
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p allocated w/ default context", self);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int canvas_new1_3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *file = LUAX_STRING(L, 1);
    GL_Pixel_t background_index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 2, 0);
    GL_Pixel_t foreground_index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 3, background_index);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));
    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    const GL_Pixel_t indexes[] = { background_index, foreground_index };

    GL_Surface_Callback_t callback;
    void *user_data;
    if (background_index == foreground_index) {
        callback = surface_callback_palette; // Back- and fore-ground equals, convert to palette.
        user_data = (void *)Display_get_palette(display);
    } else {
        callback = surface_callback_indexes; // Otherwise, convert to the specified indexes.
        user_data = (void *)indexes;
    }

    GL_Context_t *context;
    if (Storage_exists(storage, file)) {
        const Storage_Resource_t *image = Storage_load(storage, file, STORAGE_RESOURCE_IMAGE);
        if (!image) {
            return luaL_error(L, "can't load file `%s`", file);
        }

        context = GL_context_decode(S_IWIDTH(image), S_IHEIGHT(image), S_IPIXELS(image), callback, user_data);
        if (!context) {
            return luaL_error(L, "can't decode file `%s`", file);
        }
    } else {
        return luaL_error(L, "unknown file `%s`", file);
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context %p loaded from file `%s`", context, file);

    Canvas_Object_t *self = (Canvas_Object_t *)lua_newuserdatauv(L, sizeof(Canvas_Object_t), 1);
    *self = (Canvas_Object_t){
            .context = context,
            .allocated = true
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p allocated w/ context", self, context);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int canvas_new2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t width = (size_t)LUAX_INTEGER(L, 1);
    size_t height = (size_t)LUAX_INTEGER(L, 2);

    GL_Context_t *context = GL_context_create(width, height);
    if (!context) {
        return luaL_error(L, "can't create %dx%d context", width, height);
    }

    Canvas_Object_t *self = (Canvas_Object_t *)lua_newuserdatauv(L, sizeof(Canvas_Object_t), 1);
    *self = (Canvas_Object_t){
            .context = context,
            .allocated = true
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p allocated w/ context", self, context);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int canvas_new(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_new0)
        LUAX_OVERLOAD_ARITY(1, canvas_new1_3)
        LUAX_OVERLOAD_ARITY(2, canvas_new2)
        LUAX_OVERLOAD_ARITY(3, canvas_new1_3)
    LUAX_OVERLOAD_END
}

static int canvas_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    if (self->allocated) {
        GL_context_destroy(self->context);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context %p destroyed", self->context);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p finalized", self);

    return 0;
}

static int canvas_size(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_USERDATA(L, 1);

    const GL_Context_t *context = self->context;

    lua_pushinteger(L, (lua_Integer)context->surface->width);
    lua_pushinteger(L, (lua_Integer)context->surface->height);

    return 2;
}

static int canvas_center(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_USERDATA(L, 1);

    const GL_Context_t *context = self->context;

    lua_pushinteger(L, (lua_Integer)(context->surface->width / 2));
    lua_pushinteger(L, (lua_Integer)(context->surface->height / 2));

    return 2;
}

static int canvas_push(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_push(context);

    return 0;
}

static int canvas_pop(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_pop(context);

    return 0;
}

static int canvas_reset(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_reset(context);

    return 0;
}

static int canvas_background(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 2);

    GL_Context_t *context = self->context;
    GL_context_set_background(context, index);

    return 0;
}

static int canvas_color(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 2);

    GL_Context_t *context = self->context;
    GL_context_set_color(context, index);

    return 0;
}

static int canvas_pattern(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pattern_t pattern = (GL_Pattern_t)LUAX_INTEGER(L, 0);

    GL_Context_t *context = self->context;
    GL_context_set_pattern(context, pattern);

    return 0;
}

static int canvas_shift1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_set_shifting(context, NULL, NULL, 0);

    return 0;
}

static int canvas_shift2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Pixel_t *from = NULL;
    GL_Pixel_t *to = NULL;

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        arrpush(from, LUAX_INTEGER(L, -2));
        arrpush(to, LUAX_INTEGER(L, -1));

        lua_pop(L, 1);
    }

    GL_Context_t *context = self->context;
    GL_context_set_shifting(context, from, to, arrlen(from));

    arrfree(from);
    arrfree(to);

    return 0;
}

static int canvas_shift3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pixel_t from = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    GL_Pixel_t to = (GL_Pixel_t)LUAX_INTEGER(L, 3);

    GL_Context_t *context = self->context;
    GL_context_set_shifting(context, &from, &to, 1);

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
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_set_transparent(context, NULL, NULL, 0);

    return 0;
}

static int canvas_transparent2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Pixel_t *indexes = NULL;
    GL_Bool_t *transparent = NULL;

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        arrpush(indexes, (GL_Pixel_t)LUAX_INTEGER(L, -2));
        arrpush(transparent, LUAX_BOOLEAN(L, -1) ? GL_BOOL_TRUE : GL_BOOL_FALSE);

        lua_pop(L, 1);
    }

    GL_Context_t *context = self->context;
    GL_context_set_transparent(context, indexes, transparent, arrlen(indexes));

    arrfree(indexes);
    arrfree(transparent);

    return 0;
}

static int canvas_transparent3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    GL_Bool_t transparent = LUAX_BOOLEAN(L, 3) ? GL_BOOL_TRUE : GL_BOOL_FALSE;

    GL_Context_t *context = self->context;
    GL_context_set_transparent(context, &index, &transparent, 1);

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
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_set_clipping(context, NULL);

    return 0;
}

static int canvas_clipping5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    size_t width = (size_t)LUAX_INTEGER(L, 4);
    size_t height = (size_t)LUAX_INTEGER(L, 5);

    GL_Context_t *context = self->context;
    GL_context_set_clipping(context, &(GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height });

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
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_set_mask(context, NULL);

    return 0;
}

static int canvas_mask2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA, LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int type = lua_type(L, 2);

    GL_Context_t *context = self->context;
    GL_Mask_t mask = context->state.mask;
    if (type == LUA_TUSERDATA) {
        const Surface_Class_t *self = (const Surface_Class_t *)LUAX_USERDATA(L, 1);

        mask.stencil = self->surface;
    } else
    if (type == LUA_TNUMBER) {
        GL_Pixel_t index = LUAX_INTEGER(L, 1);

        mask.threshold = index;
    }
    GL_context_set_mask(context, &mask);

    return 0;
}

static int canvas_mask3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    const Surface_Class_t *self = (const Surface_Class_t *)LUAX_USERDATA(L, 2);
    GL_Pixel_t index = LUAX_INTEGER(L, 3);

    GL_Context_t *context = self->context;
    GL_context_set_mask(context, &(GL_Mask_t){ self->surface, index });

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

static int canvas_clear(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 2, self->context->state.background);

    const GL_Context_t *context = self->context;
    GL_context_clear(context, index);

    return 0;
}

static int canvas_point(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 4, self->context->state.color);

    const GL_Context_t *context = self->context;
    GL_primitive_point(context, (GL_Point_t){ .x = x, .y = y }, index);

    return 0;
}

static int canvas_hline(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    size_t width = (size_t)LUAX_INTEGER(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 5, self->context->state.color); // TODO: is state stack useful?

    const GL_Context_t *context = self->context;
    GL_primitive_hline(context, (GL_Point_t){ .x = x, .y = y }, width, index);

    return 0;
}

static int canvas_vline(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    size_t height = (size_t)LUAX_INTEGER(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 5, self->context->state.color);

    const GL_Context_t *context = self->context;
    GL_primitive_vline(context, (GL_Point_t){ .x = x, .y = y }, height, index);

    return 0;
}

static int canvas_line(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x0 = LUAX_INTEGER(L, 2);
    int y0 = LUAX_INTEGER(L, 3);
    int x1 = LUAX_INTEGER(L, 4);
    int y1 = LUAX_INTEGER(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 6, self->context->state.color);

    const GL_Context_t *context = self->context;
    GL_Point_t vertices[2] = {
            (GL_Point_t){ .x = x0, .y = y0 },
            (GL_Point_t){ .x = x1, .y = y1 }
        };
    GL_primitive_polyline(context, vertices, 2, index);

    return 0;
}

static GL_Point_t *_fetch(lua_State *L, int idx)
{
    GL_Point_t *vertices = NULL;
    size_t index = 0;
    int aux = 0;

    lua_pushnil(L);
    while (lua_next(L, idx)) {
        int value = LUAX_INTEGER(L, -1);
        ++index;
        if (index > 0 && (index % 2) == 0) {
            GL_Point_t point = (GL_Point_t){ .x = aux, .y = value }; // Can't pass compound-literal to macro. :(
            arrpush(vertices, point);
        } else {
            aux = value;
        }
        lua_pop(L, 1);
    }

    return vertices;
}

static int canvas_polyline(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 3, self->context->state.color);

    GL_Point_t *vertices = _fetch(L, 2);

    size_t count = arrlen(vertices);
    if (count > 1) {
        const GL_Context_t *context = self->context;
        GL_primitive_polyline(context, vertices, count, index);
    } else {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "polyline requires al least 2 points (provided %d)", count);
    }

    arrfree(vertices);

    return 0;
}

static int canvas_fill(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 4, self->context->state.color);

    const GL_Context_t *context = self->context;
    GL_context_fill(context, (GL_Point_t){ .x = x, .y = y }, index); // TODO: pass `GL_INDEX_COLOR` fake?

    return 0;
}

static int canvas_triangle(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    const char *mode = LUAX_STRING(L, 2);
    int x0 = LUAX_INTEGER(L, 3);
    int y0 = LUAX_INTEGER(L, 4);
    int x1 = LUAX_INTEGER(L, 5);
    int y1 = LUAX_INTEGER(L, 6);
    int x2 = LUAX_INTEGER(L, 7);
    int y2 = LUAX_INTEGER(L, 8);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 9, self->context->state.color);

    const GL_Context_t *context = self->context;
    if (mode[0] == 'f') {
        GL_primitive_filled_triangle(context, (GL_Point_t){ .x = x0, .y = y0 }, (GL_Point_t){ .x = x1, .y = y1 }, (GL_Point_t){ .x = x2, .y = y2 }, index);
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
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    const char *mode = LUAX_STRING(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    size_t width = (size_t)LUAX_INTEGER(L, 5);
    size_t height = (size_t)LUAX_INTEGER(L, 6);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 7, self->context->state.color);

    const GL_Context_t *context = self->context;
    if (mode[0] == 'f') {
        GL_primitive_filled_rectangle(context, (GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height }, index);
    } else {
        int x0 = x;
        int y0 = y;
        int x1 = x0 + (int)width - 1;
        int y1 = y0 + (int)height - 1;

        GL_Point_t vertices[5] = {
                (GL_Point_t){ .x = x0, .y = y0 },
                (GL_Point_t){ .x = x0, .y = y1 },
                (GL_Point_t){ .x = x1, .y = y1 },
                (GL_Point_t){ .x = x1, .y = y0 },
                (GL_Point_t){ .x = x0, .y = y0 }
            };
        GL_primitive_polyline(context, vertices, 5, index);
    }

    return 0;
}

static int canvas_circle(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    const char *mode = LUAX_STRING(L, 2);
    int cx = LUAX_INTEGER(L, 3);
    int cy = LUAX_INTEGER(L, 4);
    size_t radius = (size_t)LUAX_INTEGER(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 6, self->context->state.color);

    const GL_Context_t *context = self->context;
    if (radius < 1) { // Null radius, just a point regardless mode!
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
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);

    const GL_Context_t *context = self->context;
    lua_pushinteger(L, (lua_Integer)GL_context_peek(context, x, y));

    return 1;
}

static int canvas_poke(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 4);

    GL_Context_t *context = self->context;
    GL_context_poke(context, x, y, index);

    return 0;
}

typedef struct _Process_Closure_t {
    const Interpreter_t *interpreter;
    lua_State *L;
} Process_Closure_t;

static GL_Pixel_t _process_callback(void *user_data, int x, int y, GL_Pixel_t from, GL_Pixel_t to)
{
    Process_Closure_t *closure = (Process_Closure_t *)user_data;

    lua_pushvalue(closure->L, 2); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed, in the meanwhile)
    lua_pushinteger(closure->L, (lua_Integer)x);
    lua_pushinteger(closure->L, (lua_Integer)y);
    lua_pushinteger(closure->L, (lua_Integer)from);
    lua_pushinteger(closure->L, (lua_Integer)to);
    Interpreter_call(closure->interpreter, 4, 1);

    GL_Pixel_t pixel = (GL_Pixel_t)LUAX_INTEGER(closure->L, -1);

    lua_pop(closure->L, 1);

    return pixel;
}

static int canvas_process5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    Canvas_Object_t *source = (Canvas_Object_t *)LUAX_USERDATA(L, 2);
//    luaX_Reference callback = luaX_tofunction(L, 3);
    int x = LUAX_INTEGER(L, 4);
    int y = LUAX_INTEGER(L, 5);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const GL_Context_t *context = self->context;
    const GL_Surface_t *surface = source->context->surface;
    GL_context_process(context, (GL_Point_t){ .x = x, .y = y }, surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = surface->width, .height = surface->height },
        _process_callback, &(Process_Closure_t){ .interpreter = interpreter, .L = L });

    return 0;
}

static int canvas_process8_9(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
//    luaX_Reference callback = luaX_tofunction(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    int ox = LUAX_INTEGER(L, 5);
    int oy = LUAX_INTEGER(L, 6);
    size_t width = (size_t)LUAX_INTEGER(L, 7);
    size_t height = (size_t)LUAX_INTEGER(L, 8);
    Canvas_Object_t *source = (Canvas_Object_t *)LUAX_OPTIONAL_USERDATA(L, 9, self);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const GL_Context_t *context = self->context;
    const GL_Surface_t *surface = source->context->surface;
    GL_context_process(context, (GL_Point_t){ .x = x, .y = y }, surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height },
        _process_callback, &(Process_Closure_t){ .interpreter = interpreter, .L = L });

    return 0;
}

static int canvas_process(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(5, canvas_process5)
        LUAX_OVERLOAD_ARITY(8, canvas_process8_9)
        LUAX_OVERLOAD_ARITY(9, canvas_process8_9)
    LUAX_OVERLOAD_END
}

static int canvas_copy4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    Canvas_Object_t *source = (Canvas_Object_t *)LUAX_USERDATA(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);

    const GL_Context_t *context = self->context;
    const GL_Surface_t *surface = source->context->surface;
    GL_context_copy(context, (GL_Point_t){ .x = x, .y = y }, surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = surface->width, .height = surface->height });

    return 0;
}

static int canvas_copy7_8(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    int ox = LUAX_INTEGER(L, 4);
    int oy = LUAX_INTEGER(L, 5);
    size_t width = (size_t)LUAX_INTEGER(L, 6);
    size_t height = (size_t)LUAX_INTEGER(L, 7);
    Canvas_Object_t *source = (Canvas_Object_t *)LUAX_OPTIONAL_USERDATA(L, 8, self);

    const GL_Context_t *context = self->context;
    const GL_Surface_t *surface = source->context->surface;
    GL_context_copy(context, (GL_Point_t){ .x = x, .y = y }, surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height });

    return 0;
}

static int canvas_copy(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(4, canvas_copy4)
        LUAX_OVERLOAD_ARITY(7, canvas_copy7_8)
        LUAX_OVERLOAD_ARITY(8, canvas_copy7_8)
    LUAX_OVERLOAD_END
}
