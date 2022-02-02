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
#include <libs/log.h>
#include <libs/path.h>
#include <libs/stb.h>
#include <systems/display.h>
#include <systems/interpreter.h>

#include "udt.h"
#include "utils/map.h"

#define LOG_CONTEXT "canvas"
#define MODULE_NAME "tofu.graphics.canvas"
#define META_TABLE  "Tofu_Graphics_Canvas_mt"
// FIXME: collapse meta and script name?

static int canvas_new_3oOO_1o(lua_State *L);
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
static int canvas_scan_v_0(lua_State *L);
static int canvas_process_v_0(lua_State *L);
static int canvas_copy_v_0(lua_State *L);
static int canvas_blit_v_0(lua_State *L);
static int canvas_tile_v_0(lua_State *L);
static int canvas_stencil_v_0(lua_State *L);
static int canvas_blend_v_0(lua_State *L);

// TODO: rename `Canvas` to `Context`?

int canvas_loader(lua_State *L)
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
            { "new", canvas_new_3oOO_1o },
            { "__gc", canvas_gc_1o_0 },
            // -- observers --
            { "image", canvas_image_1o_1o },
            // -- modifiers --
            { "push", canvas_push_1o_0 },
            { "pop", canvas_pop_2oN_0 },
            { "reset", canvas_reset_1o_0 },
            { "clipping", canvas_clipping_v_0 },
            { "shift", canvas_shift_v_0 },
            { "transparent", canvas_transparent_v_0 },
            // -- operations --
            { "clear", canvas_clear_2onB_0 },
            { "fill", canvas_fill_4onnnB_0 },
            { "scan", canvas_scan_v_0 },
            { "process", canvas_process_v_0 },
            { "copy", canvas_copy_v_0 },
            { "blit", canvas_blit_v_0 },
            { "tile", canvas_tile_v_0 },
            { "stencil", canvas_stencil_v_0 },
            { "blend", canvas_blend_v_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int canvas_new_3oOO_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Image_Object_t *image = (const Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);
    const Bank_Object_t *bank = (const Bank_Object_t *)LUAX_OPTIONAL_OBJECT(L, 2, OBJECT_TYPE_BANK, NULL);
    const Font_Object_t *font = (const Font_Object_t *)LUAX_OPTIONAL_OBJECT(L, 3, OBJECT_TYPE_FONT, NULL);

    GL_Context_t *context = GL_context_create(image->surface);
    if (!context) {
        return luaL_error(L, "can't create context");
    }

    Canvas_Object_t *self = (Canvas_Object_t *)luaX_newobject(L, sizeof(Canvas_Object_t), &(Canvas_Object_t){
            .context = context,
            .image = {
                .instance = image,
                .reference = luaX_ref(L, 1)
            },
            .bank = {
                .instance = bank,
                .reference = bank ? luaX_ref(L, 2) : -1
            },
            .font = {
                .instance = font,
                .reference = font ? luaX_ref(L, 3) : -1
            }
        }, OBJECT_TYPE_CANVAS, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p allocated w/ image %p", self, image);

    return 1;
}

static int canvas_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Canvas_Object_t *self = (Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    luaX_unref(L, self->image.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "image reference #%d released", self->image.reference);

    luaX_unref(L, self->bank.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank reference #%d released", self->bank.reference);

    luaX_unref(L, self->font.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "font reference #%d released", self->font.reference);

    GL_context_destroy(self->context);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "context %p destroyed", self->context);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "canvas %p finalized", self);

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
        LUAX_OVERLOAD_ARITY(1, canvas_transparent_1o_0)
        LUAX_OVERLOAD_ARITY(2, canvas_transparent_2ot_0)
        LUAX_OVERLOAD_ARITY(3, canvas_transparent_3onb_0)
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
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 5, self->image.instance->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 6, self->image.instance->surface->height);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    GL_context_scan(self->context, (GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height },
        _scan_callback, &(Canvas_Scan_Closure_t){ .interpreter = interpreter, .L = L, .index = 2 });

    return 0;
}

static int canvas_scan_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, canvas_scan_6ofNNNN_0)
        LUAX_OVERLOAD_ARITY(6, canvas_scan_6ofNNNN_0)
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
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    // idx #3: LUA_TFUNCTION
    int x = LUAX_OPTIONAL_INTEGER(L, 4, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 5, 0);
    int ox = LUAX_OPTIONAL_INTEGER(L, 6, 0);
    int oy = LUAX_OPTIONAL_INTEGER(L, 7, 0);
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 8, self->image.instance->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 9, self->image.instance->surface->height);

    const Interpreter_t *interpreter = (const Interpreter_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_INTERPRETER));

    GL_context_process(target->context, (GL_Point_t){ .x = x, .y = y },
        self->image.instance->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height },
        _process_callback, &(Canvas_Process_Closure_t){ .interpreter = interpreter, .L = L, .index = 3 });

    return 0;
}

static int canvas_process_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, canvas_process_9oofNNNNNN_0)
        LUAX_OVERLOAD_ARITY(5, canvas_process_9oofNNNNNN_0)
        LUAX_OVERLOAD_ARITY(9, canvas_process_9oofNNNNNN_0)
    LUAX_OVERLOAD_END
}

static int canvas_copy_8ooNNNNNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    int x = LUAX_OPTIONAL_INTEGER(L, 3, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 4, 0);
    int ox = LUAX_OPTIONAL_INTEGER(L, 5, 0);
    int oy = LUAX_OPTIONAL_INTEGER(L, 6, 0);
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 7, self->image.instance->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 8, self->image.instance->surface->height);

    GL_context_copy(target->context, (GL_Point_t){ .x = x, .y = y },
        self->image.instance->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height });

    return 0;
}

static int canvas_copy_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, canvas_copy_8ooNNNNNN_0)
        LUAX_OVERLOAD_ARITY(4, canvas_copy_8ooNNNNNN_0)
        LUAX_OVERLOAD_ARITY(8, canvas_copy_8ooNNNNNN_0)
    LUAX_OVERLOAD_END
}

// FIXME: rename `surface` to source/target on arguments list where a surface appears twice.
static int canvas_blit_8ooNNNNNNNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    int x = LUAX_OPTIONAL_INTEGER(L, 3, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 4, 0);
    int ox = LUAX_OPTIONAL_INTEGER(L, 5, 0);
    int oy = LUAX_OPTIONAL_INTEGER(L, 6, 0);
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 7, self->image.instance->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 8, self->image.instance->surface->height);

    GL_context_blit(target->context, (GL_Point_t){ .x = x, .y = y },
        self->image.instance->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height });

    return 0;
}

static int canvas_blit_9oooNNNNNNNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    const XForm_Object_t *xform = (const XForm_Object_t *)LUAX_OBJECT(L, 3, OBJECT_TYPE_XFORM);
    int x = LUAX_OPTIONAL_INTEGER(L, 4, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 5, 0);
    int ox = LUAX_OPTIONAL_INTEGER(L, 6, 0);
    int oy = LUAX_OPTIONAL_INTEGER(L, 7, 0);
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 8, self->image.instance->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 9, self->image.instance->surface->height);

    // FIXME: rename `GL_xform_blit` to `GL_surface_xblit`?
    GL_xform_blit(xform->xform,
        target->context, (GL_Point_t){ .x = x, .y = y },
        self->image.instance->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height });

    return 0;
}

static int canvas_blit_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, canvas_blit_8ooNNNNNNNN_0)
        LUAX_OVERLOAD_ARITY(3, canvas_blit_9oooNNNNNNNN_0)
        LUAX_OVERLOAD_ARITY(4, canvas_blit_8ooNNNNNNNN_0)
        LUAX_OVERLOAD_ARITY(5, canvas_blit_9oooNNNNNNNN_0)
        LUAX_OVERLOAD_ARITY(8, canvas_blit_8ooNNNNNNNN_0)
        LUAX_OVERLOAD_ARITY(9, canvas_blit_9oooNNNNNNNN_0)
    LUAX_OVERLOAD_END
}

static int canvas_tile_10oonnNNNNNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    int offset_x = LUAX_INTEGER(L, 3);
    int offset_y = LUAX_INTEGER(L, 4);
    int x = LUAX_OPTIONAL_INTEGER(L, 5, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 6, 0);
    int ox = LUAX_OPTIONAL_INTEGER(L, 7, 0);
    int oy = LUAX_OPTIONAL_INTEGER(L, 8, 0);
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 9, self->image.instance->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 10, self->image.instance->surface->height);

    GL_context_tile(target->context, (GL_Point_t){ .x = x, .y = y },
        self->image.instance->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height },
        (GL_Point_t){ .x = offset_x, .y = offset_y });

    return 0;
}

static int canvas_tile_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(4, canvas_tile_10oonnNNNNNN_0)
        LUAX_OVERLOAD_ARITY(6, canvas_tile_10oonnNNNNNN_0)
        LUAX_OVERLOAD_ARITY(10, canvas_tile_10oonnNNNNNN_0)
    LUAX_OVERLOAD_END
}

static const Map_Entry_t _comparators[GL_Comparators_t_CountOf] = {
    { "never", GL_COMPARATOR_NEVER },
    { "less", GL_COMPARATOR_LESS },
    { "less-or-equal", GL_COMPARATOR_LESS_OR_EQUAL },
    { "greater", GL_COMPARATOR_GREATER },
    { "greater-or-equal", GL_COMPARATOR_GREATER_OR_EQUAL },
    { "equal", GL_COMPARATOR_EQUAL },
    { "not-equal", GL_COMPARATOR_NOT_EQUAL },
    { "always", GL_COMPARATOR_ALWAYS }
};

static int canvas_stencil_11ooosnNNNNNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *mask = (const Canvas_Object_t *)LUAX_OBJECT(L, 3, OBJECT_TYPE_CANVAS);
    const char *comparator = LUAX_STRING(L, 4);
    GL_Pixel_t threshold = (GL_Pixel_t)LUAX_UNSIGNED(L, 5);
    int x = LUAX_OPTIONAL_INTEGER(L, 6, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 7, 0);
    int ox = LUAX_OPTIONAL_INTEGER(L, 8, 0);
    int oy = LUAX_OPTIONAL_INTEGER(L, 9, 0);
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 10, self->image.instance->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 11, self->image.instance->surface->height);

    const Map_Entry_t *entry = map_find_key(L, comparator, _comparators, GL_Comparators_t_CountOf);
    GL_context_stencil(target->context, (GL_Point_t){ .x = x, .y = y },
        self->image.instance->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height  },
        mask->image.instance->surface, (GL_Comparators_t)entry->value, threshold);

    return 0;
}

static int canvas_stencil_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(5, canvas_stencil_11ooosnNNNNNN_0)
        LUAX_OVERLOAD_ARITY(7, canvas_stencil_11ooosnNNNNNN_0)
        LUAX_OVERLOAD_ARITY(11, canvas_stencil_11ooosnNNNNNN_0)
    LUAX_OVERLOAD_END
}

static const Map_Entry_t _functions[GL_Functions_t_CountOf] = {
    { "replace", GL_FUNCTIONS_REPLACE },
    { "add", GL_FUNCTIONS_ADD },
    { "add-clamped", GL_FUNCTIONS_ADD_CLAMPED },
    { "subtract", GL_FUNCTIONS_SUBTRACT },
    { "subtract-clamped", GL_FUNCTIONS_SUBTRACT_CLAMPED },
    { "reverse-subtract", GL_FUNCTIONS_REVERSE_SUBTRACT },
    { "reverse-subtract-clamped", GL_FUNCTIONS_REVERSE_SUBTRACT_CLAMPED },
    { "multiply", GL_FUNCTIONS_MULTIPLY },
    { "multiply-clamped", GL_FUNCTIONS_MULTIPLY_CLAMPED },
    { "min", GL_FUNCTIONS_MIN },
    { "max", GL_FUNCTIONS_MAX }
};

static int canvas_blend_9oosNNNNNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *self = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);
    const Canvas_Object_t *target = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    const char *function = LUAX_STRING(L, 3);
    int x = LUAX_OPTIONAL_INTEGER(L, 4, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 5, 0);
    int ox = LUAX_OPTIONAL_INTEGER(L, 6, 0);
    int oy = LUAX_OPTIONAL_INTEGER(L, 7, 0);
    size_t width = LUAX_OPTIONAL_UNSIGNED(L, 8, self->image.instance->surface->width);
    size_t height = LUAX_OPTIONAL_UNSIGNED(L, 9, self->image.instance->surface->height);

    const Map_Entry_t *entry = map_find_key(L, function, _functions, GL_Functions_t_CountOf);
    GL_context_blend(target->context, (GL_Point_t){ .x = x, .y = y },
        self->image.instance->surface, (GL_Rectangle_t){ .x = ox, .y = oy, .width = width, .height = height },
        (GL_Functions_t)entry->value);

    return 0;
}

static int canvas_blend_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, canvas_blend_9oosNNNNNN_0)
        LUAX_OVERLOAD_ARITY(5, canvas_blend_9oosNNNNNN_0)
        LUAX_OVERLOAD_ARITY(9, canvas_blend_9oosNNNNNN_0)
    LUAX_OVERLOAD_END
}
