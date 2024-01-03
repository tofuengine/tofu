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

#include "internal/udt.h"

#include <core/config.h>
#define _LOG_TAG "canvas"
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>
#include <systems/display.h>
#include <systems/interpreter.h>

#define MODULE_NAME "tofu.graphics.canvas"
#define META_TABLE  "Tofu_Graphics_Canvas_mt"
// FIXME: collapse meta and script name? or desume the meta-table name from the module and try and load always?

static int canvas_new_1o_1o(lua_State *L);
static int canvas_gc_1o_0(lua_State *L);
static int canvas_image_1o_1o(lua_State *L);
static int canvas_push_1o_0(lua_State *L);
static int canvas_pop_2oN_0(lua_State *L);
static int canvas_reset_1o_0(lua_State *L);
static int canvas_clipping_v_0(lua_State *L);
static int canvas_shift_v_0(lua_State *L);
static int canvas_transparent_v_0(lua_State *L);
static int canvas_clear_2onB_0(lua_State *L);
static int canvas_fill_4onnnB_0(lua_State *L);
static int canvas_point_4onnn_0(lua_State *L);
static int canvas_hline_5onnnn_0(lua_State *L);
static int canvas_vline_5onnnn_0(lua_State *L);
static int canvas_line_6onnnnn_0(lua_State *L);
// static int canvas_tline_6onnnnonnnn_0(lua_State *L);
static int canvas_polyline_3otn_0(lua_State *L);
static int canvas_triangle_9osnnnnnnn_0(lua_State *L);
static int canvas_rectangle_7osnnnnn_0(lua_State *L);
static int canvas_circle_6osnnnn_0(lua_State *L);
static int canvas_scan_v_0(lua_State *L);
static int canvas_process_v_0(lua_State *L);
static int canvas_copy_v_0(lua_State *L);
static int canvas_blit_v_0(lua_State *L);
static int canvas_xform_v_0(lua_State *L);
static int canvas_stencil_v_0(lua_State *L);
static int canvas_blend_v_0(lua_State *L);
static int canvas_sprite_v_0(lua_State *L);
static int canvas_tile_v_0(lua_State *L);
static int canvas_flush_3ooS_0(lua_State *L);
static int canvas_text_v_2nn(lua_State *L);

// TODO: rename `Canvas` to `Context`?

int canvas_loader(lua_State *L)
{
    char name[PLATFORM_PATH_MAX] = { 0 };
    const char *file = path_lua_to_fs(name, MODULE_NAME);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));
    Storage_Resource_t *script = Storage_load(storage, file, STORAGE_RESOURCE_STRING);

    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){
            .data = S_SCHARS(script),
            .size = S_SLENTGH(script),
            .name = name
        },
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", canvas_new_1o_1o },
            { "__gc", canvas_gc_1o_0 },
            // -- accessors --
            { "image", canvas_image_1o_1o },
            // -- mutators --
            { "push", canvas_push_1o_0 },
            { "pop", canvas_pop_2oN_0 },
            { "reset", canvas_reset_1o_0 },
            { "clipping", canvas_clipping_v_0 },
            { "shift", canvas_shift_v_0 },
            { "transparent", canvas_transparent_v_0 },
            // -- operations --
            { "clear", canvas_clear_2onB_0 }, // canvas only
            { "fill", canvas_fill_4onnnB_0 },
            { "point", canvas_point_4onnn_0 }, // primitives
            { "hline", canvas_hline_5onnnn_0 },
            { "vline", canvas_vline_5onnnn_0 },
            { "line", canvas_line_6onnnnn_0 },
            { "polyline", canvas_polyline_3otn_0 },
            { "triangle", canvas_triangle_9osnnnnnnn_0 },
            { "rectangle", canvas_rectangle_7osnnnnn_0 },
            { "circle", canvas_circle_6osnnnn_0 },
            { "scan", canvas_scan_v_0 }, // in-place
            { "process", canvas_process_v_0 },
            { "copy", canvas_copy_v_0 }, // canvas-to-canvas
            { "blit", canvas_blit_v_0 },
            { "xform", canvas_xform_v_0 },
            { "stencil", canvas_stencil_v_0 },
            { "blend", canvas_blend_v_0 },
            { "sprite", canvas_sprite_v_0 }, // bank-to-canvas
            { "tile", canvas_tile_v_0 },
            { "flush", canvas_flush_3ooS_0 }, // batch-to-canvas
            { "text", canvas_text_v_2nn }, // font-to-canvas
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int canvas_new_1o_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);

    GL_Context_t *context = GL_context_create(image->surface);
    if (!context) {
        return luaL_error(L, "can't create context");
    }

    Canvas_Object_t *self = (Canvas_Object_t *)luaX_newobject(L, sizeof(Canvas_Object_t), &(Canvas_Object_t){
            .context = context,
            .image = {
                .instance = image,
                .reference = luaX_ref(L, 1)
            }
        }, OBJECT_TYPE_CANVAS, META_TABLE);

    LOG_D("canvas %p allocated w/ context %p for image %p", self, context, image);

    return 1;
}

static int canvas_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    luaX_unref(L, self->image.reference);
    LOG_D("image reference #%d released", self->image.reference);

    GL_context_destroy(self->context);
    LOG_D("context %p destroyed", self->context);

    LOG_D("canvas %p finalized", self);

    return 0;
}

static int canvas_image_1o_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    luaX_pushref(L, self->image.reference); // Push the actual Lua object/userdata, from the reference!

    return 1;
}

static int canvas_push_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    GL_context_push(self->context);

    return 0;
}

static int canvas_pop_2oN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    size_t levels = LUAX_OPTIONAL_UNSIGNED(L, 2, 1);

    GL_context_pop(self->context, levels > 0 ? levels : SIZE_MAX);

    return 0;
}

static int canvas_reset_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    GL_context_reset(self->context);

    return 0;
}

static int canvas_clipping_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    GL_context_set_clipping(self->context, NULL);

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
    size_t width = LUAX_UNSIGNED(L, 4);
    size_t height = LUAX_UNSIGNED(L, 5);

    GL_context_set_clipping(self->context, &(GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height });

    return 0;
}

static int canvas_clipping_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_clipping_1o_0, 1)
        LUAX_OVERLOAD_BY_ARITY(canvas_clipping_5onnnn_0, 5)
    LUAX_OVERLOAD_END
}

static int canvas_shift_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    GL_context_set_shifting(self->context, NULL, NULL, 0);

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
        arrpush(from, (GL_Pixel_t)LUAX_UNSIGNED(L, -2));
        arrpush(to, (GL_Pixel_t)LUAX_UNSIGNED(L, -1));

        lua_pop(L, 1);
    }

    GL_context_set_shifting(self->context, from, to, arrlenu(from));

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
    GL_Pixel_t from = (GL_Pixel_t)LUAX_UNSIGNED(L, 2);
    GL_Pixel_t to = (GL_Pixel_t)LUAX_UNSIGNED(L, 3);

    GL_context_set_shifting(self->context, &from, &to, 1);

    return 0;
}

static int canvas_shift_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_shift_1o_0, 1)
        LUAX_OVERLOAD_BY_ARITY(canvas_shift_2ot_0, 2)
        LUAX_OVERLOAD_BY_ARITY(canvas_shift_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static int canvas_transparent_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    GL_context_set_transparent(self->context, NULL, NULL, 0);

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
        arrpush(indexes, (GL_Pixel_t)LUAX_UNSIGNED(L, -2));
        arrpush(transparent, LUAX_BOOLEAN(L, -1) ? GL_BOOL_TRUE : GL_BOOL_FALSE);

        lua_pop(L, 1);
    }

    GL_context_set_transparent(self->context, indexes, transparent, arrlenu(indexes));

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
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 2);
    GL_Bool_t transparent = LUAX_BOOLEAN(L, 3) ? GL_BOOL_TRUE : GL_BOOL_FALSE;

    GL_context_set_transparent(self->context, &index, &transparent, 1);

    return 0;
}

static int canvas_transparent_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_transparent_1o_0, 1)
        LUAX_OVERLOAD_BY_ARITY(canvas_transparent_2ot_0, 2)
        LUAX_OVERLOAD_BY_ARITY(canvas_transparent_3onb_0, 3)
    LUAX_OVERLOAD_END
}

static int canvas_clear_2onB_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 2);
    bool transparency = LUAX_OPTIONAL_BOOLEAN(L, 3, false);

    GL_context_clear(self->context, index, transparency);

    return 0;
}

static int canvas_fill_4onnnB_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 4);
    bool transparency = LUAX_OPTIONAL_BOOLEAN(L, 5, false);

    GL_context_fill(self->context, (GL_Point_t){ .x = x, .y = y }, index, transparency); // TODO: pass `GL_INDEX_COLOR` fake?

    return 0;
}

static int canvas_point_4onnn_0(lua_State *L)
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
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 4);

    GL_context_point(self->context, (GL_Point_t){ .x = x, .y = y }, index);

    return 0;
}

static int canvas_hline_5onnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    size_t width = LUAX_UNSIGNED(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 5);

    GL_context_hline(self->context, (GL_Point_t){ .x = x, .y = y }, width, index);

    return 0;
}

static int canvas_vline_5onnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    size_t height = LUAX_UNSIGNED(L, 4);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 5);

    GL_context_vline(self->context, (GL_Point_t){ .x = x, .y = y }, height, index);

    return 0;
}

static int canvas_line_6onnnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x0 = LUAX_INTEGER(L, 2);
    int y0 = LUAX_INTEGER(L, 3);
    int x1 = LUAX_INTEGER(L, 4);
    int y1 = LUAX_INTEGER(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 6);

    GL_context_polyline(self->context, (GL_Point_t[]){
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

static int canvas_polyline_3otn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    // idx #2: LUA_TTABLE
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 3);

    GL_Point_t *vertices = _fetch(L, 2);

    size_t count = arrlenu(vertices);
    if (count < 2) {
        // TODO: free vertices!
        return luaL_error(L, "polyline requires al least 2 points (provided %d)", count);
    }

    GL_context_polyline(self->context, vertices, count, index);

    arrfree(vertices);

    return 0;
}

static int canvas_triangle_9osnnnnnnn_0(lua_State *L)
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
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const char *mode = LUAX_STRING(L, 2);
    int x0 = LUAX_INTEGER(L, 3);
    int y0 = LUAX_INTEGER(L, 4);
    int x1 = LUAX_INTEGER(L, 5);
    int y1 = LUAX_INTEGER(L, 6);
    int x2 = LUAX_INTEGER(L, 7);
    int y2 = LUAX_INTEGER(L, 8);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 9);

    if (mode[0] == 'f') {
        GL_context_filled_triangle(self->context,
            (GL_Point_t){ .x = x0, .y = y0 }, (GL_Point_t){ .x = x1, .y = y1 }, (GL_Point_t){ .x = x2, .y = y2 },
            index);
    } else {
        GL_context_polyline(self->context, (GL_Point_t[4]){
                (GL_Point_t){ .x = x0, .y = y0 },
                (GL_Point_t){ .x = x1, .y = y1 },
                (GL_Point_t){ .x = x2, .y = y2 },
                (GL_Point_t){ .x = x0, .y = y0 }
            }, 4, index);
    }

    return 0;
}

static int canvas_rectangle_7osnnnnn_0(lua_State *L)
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
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const char *mode = LUAX_STRING(L, 2); // FIXME: move `mode` as last optional argument.
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    size_t width = LUAX_UNSIGNED(L, 5);
    size_t height = LUAX_UNSIGNED(L, 6);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 7);

    if (mode[0] == 'f') {
        GL_context_filled_rectangle(self->context, (GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height }, index);
    } else {
        const int x0 = x;
        const int y0 = y;
        const int x1 = x0 + (int)width - 1;
        const int y1 = y0 + (int)height - 1;

        GL_context_polyline(self->context, (GL_Point_t[5]){
                (GL_Point_t){ .x = x0, .y = y0 },
                (GL_Point_t){ .x = x0, .y = y1 },
                (GL_Point_t){ .x = x1, .y = y1 },
                (GL_Point_t){ .x = x1, .y = y0 },
                (GL_Point_t){ .x = x0, .y = y0 }
            }, 5, index);
    }

    return 0;
}

static int canvas_circle_6osnnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const char *mode = LUAX_STRING(L, 2);
    int cx = LUAX_INTEGER(L, 3);
    int cy = LUAX_INTEGER(L, 4);
    size_t radius = LUAX_UNSIGNED(L, 5);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 6);

    if (radius < 1) { // Null radius, just a point regardless mode!
        GL_context_point(self->context, (GL_Point_t){ .x = cx, .y = cy }, index);
    } else
    if (mode[0] == 'f') {
        GL_context_filled_circle(self->context, (GL_Point_t){ .x = cx, .y = cy }, radius, index);
    } else {
        GL_context_circle(self->context, (GL_Point_t){ .x = cx, .y = cy }, radius, index);
    }

    return 0;
}

typedef struct Canvas_Scan_Closure_s {
    const Interpreter_t *interpreter;
    lua_State *L;
    int index;
} Canvas_Scan_Closure_t;

// FIXME: use `lua_ref()` to optimize.
static GL_Pixel_t _scan_callback(void *user_data, GL_Point_t position, GL_Pixel_t index)
{
    const Canvas_Scan_Closure_t *closure = (const Canvas_Scan_Closure_t *)user_data;

    lua_pushvalue(closure->L, closure->index); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed, in the meanwhile)
    lua_pushinteger(closure->L, (lua_Integer)position.x);
    lua_pushinteger(closure->L, (lua_Integer)position.y);
    lua_pushinteger(closure->L, (lua_Integer)index);
    Interpreter_call(closure->interpreter, 3, 1);

    GL_Pixel_t pixel = (GL_Pixel_t)LUAX_UNSIGNED(closure->L, -1);

    lua_pop(closure->L, 1);

    return pixel;
}

static int canvas_scan_6ofNNNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    // idx #2: LUA_TFUNCTION
    int x = LUAX_OPTIONAL_INTEGER(L, 3, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 4, 0);
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 5, self->context->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 6, self->context->surface->height);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    GL_context_scan(self->context, (GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height },
        _scan_callback, &(Canvas_Scan_Closure_t){ .interpreter = interpreter, .L = L, .index = 2 });

    return 0;
}

static int canvas_scan_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_scan_6ofNNNN_0, 2)
        LUAX_OVERLOAD_BY_ARITY(canvas_scan_6ofNNNN_0, 6)
    LUAX_OVERLOAD_END
}

typedef struct Canvas_Process_Closure_s {
    const Interpreter_t *interpreter;
    lua_State *L;
    int index;
} Canvas_Process_Closure_t;

// FIXME: use `lua_ref()` to optimize.
static GL_Pixel_t _process_callback(void *user_data, GL_Point_t position, GL_Pixel_t from, GL_Pixel_t to)
{
    const Canvas_Process_Closure_t *closure = (const Canvas_Process_Closure_t *)user_data;

    lua_pushvalue(closure->L, closure->index); // Copy directly from stack argument, don't need to ref/unref (won't be GC-ed, in the meanwhile)
    lua_pushinteger(closure->L, (lua_Integer)position.x);
    lua_pushinteger(closure->L, (lua_Integer)position.y);
    lua_pushinteger(closure->L, (lua_Integer)from);
    lua_pushinteger(closure->L, (lua_Integer)to);
    Interpreter_call(closure->interpreter, 4, 1);

    GL_Pixel_t pixel = (GL_Pixel_t)LUAX_UNSIGNED(closure->L, -1);

    lua_pop(closure->L, 1);

    return pixel;
}

static int canvas_process_9oofNNNNNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TFUNCTION)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_IMAGE);
    // idx #3: LUA_TFUNCTION
    int x = LUAX_OPTIONAL_INTEGER(L, 4, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 5, 0);
    int ox = LUAX_OPTIONAL_INTEGER(L, 6, 0);
    int oy = LUAX_OPTIONAL_INTEGER(L, 7, 0);
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 8, image->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 9, image->surface->height);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    GL_context_process(self->context, (GL_Point_t){ .x = x, .y = y },
        image->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height },
        _process_callback, &(Canvas_Process_Closure_t){ .interpreter = interpreter, .L = L, .index = 3 });

    return 0;
}

static int canvas_process_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_process_9oofNNNNNN_0, 3)
        LUAX_OVERLOAD_BY_ARITY(canvas_process_9oofNNNNNN_0, 5)
        LUAX_OVERLOAD_BY_ARITY(canvas_process_9oofNNNNNN_0, 9)
    LUAX_OVERLOAD_END
}

static int canvas_copy_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_IMAGE);

    GL_context_copy(self->context, (GL_Point_t){ .x = 0, .y = 0 },
        image->surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = image->surface->width, .height = image->surface->height });

    return 0;
}

static int canvas_copy_8onnoNNNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_IMAGE);
    int ox = LUAX_OPTIONAL_INTEGER(L, 5, 0);
    int oy = LUAX_OPTIONAL_INTEGER(L, 6, 0);
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 7, image->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 8, image->surface->height);

    GL_context_copy(self->context, (GL_Point_t){ .x = x, .y = y },
        image->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height });

    return 0;
}

static int canvas_copy_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_copy_2oo_0, 2)
        LUAX_OVERLOAD_BY_ARITY(canvas_copy_8onnoNNNN_0, 4)
        LUAX_OVERLOAD_BY_ARITY(canvas_copy_8onnoNNNN_0, 8)
    LUAX_OVERLOAD_END
}

static int canvas_blit_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_IMAGE);

    GL_context_blit(self->context, (GL_Point_t){ .x = 0, .y = 0 },
        image->surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = image->surface->width, .height = image->surface->height });

    return 0;
}

static int canvas_blit_8onnoNNNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_IMAGE);
    int ox = LUAX_OPTIONAL_INTEGER(L, 5, 0);
    int oy = LUAX_OPTIONAL_INTEGER(L, 6, 0);
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 7, image->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 8, image->surface->height);

    GL_context_blit(self->context, (GL_Point_t){ .x = x, .y = y },
        image->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height });

    return 0;
}

static int canvas_blit_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_blit_2oo_0, 2)
        LUAX_OVERLOAD_BY_ARITY(canvas_blit_8onnoNNNN_0, 4)
        LUAX_OVERLOAD_BY_ARITY(canvas_blit_8onnoNNNN_0, 8)
    LUAX_OVERLOAD_END
}

static int canvas_xform_3ooo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_IMAGE);
    const XForm_Object_t *xform = (const XForm_Object_t *)LUAX_OBJECT(L, 3, OBJECT_TYPE_XFORM);

    // FIXME: rename `GL_xform_blit` to `GL_surface_xblit`?
    GL_xform_blit(xform->xform,
        self->context, (GL_Point_t){ .x = 0, .y = 0 },
        image->surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = image->surface->width, .height = image->surface->height });

    return 0;
}

static int canvas_xform_5onnoo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_IMAGE);
    const XForm_Object_t *xform = (const XForm_Object_t *)LUAX_OBJECT(L, 5, OBJECT_TYPE_XFORM);

    // FIXME: rename `GL_xform_blit` to `GL_surface_xblit`?
    GL_xform_blit(xform->xform,
        self->context, (GL_Point_t){ .x = x, .y = y },
        image->surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = image->surface->width, .height = image->surface->height });

    return 0;
}

static int canvas_xform_9onnonnnno_0(lua_State *L)
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
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_IMAGE);
    int ox = LUAX_INTEGER(L, 5);
    int oy = LUAX_INTEGER(L, 6);
    size_t width = LUAX_UNSIGNED(L, 7);
    size_t height = LUAX_UNSIGNED(L, 8);
    const XForm_Object_t *xform = (const XForm_Object_t *)LUAX_OBJECT(L, 9, OBJECT_TYPE_XFORM);

    // FIXME: rename `GL_xform_blit` to `GL_surface_xblit`?
    GL_xform_blit(xform->xform,
        self->context, (GL_Point_t){ .x = x, .y = y },
        image->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height });

    return 0;
}

static int canvas_xform_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_xform_3ooo_0, 3)
        LUAX_OVERLOAD_BY_ARITY(canvas_xform_5onnoo_0, 5)
        LUAX_OVERLOAD_BY_ARITY(canvas_xform_9onnonnnno_0, 9)
    LUAX_OVERLOAD_END
}

static const char *_comparators[GL_Comparators_t_CountOf + 1] = {
    "never",
    "less",
    "less-or-equal",
    "greater",
    "greater-or-equal",
    "equal",
    "not-equal",
    "always",
    NULL
};

static int canvas_stencil_5oooen_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_IMAGE);
    const Image_Object_t *mask = (const Image_Object_t *)LUAX_OBJECT(L, 3, OBJECT_TYPE_IMAGE);
    GL_Comparators_t comparator = (GL_Comparators_t)LUAX_ENUM(L, 4, _comparators);
    GL_Pixel_t threshold = (GL_Pixel_t)LUAX_UNSIGNED(L, 5);

    GL_context_stencil(self->context, (GL_Point_t){ .x = 0, .y = 0 },
        image->surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = image->surface->width, .height = image->surface->height  },
        mask->surface, comparator, threshold);

    return 0;
}

static int canvas_stencil_7onnooen_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_IMAGE);
    const Image_Object_t *mask = (const Image_Object_t *)LUAX_OBJECT(L, 5, OBJECT_TYPE_IMAGE);
    GL_Comparators_t comparator = (GL_Comparators_t)LUAX_ENUM(L, 6, _comparators);
    GL_Pixel_t threshold = (GL_Pixel_t)LUAX_UNSIGNED(L, 7);

    GL_context_stencil(self->context, (GL_Point_t){ .x = x, .y = y },
        image->surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = image->surface->width, .height = image->surface->height  },
        mask->surface, comparator, threshold);

    return 0;
}

static int canvas_stencil_11onnonnnnoen_0(lua_State *L)
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
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_IMAGE);
    int ox = LUAX_INTEGER(L, 5);
    int oy = LUAX_INTEGER(L, 6);
    size_t width = LUAX_UNSIGNED(L, 7);
    size_t height = LUAX_UNSIGNED(L, 8);
    const Image_Object_t *mask = (const Image_Object_t *)LUAX_OBJECT(L, 9, OBJECT_TYPE_IMAGE);
    GL_Comparators_t comparator = (GL_Comparators_t)LUAX_ENUM(L, 10, _comparators);
    GL_Pixel_t threshold = (GL_Pixel_t)LUAX_UNSIGNED(L, 11);

    GL_context_stencil(self->context, (GL_Point_t){ .x = x, .y = y },
        image->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height  },
        mask->surface, comparator, threshold);

    return 0;
}

static int canvas_stencil_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_stencil_5oooen_0, 5)
        LUAX_OVERLOAD_BY_ARITY(canvas_stencil_7onnooen_0, 7)
        LUAX_OVERLOAD_BY_ARITY(canvas_stencil_11onnonnnnoen_0, 11)
    LUAX_OVERLOAD_END
}

static const char *_functions[GL_Functions_t_CountOf + 1] = {
    "replace",
    "add",
    "add-clamped",
    "subtract",
    "subtract-clamped",
    "reverse-subtract",
    "reverse-subtract-clamped",
    "multiply",
    "multiply-clamped",
    "min",
    "max",
    NULL
};

static int canvas_blend_3ooe_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_IMAGE);
    GL_Functions_t function = (GL_Functions_t)LUAX_ENUM(L, 3, _functions);

    GL_context_blend(self->context, (GL_Point_t){ .x = 0, .y = 0 },
        image->surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = image->surface->width, .height = image->surface->height },
        function);

    return 0;
}

static int canvas_blend_5onnoe_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_IMAGE);
    GL_Functions_t function = (GL_Functions_t)LUAX_ENUM(L, 5, _functions);

    GL_context_blend(self->context, (GL_Point_t){ .x = x, .y = y },
        image->surface, (GL_Rectangle_t){ .x = 0, .y = 0, .width = image->surface->width, .height = image->surface->height },
        function);

    return 0;
}

static int canvas_blend_9onnonnnne_0(lua_State *L)
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
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_IMAGE);
    int ox = LUAX_INTEGER(L, 5);
    int oy = LUAX_INTEGER(L, 6);
    size_t width = LUAX_UNSIGNED(L, 7);
    size_t height = LUAX_UNSIGNED(L, 8);
    GL_Functions_t function = (GL_Functions_t)LUAX_ENUM(L, 9, _functions);

    GL_context_blend(self->context, (GL_Point_t){ .x = x, .y = y },
        image->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height },
        function);

    return 0;
}

static int canvas_blend_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_blend_3ooe_0, 3)
        LUAX_OVERLOAD_BY_ARITY(canvas_blend_5onnoe_0, 5)
        LUAX_OVERLOAD_BY_ARITY(canvas_blend_9onnonnnne_0, 9)
    LUAX_OVERLOAD_END
}

// FIXME: rename `surface` to source/target on arguments list where a surface appears twice.
static int canvas_sprite_5onnon_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Bank_Object_t *bank = (const Bank_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_BANK);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = bank->sheet;
    GL_sheet_blit(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id);

    return 0;
}

static int canvas_sprite_6onnonn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Bank_Object_t *bank = (const Bank_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_BANK);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);
    int rotation = LUAX_INTEGER(L, 6);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = bank->sheet;
    GL_sheet_blit_sr(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id, 1.0f, 1.0f, rotation, 0.5f, 0.5f);

    return 0;
}

static int canvas_sprite_7onnonnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Bank_Object_t *bank = (const Bank_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_BANK);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);
    float scale_x = LUAX_NUMBER(L, 6);
    float scale_y = LUAX_NUMBER(L, 7);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = bank->sheet;
    GL_sheet_blit_s(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id, scale_x, scale_y);

    return 0;
}

static int canvas_sprite_10onnonnnnNN_0(lua_State *L)
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
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Bank_Object_t *bank = (const Bank_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_BANK);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);
    float scale_x = LUAX_NUMBER(L, 6);
    float scale_y = LUAX_NUMBER(L, 7);
    int rotation = LUAX_INTEGER(L, 8);
    float anchor_x = LUAX_OPTIONAL_NUMBER(L, 9, 0.5f);
    float anchor_y = LUAX_OPTIONAL_NUMBER(L, 10, anchor_x);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = bank->sheet;
    GL_sheet_blit_sr(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id, scale_x, scale_y, rotation, anchor_x, anchor_y);

    return 0;
}

static int canvas_sprite_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_sprite_5onnon_0, 5)
        LUAX_OVERLOAD_BY_ARITY(canvas_sprite_6onnonn_0, 6)
        LUAX_OVERLOAD_BY_ARITY(canvas_sprite_7onnonnn_0, 7)
        LUAX_OVERLOAD_BY_ARITY(canvas_sprite_10onnonnnnNN_0, 8)
        LUAX_OVERLOAD_BY_ARITY(canvas_sprite_10onnonnnnNN_0, 9)
        LUAX_OVERLOAD_BY_ARITY(canvas_sprite_10onnonnnnNN_0, 10)
    LUAX_OVERLOAD_END
}

// FIXME: fix the consts!!!
static int canvas_tile_7onnonnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Bank_Object_t *bank = (const Bank_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_BANK);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);
    int offset_x = LUAX_INTEGER(L, 6);
    int offset_y = LUAX_INTEGER(L, 7);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = bank->sheet;
    GL_sheet_tile(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id, (GL_Point_t){ .x = offset_x, .y = offset_y });

    return 0;
}

static int canvas_tile_9onnonnnnN_0(lua_State *L)
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
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Bank_Object_t *bank = (const Bank_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_BANK);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 5);
    int offset_x = LUAX_INTEGER(L, 6);
    int offset_y = LUAX_INTEGER(L, 7);
    int scale_x = LUAX_INTEGER(L, 8);
    int scale_y = LUAX_OPTIONAL_INTEGER(L, 9, scale_x);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = bank->sheet;
    GL_sheet_tile_s(sheet, context, (GL_Point_t){ .x = x, .y = y },
        cell_id, (GL_Point_t){ .x = offset_x, .y = offset_y }, scale_x, scale_y);

    return 0;
}

static int canvas_tile_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_tile_7onnonnn_0, 7)
        LUAX_OVERLOAD_BY_ARITY(canvas_tile_9onnonnnnN_0, 8)
        LUAX_OVERLOAD_BY_ARITY(canvas_tile_9onnonnnnN_0, 9)
    LUAX_OVERLOAD_END
}

// TODO: are sprite-batches useful? Profile it...
static int canvas_flush_3ooS_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Batch_Object_t *batch = (const Batch_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_BATCH);
    const char *mode = LUAX_OPTIONAL_STRING(L, 3, "fast");

    const GL_Queue_t *queue = batch->queue;
    const GL_Context_t *context = self->context;
    if (mode[0] == 'f') { // FIXME: translate all these into map-lookups?
        GL_queue_blit(queue, context);
    } else
    if (mode[0] == 's') {
        GL_queue_blit_s(queue, context);
    } else
    if (mode[0] == 'c') {
        GL_queue_blit_sr(queue, context);
    } else {
        return luaL_error(L, "unknown mode `%s`", mode);
    }

    return 0;
}

static int canvas_text_5onnos_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING, LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Font_Object_t *font = (const Font_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_FONT);
    const char *text = LUAX_STRING(L, 5);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = font->sheet;
    const GL_Cell_t *glyphs = font->glyphs;

    size_t width = 0, height = 0;
    for (const uint8_t *ptr = (const uint8_t *)text; *ptr != '\0'; ++ptr) { // Hack! Treat as unsigned! :)
        uint8_t c = *ptr;
        const GL_Cell_t cell_id = glyphs[(size_t)c];
        if (cell_id == GL_CELL_NIL) {
            continue;
        }
        const GL_Size_t cell_size = GL_sheet_size(sheet, cell_id, 1.0f, 1.0f);
        GL_sheet_blit(sheet, context, (GL_Point_t){ .x = x + (int)width, .y = y }, cell_id);
        width += cell_size.width;
        if (height < cell_size.height) {
            height = cell_size.height;
        }
    }

    lua_pushinteger(L, (lua_Integer)width);
    lua_pushinteger(L, (lua_Integer)height);

    return 2;
}

static int canvas_text_7onnosnN_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING, LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    const Font_Object_t *font = (const Font_Object_t *)LUAX_OBJECT(L, 4, OBJECT_TYPE_FONT);
    const char *text = LUAX_STRING(L, 5);
    float scale_x = LUAX_NUMBER(L, 6);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 7, scale_x);

    const GL_Context_t *context = self->context;
    const GL_Sheet_t *sheet = font->sheet;
    const GL_Cell_t *glyphs = font->glyphs;

    size_t width = 0, height = 0;
    for (const uint8_t *ptr = (const uint8_t *)text; *ptr != '\0'; ++ptr) { // Hack! Treat as unsigned! :)
        uint8_t c = *ptr;
        // TODO: add support for tabs and escape-commands.
        const GL_Cell_t cell_id = glyphs[(size_t)c];
        if (cell_id == GL_CELL_NIL) {
            continue;
        }
        const GL_Size_t cell_size = GL_sheet_size(sheet, cell_id, scale_x, scale_y);
        GL_sheet_blit_s(sheet, context, (GL_Point_t){ .x = x + (int)width, .y = y }, cell_id, scale_x, scale_y);
        width += cell_size.width;
        if (height < cell_size.height) {
            height = cell_size.height;
        }
    }

    lua_pushinteger(L, (lua_Integer)width);
    lua_pushinteger(L, (lua_Integer)height);

    return 2;
}

static int canvas_text_v_2nn(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(canvas_text_5onnos_2nn, 5)
        LUAX_OVERLOAD_BY_ARITY(canvas_text_7onnosnN_2nn, 6)
        LUAX_OVERLOAD_BY_ARITY(canvas_text_7onnosnN_2nn, 7)
    LUAX_OVERLOAD_END
}
