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

#include "udt.h"
#include "utils/callbacks.h"

#define LOG_CONTEXT "canvas"
#define META_TABLE  "Tofu_Graphics_Canvas_mt"

static int canvas_new_v_1u(lua_State *L);
static int canvas_gc_1u_0(lua_State *L);
static int canvas_size_1u_2nn(lua_State *L);
static int canvas_center_1u_2nn(lua_State *L);
static int canvas_push_1u_0(lua_State *L);
static int canvas_pop_2uN_0(lua_State *L);
static int canvas_reset_1u_0(lua_State *L);
static int canvas_background_2un_0(lua_State *L);
static int canvas_color_2un_0(lua_State *L);
static int canvas_pattern_2un_0(lua_State *L);
static int canvas_shift_v_0(lua_State *L);
static int canvas_transparent_v_0(lua_State *L);
static int canvas_clipping_v_0(lua_State *L);
#ifdef __GL_MASK_SUPPORT__
static int canvas_mask(lua_State *L);
#endif
static int canvas_clear_2uN_0(lua_State *L);
static int canvas_point_4unnN_0(lua_State *L);
static int canvas_hline_5unnnN_0(lua_State *L);
static int canvas_vline_5unnnN_0(lua_State *L);
static int canvas_line_6unnnnN_0(lua_State *L);
static int canvas_polyline_3utN_0(lua_State *L);
static int canvas_fill_4unnN_0(lua_State *L);
static int canvas_triangle_9usnnnnnnN_0(lua_State *L);
static int canvas_rectangle_7usnnnnN_0(lua_State *L);
static int canvas_circle_6usnnnN_0(lua_State *L);
static int canvas_peek_3unn_1n(lua_State *L);
static int canvas_poke_4unnn_0(lua_State *L);
static int canvas_process_v_0(lua_State *L);
static int canvas_copy_v_0(lua_State *L);
static int canvas_blit_v_0(lua_State *L);
//static int canvas_grab(lua_State *L);

// TODO: rename `Canvas` to `Context`?

static const struct luaL_Reg _canvas_functions[] = {
    { "new", canvas_new_v_1u },
    { "__gc", canvas_gc_1u_0 },
    { "size", canvas_size_1u_2nn },
    { "center", canvas_center_1u_2nn },
    { "push", canvas_push_1u_0 },
    { "pop", canvas_pop_2uN_0 },
    { "reset", canvas_reset_1u_0 },
    { "background", canvas_background_2un_0 },
    { "color", canvas_color_2un_0 },
    { "pattern", canvas_pattern_2un_0 },
    { "shift", canvas_shift_v_0 },
    { "transparent", canvas_transparent_v_0 },
    { "clipping", canvas_clipping_v_0 },
    { "clear", canvas_clear_2uN_0 },
#ifdef __GL_MASK_SUPPORT__
    { "mask", canvas_mask },
#endif
    { "point", canvas_point_4unnN_0 },
    { "hline", canvas_hline_5unnnN_0 },
    { "vline", canvas_vline_5unnnN_0 },
    { "line", canvas_line_6unnnnN_0 },
    { "polyline", canvas_polyline_3utN_0 },
    { "fill", canvas_fill_4unnN_0 },
    { "triangle", canvas_triangle_9usnnnnnnN_0 },
    { "rectangle", canvas_rectangle_7usnnnnN_0 },
    { "circle", canvas_circle_6usnnnN_0 },
    { "peek", canvas_peek_3unn_1n },
    { "poke", canvas_poke_4unnn_0 },
    { "process", canvas_process_v_0 },
    { "copy", canvas_copy_v_0 },
    { "blit", canvas_blit_v_0 },
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

static int canvas_new_0_1u(lua_State *L)
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

static int canvas_new_2nn_1u(lua_State *L)
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

static int canvas_new_3snU_1u(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    GL_Pixel_t transparent_index = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    const Palette_Object_t *palette = (const Palette_Object_t *)LUAX_OPTIONAL_USERDATA(L, 3, NULL);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));
    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Callback_Palette_Closure_t closure = (Callback_Palette_Closure_t){
            .palette = palette ? &palette->palette : Display_get_palette(display), // Use current display's if not passed.
            .transparent = transparent_index,
            .threshold = 0
        };

    GL_Context_t *context;
    if (Storage_exists(storage, name)) {
        const Storage_Resource_t *image = Storage_load(storage, name, STORAGE_RESOURCE_IMAGE);
        if (!image) {
            return luaL_error(L, "can't load file `%s`", name);
        }

        context = GL_context_decode(S_IWIDTH(image), S_IHEIGHT(image), S_IPIXELS(image), surface_callback_palette, (void *)&closure);
        if (!context) {
            return luaL_error(L, "can't decode file `%s`", name);
        }
    } else {
        return luaL_error(L, "unknown file `%s`", name);
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context %p loaded from file `%s`", context, name);

    Canvas_Object_t *self = (Canvas_Object_t *)lua_newuserdatauv(L, sizeof(Canvas_Object_t), 1);
    *self = (Canvas_Object_t){
            .context = context,
            .allocated = true
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p allocated w/ context", self, context);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int canvas_new_3snn_1u(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    GL_Pixel_t background_index = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    GL_Pixel_t foreground_index = (GL_Pixel_t)LUAX_INTEGER(L, 3);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));

    Callback_Indexes_Closure_t closure = (Callback_Indexes_Closure_t){
            .background = background_index,
            .foreground = foreground_index
        };

    GL_Context_t *context;
    if (Storage_exists(storage, name)) {
        const Storage_Resource_t *image = Storage_load(storage, name, STORAGE_RESOURCE_IMAGE);
        if (!image) {
            return luaL_error(L, "can't load file `%s`", name);
        }

        context = GL_context_decode(S_IWIDTH(image), S_IHEIGHT(image), S_IPIXELS(image), surface_callback_indexes, (void *)&closure);
        if (!context) {
            return luaL_error(L, "can't decode file `%s`", name);
        }
    } else {
        return luaL_error(L, "unknown file `%s`", name);
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context %p loaded from file `%s`", context, name);

    Canvas_Object_t *self = (Canvas_Object_t *)lua_newuserdatauv(L, sizeof(Canvas_Object_t), 1);
    *self = (Canvas_Object_t){
            .context = context,
            .allocated = true
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p allocated w/ context", self, context);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int canvas_new_v_1u(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_new_0_1u)
        LUAX_OVERLOAD_SIGNATURE(canvas_new_3snU_1u, LUA_TSTRING, LUA_TNUMBER)
        LUAX_OVERLOAD_ARITY(2, canvas_new_2nn_1u)
        LUAX_OVERLOAD_SIGNATURE(canvas_new_3snU_1u, LUA_TSTRING, LUA_TNUMBER, LUA_TUSERDATA)
        LUAX_OVERLOAD_ARITY(3, canvas_new_3snn_1u)
    LUAX_OVERLOAD_END
}

static int canvas_gc_1u_0(lua_State *L)
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

static int canvas_size_1u_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_USERDATA(L, 1);

    const GL_Context_t *context = self->context;
    GL_Size_t size = GL_context_get_size(context);

    lua_pushinteger(L, (lua_Integer)size.width);
    lua_pushinteger(L, (lua_Integer)size.height);

    return 2;
}

static int canvas_center_1u_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_USERDATA(L, 1);

    const GL_Context_t *context = self->context;
    GL_Size_t size = GL_context_get_size(context);

    lua_pushinteger(L, (lua_Integer)(size.width / 2));
    lua_pushinteger(L, (lua_Integer)(size.height / 2));

    return 2;
}

static int canvas_push_1u_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_push(context);

    return 0;
}

static int canvas_pop_2uN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    size_t levels = (size_t)LUAX_OPTIONAL_INTEGER(L, 2, 1);

    GL_Context_t *context = self->context;
    GL_context_pop(context, levels > 0 ? levels : SIZE_MAX);

    return 0;
}

static int canvas_reset_1u_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_reset(context);

    return 0;
}

static int canvas_background_2un_0(lua_State *L)
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

static int canvas_color_2un_0(lua_State *L)
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

static int canvas_pattern_2un_0(lua_State *L)
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

static int canvas_shift_1u_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_set_shifting(context, NULL, NULL, 0);

    return 0;
}

static int canvas_shift_2ut_0(lua_State *L)
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
        arrpush(from, (GL_Pixel_t)LUAX_INTEGER(L, -2));
        arrpush(to, (GL_Pixel_t)LUAX_INTEGER(L, -1));

        lua_pop(L, 1);
    }

    GL_Context_t *context = self->context;
    GL_context_set_shifting(context, from, to, arrlen(from));

    arrfree(from);
    arrfree(to);

    return 0;
}

static int canvas_shift_3unn_0(lua_State *L)
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

static int canvas_shift_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, canvas_shift_1u_0)
        LUAX_OVERLOAD_ARITY(2, canvas_shift_2ut_0)
        LUAX_OVERLOAD_ARITY(3, canvas_shift_3unn_0)
    LUAX_OVERLOAD_END
}

static int canvas_transparent_1u_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_set_transparent(context, NULL, NULL, 0);

    return 0;
}

static int canvas_transparent_2ut_0(lua_State *L)
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

static int canvas_transparent_3unb_0(lua_State *L)
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

static int canvas_transparent_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, canvas_transparent_1u_0)
        LUAX_OVERLOAD_ARITY(2, canvas_transparent_2ut_0)
        LUAX_OVERLOAD_ARITY(3, canvas_transparent_3unb_0)
    LUAX_OVERLOAD_END
}

static int canvas_clipping_1u_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);

    GL_Context_t *context = self->context;
    GL_context_set_clipping(context, NULL);

    return 0;
}

static int canvas_clipping_5unnnn_0(lua_State *L)
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

static int canvas_clipping_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, canvas_clipping_1u_0)
        LUAX_OVERLOAD_ARITY(5, canvas_clipping_5unnnn_0)
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

static int canvas_clear_2uN_0(lua_State *L)
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

static int canvas_point_4unnN_0(lua_State *L)
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

static int canvas_hline_5unnnN_0(lua_State *L)
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

static int canvas_vline_5unnnN_0(lua_State *L)
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

static int canvas_line_6unnnnN_0(lua_State *L)
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

static int canvas_polyline_3utN_0(lua_State *L)
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

static int canvas_fill_4unnN_0(lua_State *L)
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

static int canvas_triangle_9usnnnnnnN_0(lua_State *L)
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

static int canvas_rectangle_7usnnnnN_0(lua_State *L)
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

static int canvas_circle_6usnnnN_0(lua_State *L)
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

static int canvas_peek_3unn_1n(lua_State *L)
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

static int canvas_poke_4unnn_0(lua_State *L)
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

static int canvas_process_5ufnnU_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
//    luaX_Reference callback = luaX_tofunction(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    Canvas_Object_t *canvas = (Canvas_Object_t *)LUAX_OPTIONAL_USERDATA(L, 5, self);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const GL_Context_t *context = canvas->context;
    const GL_Surface_t *surface = GL_context_get_surface(self->context);
    GL_context_process(context, surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = surface->width, .height = surface->height }, (GL_Point_t){ .x = x, .y = y },
        _process_callback, &(Process_Closure_t){ .interpreter = interpreter, .L = L });

    return 0;
}

static int canvas_process_9ufnnnnnnU_0(lua_State *L)
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
    Canvas_Object_t *canvas = (Canvas_Object_t *)LUAX_OPTIONAL_USERDATA(L, 9, self);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const GL_Context_t *context = canvas->context;
    const GL_Surface_t *surface = GL_context_get_surface(self->context);
    GL_context_process(context, surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height }, (GL_Point_t){ .x = x, .y = y },
        _process_callback, &(Process_Closure_t){ .interpreter = interpreter, .L = L });

    return 0;
}

static int canvas_process_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(4, canvas_process_5ufnnU_0)
        LUAX_OVERLOAD_ARITY(5, canvas_process_5ufnnU_0)
        LUAX_OVERLOAD_ARITY(8, canvas_process_9ufnnnnnnU_0)
        LUAX_OVERLOAD_ARITY(9, canvas_process_9ufnnnnnnU_0)
    LUAX_OVERLOAD_END
}

static int canvas_copy_4unnU_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    Canvas_Object_t *canvas = (Canvas_Object_t *)LUAX_OPTIONAL_USERDATA(L, 4, self);

    const GL_Context_t *context = canvas->context;
    const GL_Surface_t *surface = GL_context_get_surface(self->context);
    GL_context_copy(context, surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = surface->width, .height = surface->height }, (GL_Point_t){ .x = x, .y = y });

    return 0;
}

static int canvas_copy_8unnnnnnU_0(lua_State *L)
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
    Canvas_Object_t *canvas = (Canvas_Object_t *)LUAX_OPTIONAL_USERDATA(L, 8, self);

    const GL_Context_t *context = canvas->context;
    const GL_Surface_t *surface = GL_context_get_surface(self->context);
    GL_context_copy(context, surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height }, (GL_Point_t){ .x = x, .y = y });

    return 0;
}

static int canvas_copy_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, canvas_copy_4unnU_0)
        LUAX_OVERLOAD_ARITY(4, canvas_copy_4unnU_0)
        LUAX_OVERLOAD_ARITY(7, canvas_copy_8unnnnnnU_0)
        LUAX_OVERLOAD_ARITY(8, canvas_copy_8unnnnnnU_0)
    LUAX_OVERLOAD_END
}

static int canvas_blit_3uuU_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    Canvas_Object_t *canvas = (Canvas_Object_t *)LUAX_USERDATA(L, 2);
    XForm_Object_t *xform = (XForm_Object_t *)LUAX_OPTIONAL_USERDATA(L, 3, NULL);

    const GL_Context_t *context = canvas->context;
    const GL_Surface_t *surface = GL_context_get_surface(self->context);
    if (xform) {
        GL_context_xform(context, surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = surface->width, .height = surface->height }, (GL_Point_t){ .x = 0, .y = 0 }, &xform->xform);
    } else {
        GL_context_blit(context, surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = surface->width, .height = surface->height }, (GL_Point_t){ .x = 0, .y = 0 });
    }

    return 0;
}

static int canvas_blit_5unnuU_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    Canvas_Object_t *canvas = (Canvas_Object_t *)LUAX_USERDATA(L, 4);
    XForm_Object_t *xform = (XForm_Object_t *)LUAX_OPTIONAL_USERDATA(L, 5, NULL);

    const GL_Context_t *context = canvas->context;
    const GL_Surface_t *surface = GL_context_get_surface(self->context);
    if (xform) {
        GL_context_xform(context, surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = surface->width, .height = surface->height }, (GL_Point_t){ .x = x, .y = y }, &xform->xform);
    } else {
        GL_context_blit(context, surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = surface->width, .height = surface->height }, (GL_Point_t){ .x = x, .y = y });
    }

    return 0;
}

static int canvas_blit_9unnnnnnuU_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_USERDATA(L, 1);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    int ox = LUAX_INTEGER(L, 4);
    int oy = LUAX_INTEGER(L, 5);
    size_t width = (size_t)LUAX_INTEGER(L, 6);
    size_t height = (size_t)LUAX_INTEGER(L, 7);
    Canvas_Object_t *canvas = (Canvas_Object_t *)LUAX_USERDATA(L, 8);
    XForm_Object_t *xform = (XForm_Object_t *)LUAX_OPTIONAL_USERDATA(L, 9, NULL);

    const GL_Context_t *context = canvas->context;
    const GL_Surface_t *surface = GL_context_get_surface(self->context);
    if (xform) {
        GL_context_xform(context, surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height }, (GL_Point_t){ .x = x, .y = y }, &xform->xform);
    } else {
        GL_context_blit(context, surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height }, (GL_Point_t){ .x = x, .y = y });
    }

    return 0;
}

static int canvas_blit_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, canvas_blit_3uuU_0)
        LUAX_OVERLOAD_ARITY(3, canvas_blit_3uuU_0)
        LUAX_OVERLOAD_ARITY(4, canvas_blit_5unnuU_0)
        LUAX_OVERLOAD_ARITY(5, canvas_blit_5unnuU_0)
        LUAX_OVERLOAD_ARITY(8, canvas_blit_9unnnnnnuU_0)
        LUAX_OVERLOAD_ARITY(9, canvas_blit_9unnnnnnuU_0)
    LUAX_OVERLOAD_END
}
