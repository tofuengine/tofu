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
#include <libs/imath.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "udt.h"
#include "utils/callbacks.h"
#include "utils/map.h"

#define LOG_CONTEXT "canvas"
#define META_TABLE  "Tofu_Graphics_Canvas_mt"
#define SCRIPT_NAME "@canvas.lua"

#define CANVAS_DEFAULT_BACKGROUND   0
#define CANVAS_DEFAULT_FOREGROUND   1

static int canvas_new_v_1o(lua_State *L);
static int canvas_gc_1o_0(lua_State *L);
static int canvas_size_1o_2nn(lua_State *L);
static int canvas_center_1o_2nn(lua_State *L);
static int canvas_push_1o_0(lua_State *L);
static int canvas_pop_2oN_0(lua_State *L);
static int canvas_reset_1o_0(lua_State *L);
static int canvas_background_2on_0(lua_State *L);
static int canvas_clipping_v_0(lua_State *L);
static int canvas_foreground_2on_0(lua_State *L);
static int canvas_shift_v_0(lua_State *L);
static int canvas_transparent_v_0(lua_State *L);
static int canvas_clear_2oN_0(lua_State *L);
static int canvas_point_4onnN_0(lua_State *L);
static int canvas_hline_5onnnN_0(lua_State *L);
static int canvas_vline_5onnnN_0(lua_State *L);
static int canvas_line_6onnnnN_0(lua_State *L);
// static int canvas_tline_6onnnnonnnn_0(lua_State *L);
static int canvas_polyline_3otN_0(lua_State *L);
static int canvas_fill_4onnN_0(lua_State *L);
static int canvas_triangle_9osnnnnnnN_0(lua_State *L);
static int canvas_rectangle_7osnnnnN_0(lua_State *L);
static int canvas_circle_6osnnnN_0(lua_State *L);
static int canvas_peek_3onn_1n(lua_State *L);
static int canvas_poke_4onnn_0(lua_State *L);
static int canvas_process_v_0(lua_State *L);
static int canvas_copy_v_0(lua_State *L);
static int canvas_blit_v_0(lua_State *L);
static int canvas_stencil_v_0(lua_State *L);
static int canvas_blend_v_0(lua_State *L);
//static int canvas_grab(lua_State *L);

// TODO: rename `Canvas` to `Context`?

static const char _canvas_lua[] = {
#include "canvas.inc"
};

int canvas_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){
            .data = _canvas_lua,
            .size = sizeof(_canvas_lua) / sizeof(char),
            .name = SCRIPT_NAME
        },
        (const struct luaL_Reg[]){
            { "new", canvas_new_v_1o },
            { "__gc", canvas_gc_1o_0 },
            // -- observers --
            { "size", canvas_size_1o_2nn },
            { "center", canvas_center_1o_2nn },
            // -- modifiers --
            { "push", canvas_push_1o_0 },
            { "pop", canvas_pop_2oN_0 },
            { "reset", canvas_reset_1o_0 },
            { "background", canvas_background_2on_0 },
            { "foreground", canvas_foreground_2on_0 },
            { "clipping", canvas_clipping_v_0 },
            { "shift", canvas_shift_v_0 },
            { "transparent", canvas_transparent_v_0 },
            // -- operations --
            { "clear", canvas_clear_2oN_0 },
            { "point", canvas_point_4onnN_0 },
            { "hline", canvas_hline_5onnnN_0 },
            { "vline", canvas_vline_5onnnN_0 },
            { "line", canvas_line_6onnnnN_0 },
            { "polyline", canvas_polyline_3otN_0 },
            { "fill", canvas_fill_4onnN_0 },
            { "triangle", canvas_triangle_9osnnnnnnN_0 },
            { "rectangle", canvas_rectangle_7osnnnnN_0 },
            { "circle", canvas_circle_6osnnnN_0 },
            { "peek", canvas_peek_3onn_1n },
            { "poke", canvas_poke_4onnn_0 },
            { "process", canvas_process_v_0 },
            { "copy", canvas_copy_v_0 },
            { "blit", canvas_blit_v_0 },
            { "stencil", canvas_stencil_v_0 },
            { "blend", canvas_blend_v_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int canvas_new_0_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    GL_Surface_t *surface = Display_get_surface(display);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "default surface %p retrieved", surface);

    Canvas_Object_t *self = (Canvas_Object_t *)luaX_newobject(L, sizeof(Canvas_Object_t), &(Canvas_Object_t){
            .surface = surface,
            .allocated = false,
            .color = {
                .background = CANVAS_DEFAULT_BACKGROUND,
                .foreground = CANVAS_DEFAULT_FOREGROUND
            }
        }, OBJECT_TYPE_CANVAS, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p allocated w/ default context", self);

    return 1;
}

static int canvas_new_2nn_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t width = (size_t)LUAX_INTEGER(L, 1);
    size_t height = (size_t)LUAX_INTEGER(L, 2);

    GL_Surface_t *surface = GL_surface_create(width, height);
    if (!surface) {
        return luaL_error(L, "can't create %dx%d surface", width, height);
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%dx%d surface allocate at %p", width, height, surface);

    Canvas_Object_t *self = (Canvas_Object_t *)luaX_newobject(L, sizeof(Canvas_Object_t), &(Canvas_Object_t){
            .surface = surface,
            .allocated = true,
            .color = {
                .background = CANVAS_DEFAULT_BACKGROUND,
                .foreground = CANVAS_DEFAULT_FOREGROUND
            }
        }, OBJECT_TYPE_CANVAS, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p allocated w/ surface %p", self, surface);

    return 1;
}

static int canvas_new_3sNO_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    GL_Pixel_t transparent_index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 2, 0);
    const Palette_Object_t *palette = (const Palette_Object_t *)LUAX_OPTIONAL_OBJECT(L, 3, OBJECT_TYPE_PALETTE, NULL);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));
    const Display_t *display = (const Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Callback_Palette_Closure_t closure = (Callback_Palette_Closure_t){
            .palette = palette ? palette->palette : Display_get_palette(display), // Use current display's if not passed.
            .transparent = transparent_index,
            .threshold = 0
        };

    GL_Surface_t *surface;
    if (Storage_exists(storage, name)) {
        const Storage_Resource_t *image = Storage_load(storage, name, STORAGE_RESOURCE_IMAGE);
        if (!image) {
            return luaL_error(L, "can't load file `%s`", name);
        }

        surface = GL_surface_decode(S_IWIDTH(image), S_IHEIGHT(image), S_IPIXELS(image), surface_callback_palette, (void *)&closure);
        if (!surface) {
            return luaL_error(L, "can't decode file `%s`", name);
        }
    } else {
        return luaL_error(L, "unknown file `%s`", name);
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface %p loaded from file `%s`", surface, name);

    Canvas_Object_t *self = (Canvas_Object_t *)luaX_newobject(L, sizeof(Canvas_Object_t), &(Canvas_Object_t){
            .surface = surface,
            .allocated = true,
            .color = {
                .background = CANVAS_DEFAULT_BACKGROUND,
                .foreground = CANVAS_DEFAULT_FOREGROUND
            }
        }, OBJECT_TYPE_CANVAS, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p allocated w/ surface %p", self, surface);

    return 1;
}

static int canvas_new_3snn_1o(lua_State *L)
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

    GL_Surface_t *surface;
    if (Storage_exists(storage, name)) {
        const Storage_Resource_t *image = Storage_load(storage, name, STORAGE_RESOURCE_IMAGE);
        if (!image) {
            return luaL_error(L, "can't load file `%s`", name);
        }

        surface = GL_surface_decode(S_IWIDTH(image), S_IHEIGHT(image), S_IPIXELS(image), surface_callback_indexes, (void *)&closure);
        if (!surface) {
            return luaL_error(L, "can't decode file `%s`", name);
        }
    } else {
        return luaL_error(L, "unknown file `%s`", name);
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface %p loaded from file `%s`", surface, name);

    Canvas_Object_t *self = (Canvas_Object_t *)luaX_newobject(L, sizeof(Canvas_Object_t), &(Canvas_Object_t){
            .surface = surface,
            .allocated = true,
            .color = {
                .background = CANVAS_DEFAULT_BACKGROUND,
                .foreground = CANVAS_DEFAULT_FOREGROUND
            }
        }, OBJECT_TYPE_CANVAS, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p allocated w/ surface %p", self, surface);

    return 1;
}

static int canvas_new_v_1o(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, canvas_new_0_1o)
        LUAX_OVERLOAD_SIGNATURE(canvas_new_3sNO_1o, LUA_TSTRING)
        LUAX_OVERLOAD_SIGNATURE(canvas_new_3sNO_1o, LUA_TSTRING, LUA_TNUMBER)
        LUAX_OVERLOAD_ARITY(2, canvas_new_2nn_1o)
        LUAX_OVERLOAD_SIGNATURE(canvas_new_3sNO_1o, LUA_TSTRING, LUA_TNUMBER, LUA_TOBJECT)
        LUAX_OVERLOAD_ARITY(3, canvas_new_3snn_1o)
    LUAX_OVERLOAD_END
}

static int canvas_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    if (self->allocated) {
        GL_surface_destroy(self->surface);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "surface %p destroyed", self->surface);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p finalized", self);

    return 0;
}

static int canvas_size_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    const GL_Surface_t *surface = self->surface;
    lua_pushinteger(L, (lua_Integer)surface->width);
    lua_pushinteger(L, (lua_Integer)surface->height);

    return 2;
}

static int canvas_center_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    const GL_Surface_t *surface = self->surface;
    lua_pushinteger(L, (lua_Integer)(surface->width / 2));
    lua_pushinteger(L, (lua_Integer)(surface->height / 2));

    return 2;
}

static int canvas_push_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    GL_Surface_t *surface = self->surface;
    GL_surface_push(surface);

    return 0;
}

static int canvas_pop_2oN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    size_t levels = (size_t)LUAX_OPTIONAL_INTEGER(L, 2, 1);

    GL_Surface_t *surface = self->surface;
    GL_surface_pop(surface, levels > 0 ? levels : SIZE_MAX);

    return 0;
}

static int canvas_reset_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    GL_Surface_t *surface = self->surface;
    GL_surface_reset(surface);

    return 0;
}

static int canvas_background_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 2);

    self->color.background = index;

    return 0;
}

static int canvas_foreground_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 2);

    self->color.foreground = index;

    return 0;
}

static int canvas_clipping_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    GL_Surface_t *surface = self->surface;
    GL_surface_set_clipping(surface, NULL);

    return 0;
}

static int canvas_clipping_5onnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    size_t width = (size_t)LUAX_INTEGER(L, 4);
    size_t height = (size_t)LUAX_INTEGER(L, 5);

    GL_Surface_t *surface = self->surface;
    GL_surface_set_clipping(surface, &(GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height });

    return 0;
}

static int canvas_clipping_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, canvas_clipping_1o_0)
        LUAX_OVERLOAD_ARITY(5, canvas_clipping_5onnnn_0)
    LUAX_OVERLOAD_END
}

static int canvas_shift_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    GL_Surface_t *surface = self->surface;
    GL_surface_set_shifting(surface, NULL, NULL, 0);

    return 0;
}

static int canvas_shift_2ot_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    // idx #2: LUA_TTABLE

    GL_Pixel_t *from = NULL;
    GL_Pixel_t *to = NULL;

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        arrpush(from, (GL_Pixel_t)LUAX_INTEGER(L, -2));
        arrpush(to, (GL_Pixel_t)LUAX_INTEGER(L, -1));

        lua_pop(L, 1);
    }

    GL_Surface_t *surface = self->surface;
    GL_surface_set_shifting(surface, from, to, arrlen(from));

    arrfree(from);
    arrfree(to);

    return 0;
}

static int canvas_shift_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    GL_Pixel_t from = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    GL_Pixel_t to = (GL_Pixel_t)LUAX_INTEGER(L, 3);

    GL_Surface_t *surface = self->surface;
    GL_surface_set_shifting(surface, &from, &to, 1);

    return 0;
}

static int canvas_shift_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, canvas_shift_1o_0)
        LUAX_OVERLOAD_ARITY(2, canvas_shift_2ot_0)
        LUAX_OVERLOAD_ARITY(3, canvas_shift_3onn_0)
    LUAX_OVERLOAD_END
}

static int canvas_transparent_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    GL_Surface_t *surface = self->surface;
    GL_surface_set_transparent(surface, NULL, NULL, 0);

    return 0;
}

static int canvas_transparent_2ot_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    // idx #2: LUA_TTABLE

    GL_Pixel_t *indexes = NULL;
    GL_Bool_t *transparent = NULL;

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        arrpush(indexes, (GL_Pixel_t)LUAX_INTEGER(L, -2));
        arrpush(transparent, LUAX_BOOLEAN(L, -1) ? GL_BOOL_TRUE : GL_BOOL_FALSE);

        lua_pop(L, 1);
    }

    GL_Surface_t *surface = self->surface;
    GL_surface_set_transparent(surface, indexes, transparent, arrlen(indexes));

    arrfree(indexes);
    arrfree(transparent);

    return 0;
}

static int canvas_transparent_3onb_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    GL_Bool_t transparent = LUAX_BOOLEAN(L, 3) ? GL_BOOL_TRUE : GL_BOOL_FALSE;

    GL_Surface_t *surface = self->surface;
    GL_surface_set_transparent(surface, &index, &transparent, 1);

    return 0;
}

static int canvas_transparent_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, canvas_transparent_1o_0)
        LUAX_OVERLOAD_ARITY(2, canvas_transparent_2ot_0)
        LUAX_OVERLOAD_ARITY(3, canvas_transparent_3onb_0)
    LUAX_OVERLOAD_END
}

static int canvas_clear_2oN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 2, self->color.background);

    const GL_Surface_t *surface = self->surface;
    GL_surface_clear(surface, index);

    return 0;
}

static int canvas_point_4onnN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 4, self->color.foreground);

    const GL_Surface_t *surface = self->surface;
    GL_surface_point(surface, (GL_Point_t){ .x = x, .y = y }, index);

    return 0;
}

static int canvas_hline_5onnnN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    size_t width = (size_t)LUAX_INTEGER(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 5, self->color.foreground);

    const GL_Surface_t *surface = self->surface;
    GL_surface_hline(surface, (GL_Point_t){ .x = x, .y = y }, width, index);

    return 0;
}

static int canvas_vline_5onnnN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    size_t height = (size_t)LUAX_INTEGER(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 5, self->color.foreground);

    const GL_Surface_t *surface = self->surface;
    GL_surface_vline(surface, (GL_Point_t){ .x = x, .y = y }, height, index);

    return 0;
}

static int canvas_line_6onnnnN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x0 = LUAX_INTEGER(L, 2);
    int y0 = LUAX_INTEGER(L, 3);
    int x1 = LUAX_INTEGER(L, 4);
    int y1 = LUAX_INTEGER(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 6, self->color.foreground);

    const GL_Surface_t *surface = self->surface;
    GL_surface_polyline(surface, (GL_Point_t[]){
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

static int canvas_polyline_3otN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    // idx #2: LUA_TTABLE
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 3, self->color.foreground);

    GL_Point_t *vertices = _fetch(L, 2);

    size_t count = arrlen(vertices);
    if (count > 1) {
        const GL_Surface_t *surface = self->surface;
        GL_surface_polyline(surface, vertices, count, index);
    } else {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "polyline requires al least 2 points (provided %d)", count);
    }

    arrfree(vertices);

    return 0;
}

static int canvas_fill_4onnN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 4, self->color.foreground);

    const GL_Surface_t *surface = self->surface;
    GL_surface_fill(surface, (GL_Point_t){ .x = x, .y = y }, index); // TODO: pass `GL_INDEX_COLOR` fake?

    return 0;
}

static int canvas_triangle_9osnnnnnnN_0(lua_State *L)
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
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const char *mode = LUAX_STRING(L, 2);
    int x0 = LUAX_INTEGER(L, 3);
    int y0 = LUAX_INTEGER(L, 4);
    int x1 = LUAX_INTEGER(L, 5);
    int y1 = LUAX_INTEGER(L, 6);
    int x2 = LUAX_INTEGER(L, 7);
    int y2 = LUAX_INTEGER(L, 8);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 9, self->color.foreground);

    const GL_Surface_t *surface = self->surface;
    if (mode[0] == 'f') {
        GL_surface_filled_triangle(surface, (GL_Point_t){ .x = x0, .y = y0 }, (GL_Point_t){ .x = x1, .y = y1 }, (GL_Point_t){ .x = x2, .y = y2 }, index);
    } else {
        GL_Point_t vertices[4] = {
                (GL_Point_t){ .x = x0, .y = y0 },
                (GL_Point_t){ .x = x1, .y = y1 },
                (GL_Point_t){ .x = x2, .y = y2 },
                (GL_Point_t){ .x = x0, .y = y0 }
            };
        GL_surface_polyline(surface, vertices, 4, index);
    }

    return 0;
}

static int canvas_rectangle_7osnnnnN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const char *mode = LUAX_STRING(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    size_t width = (size_t)LUAX_INTEGER(L, 5);
    size_t height = (size_t)LUAX_INTEGER(L, 6);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 7, self->color.foreground);

    const GL_Surface_t *surface = self->surface;
    if (mode[0] == 'f') {
        GL_surface_filled_rectangle(surface, (GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height }, index);
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
        GL_surface_polyline(surface, vertices, 5, index);
    }

    return 0;
}

static int canvas_circle_6osnnnN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const char *mode = LUAX_STRING(L, 2);
    int cx = LUAX_INTEGER(L, 3);
    int cy = LUAX_INTEGER(L, 4);
    size_t radius = (size_t)LUAX_INTEGER(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_OPTIONAL_INTEGER(L, 6, self->color.foreground);

    const GL_Surface_t *surface = self->surface;
    if (radius < 1) { // Null radius, just a point regardless mode!
        GL_surface_point(surface, (GL_Point_t){ .x = cx, .y = cy }, index);
    } else
    if (mode[0] == 'f') {
        GL_surface_filled_circle(surface, (GL_Point_t){ .x = cx, .y = cy }, radius, index);
    } else {
        GL_surface_circle(surface, (GL_Point_t){ .x = cx, .y = cy }, radius, index);
    }

    return 0;
}

static int canvas_peek_3onn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);

    const GL_Surface_t *surface = self->surface;
    const GL_Pixel_t index = GL_surface_peek(surface, (GL_Point_t){ .x = x, .y = y });

    lua_pushinteger(L, (lua_Integer)index);

    return 1;
}

static int canvas_poke_4onnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 4);

    const GL_Surface_t *surface = self->surface;
    GL_surface_poke(surface, (GL_Point_t){ .x = x, .y = y }, index);

    return 0;
}

typedef struct _Process_Closure_t {
    const Interpreter_t *interpreter;
    lua_State *L;
    int index;
} Process_Closure_t;

// FIXME: use `lua_ref()` to optimize.
static GL_Pixel_t _process_callback(void *user_data, GL_Point_t position, GL_Pixel_t from, GL_Pixel_t to)
{
    Process_Closure_t *closure = (Process_Closure_t *)user_data;

    lua_pushvalue(closure->L, closure->index); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed, in the meanwhile)
    lua_pushinteger(closure->L, (lua_Integer)position.x);
    lua_pushinteger(closure->L, (lua_Integer)position.y);
    lua_pushinteger(closure->L, (lua_Integer)from);
    lua_pushinteger(closure->L, (lua_Integer)to);
    Interpreter_call(closure->interpreter, 4, 1);

    GL_Pixel_t pixel = (GL_Pixel_t)LUAX_INTEGER(closure->L, -1);

    lua_pop(closure->L, 1);

    return pixel;
}

static int canvas_process_3oof_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
//    luaX_Reference callback = luaX_tofunction(L, 3);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    GL_surface_process(surface, (GL_Point_t){ .x = 0, .y = 0 },
        source, (GL_Rectangle_t){ .x = 0, .y = 0, .width = source->width, .height = source->height },
        _process_callback, &(Process_Closure_t){ .interpreter = interpreter, .L = L, .index = 3 });

    return 0;
}

static int canvas_process_5onnof_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_CANVAS);
//    luaX_Reference callback = luaX_tofunction(L, 5);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    GL_surface_process(surface, (GL_Point_t){ .x = x, .y = y },
        source, (GL_Rectangle_t){ .x = 0, .y = 0, .width = source->width, .height = source->height },
        _process_callback, &(Process_Closure_t){ .interpreter = interpreter, .L = L, .index = 5 });

    return 0;
}

static int canvas_process_9onnonnnnf_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_CANVAS);
    int ox = LUAX_INTEGER(L, 5);
    int oy = LUAX_INTEGER(L, 6);
    size_t width = (size_t)LUAX_INTEGER(L, 7);
    size_t height = (size_t)LUAX_INTEGER(L, 8);
//    luaX_Reference callback = luaX_tofunction(L, 9);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    GL_surface_process(surface, (GL_Point_t){ .x = x, .y = y },
        source, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height },
        _process_callback, &(Process_Closure_t){ .interpreter = interpreter, .L = L, .index = 9 });

    return 0;
}

static int canvas_process_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, canvas_process_3oof_0)
        LUAX_OVERLOAD_ARITY(5, canvas_process_5onnof_0)
        LUAX_OVERLOAD_ARITY(9, canvas_process_9onnonnnnf_0)
    LUAX_OVERLOAD_END
}

static int canvas_copy_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    GL_surface_copy(surface, (GL_Point_t){ .x = 0, .y = 0 },
        source, (GL_Rectangle_t){ .x = 0, .y = 0, .width = source->width, .height = source->height });

    return 0;
}

static int canvas_copy_4onno_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_CANVAS);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    GL_surface_copy(surface, (GL_Point_t){ .x = x, .y = y },
        source, (GL_Rectangle_t){ .x = 0, .y = 0, .width = source->width, .height = source->height });

    return 0;
}

static int canvas_copy_8onnonnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_CANVAS);
    int ox = LUAX_INTEGER(L, 5);
    int oy = LUAX_INTEGER(L, 6);
    size_t width = (size_t)LUAX_INTEGER(L, 7);
    size_t height = (size_t)LUAX_INTEGER(L, 8);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    GL_surface_copy(surface, (GL_Point_t){ .x = x, .y = y },
        source, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height });

    return 0;
}

static int canvas_copy_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, canvas_copy_2oo_0)
        LUAX_OVERLOAD_ARITY(4, canvas_copy_4onno_0)
        LUAX_OVERLOAD_ARITY(8, canvas_copy_8onnonnnn_0)
    LUAX_OVERLOAD_END
}

static int canvas_blit_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    GL_surface_blit(surface, (GL_Point_t){ .x = 0, .y = 0 },
        source, (GL_Rectangle_t){ .x = 0, .y = 0, .width = source->width, .height = source->height });

    return 0;
}

static int canvas_blit_4onno_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_CANVAS);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    GL_surface_blit(surface, (GL_Point_t){ .x = x, .y = y },
        source, (GL_Rectangle_t){ .x = 0, .y = 0, .width = source->width, .height = source->height });

    return 0;
}

static int canvas_blit_8onnonnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_CANVAS);
    int ox = LUAX_INTEGER(L, 5);
    int oy = LUAX_INTEGER(L, 6);
    size_t width = (size_t)LUAX_INTEGER(L, 7);
    size_t height = (size_t)LUAX_INTEGER(L, 8);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    GL_surface_blit(surface, (GL_Point_t){ .x = x, .y = y },
        source, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height });

    return 0;
}

static int canvas_blit_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, canvas_blit_2oo_0)
        LUAX_OVERLOAD_ARITY(4, canvas_blit_4onno_0)
        LUAX_OVERLOAD_ARITY(8, canvas_blit_8onnonnnn_0)
    LUAX_OVERLOAD_END
}

static const Map_Entry_t _comparators[GL_Comparators_t_CountOf] = { // Need to be sorted for `bsearch()`
    { "always", GL_COMPARATOR_ALWAYS },
    { "equal", GL_COMPARATOR_EQUAL },
    { "greater", GL_COMPARATOR_GREATER },
    { "greater-or-equal", GL_COMPARATOR_GREATER_OR_EQUAL },
    { "less", GL_COMPARATOR_LESS },
    { "less-or-equal", GL_COMPARATOR_LESS_OR_EQUAL },
    { "never", GL_COMPARATOR_NEVER },
    { "not-equal", GL_COMPARATOR_NOT_EQUAL }
};

static int canvas_stencil_5ooosn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *mask = (const Canvas_Object_t *)LUAX_OBJECT(L, 3, OBJECT_TYPE_CANVAS);
    const char *comparator = LUAX_STRING(L, 4);
    GL_Pixel_t threshold = (GL_Pixel_t)LUAX_INTEGER(L, 5);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    const GL_Surface_t *mask_surface = mask->surface;
    const Map_Entry_t *entry = map_find(L, comparator, _comparators, GL_Comparators_t_CountOf);
    GL_surface_stencil(surface, (GL_Point_t){ .x = 0, .y = 0 },
        source, (GL_Rectangle_t){ .x = 0, .y = 0, .width = source->width, .height = source->height },
        mask_surface, (GL_Comparators_t)entry->value, threshold);

    return 0;
}

static int canvas_stencil_7onnoosn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *mask = (const Canvas_Object_t *)LUAX_OBJECT(L, 5, OBJECT_TYPE_CANVAS);
    const char *comparator = LUAX_STRING(L, 6);
    GL_Pixel_t threshold = (GL_Pixel_t)LUAX_INTEGER(L, 7);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    const GL_Surface_t *mask_surface = mask->surface;
    const Map_Entry_t *entry = map_find(L, comparator, _comparators, GL_Comparators_t_CountOf);
    GL_surface_stencil(surface, (GL_Point_t){ .x = x, .y = y },
        source, (GL_Rectangle_t){ .x = 0, .y = 0, .width = source->width, .height = source->height },
        mask_surface, (GL_Comparators_t)entry->value, threshold);

    return 0;
}

static int canvas_stencil_11onnonnnnosn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_CANVAS);
    int ox = LUAX_INTEGER(L, 5);
    int oy = LUAX_INTEGER(L, 6);
    size_t width = (size_t)LUAX_INTEGER(L, 7);
    size_t height = (size_t)LUAX_INTEGER(L, 8);
    const Canvas_Object_t *mask = (const Canvas_Object_t *)LUAX_OBJECT(L, 9, OBJECT_TYPE_CANVAS);
    const char *comparator = LUAX_STRING(L, 10);
    GL_Pixel_t threshold = (GL_Pixel_t)LUAX_INTEGER(L, 11);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    const GL_Surface_t *mask_surface = mask->surface;
    const Map_Entry_t *entry = map_find(L, comparator, _comparators, GL_Comparators_t_CountOf);
    GL_surface_stencil(surface, (GL_Point_t){ .x = x, .y = y },
        source, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height  },
        mask_surface, (GL_Comparators_t)entry->value, threshold);

    return 0;
}

static int canvas_stencil_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(5, canvas_stencil_5ooosn_0)
        LUAX_OVERLOAD_ARITY(7, canvas_stencil_7onnoosn_0)
        LUAX_OVERLOAD_ARITY(11, canvas_stencil_11onnonnnnosn_0)
    LUAX_OVERLOAD_END
}

static const Map_Entry_t _functions[GL_Functions_t_CountOf] = { // Need to be sorted for `bsearch()`
    { "add", GL_FUNCTIONS_ADD },
    { "add-clamped", GL_FUNCTIONS_ADD_CLAMPED },
    { "max", GL_FUNCTIONS_MAX },
    { "min", GL_FUNCTIONS_MIN },
    { "multiply", GL_FUNCTIONS_MULTIPLY },
    { "multiply-clamped", GL_FUNCTIONS_MULTIPLY_CLAMPED },
    { "replace", GL_FUNCTIONS_REPLACE },
    { "reverse-subtract", GL_FUNCTIONS_REVERSE_SUBTRACT },
    { "reverse-subtract-clamped", GL_FUNCTIONS_REVERSE_SUBTRACT_CLAMPED },
    { "subtract", GL_FUNCTIONS_SUBTRACT },
    { "subtract-clamped", GL_FUNCTIONS_SUBTRACT_CLAMPED }
};

static int canvas_blend_3oos_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const char *function = LUAX_STRING(L, 2);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 3, OBJECT_TYPE_CANVAS);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    const Map_Entry_t *entry = map_find(L, function, _functions, GL_Functions_t_CountOf);
    GL_surface_blend(surface, (GL_Point_t){ .x = 0, .y = 0 },
        source, (GL_Rectangle_t){ .x = 0, .y = 0, .width = source->width, .height = source->height },
        (GL_Functions_t)entry->value);

    return 0;
}

static int canvas_blend_5onnos_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_CANVAS);
    const char *function = LUAX_STRING(L, 5);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    const Map_Entry_t *entry = map_find(L, function, _functions, GL_Functions_t_CountOf);
    GL_surface_blend(surface, (GL_Point_t){ .x = x, .y = y },
        source, (GL_Rectangle_t){ .x = 0, .y = 0, .width = source->width, .height = source->height },
        (GL_Functions_t)entry->value);

    return 0;
}

static int canvas_blend_9onnonnnns_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_CANVAS);
    int ox = LUAX_INTEGER(L, 5);
    int oy = LUAX_INTEGER(L, 6);
    size_t width = (size_t)LUAX_INTEGER(L, 7);
    size_t height = (size_t)LUAX_INTEGER(L, 8);
    const char *function = LUAX_STRING(L, 9);

    const GL_Surface_t *surface = self->surface;
    const GL_Surface_t *source = canvas->surface;
    const Map_Entry_t *entry = map_find(L, function, _functions, GL_Functions_t_CountOf);
    GL_surface_blend(surface, (GL_Point_t){ .x = x, .y = y },
        source, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height },
        (GL_Functions_t)entry->value);

    return 0;
}

static int canvas_blend_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, canvas_blend_3oos_0)
        LUAX_OVERLOAD_ARITY(5, canvas_blend_5onnos_0)
        LUAX_OVERLOAD_ARITY(9, canvas_blend_9onnonnnns_0)
    LUAX_OVERLOAD_END
}
