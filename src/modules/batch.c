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

#include "batch.h"

#include <config.h>
#include <libs/log.h>

#include "udt.h"
#include "utils/callbacks.h"

#define LOG_CONTEXT "batch"
#define META_TABLE  "Tofu_Graphics_Batch_mt"

static int batch_new_2on_1o(lua_State *L);
static int batch_gc_1o_0(lua_State *L);
static int batch_resize_2on_0(lua_State *L);
static int batch_grow_2on_0(lua_State *L);
static int batch_clear_1o_0(lua_State *L);
static int batch_add_v_0(lua_State *L);
static int batch_blit_3ooS_0(lua_State *L);

int batch_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", batch_new_2on_1o },
            { "__gc", batch_gc_1o_0 },
            { "resize", batch_resize_2on_0 },
            { "grow", batch_grow_2on_0 },
            { "clear", batch_clear_1o_0 },
            { "add", batch_add_v_0 },
            { "blit", batch_blit_3ooS_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int batch_new_2on_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *bank = (const Bank_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BANK);
    size_t capacity = (size_t)LUAX_INTEGER(L, 2);

    GL_Batch_t *batch = GL_batch_create(bank->sheet, capacity);
    if (!batch) {
        return luaL_error(L, "can't create batch");
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch %p created for bank %p w/ %d slots", batch, bank, capacity);

    Batch_Object_t *self = (Batch_Object_t *)luaX_newobject(L, sizeof(Batch_Object_t), &(Batch_Object_t){
            .bank = {
                .instance = bank,
                .reference = luaX_ref(L, 1)
            },
            .batch = batch
        }, OBJECT_TYPE_BATCH, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch %p created w/ bank %p", self, bank);

    return 1;
}

static int batch_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BATCH);

    luaX_unref(L, self->bank.reference);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank reference #%d released", self->bank.reference);

    GL_batch_destroy(self->batch);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch %p destroyed", self->batch);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch %p finalized", self);

    return 0;
}

static int batch_resize_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BATCH);
    size_t capacity = (size_t)LUAX_INTEGER(L, 2);

    bool resized = GL_batch_resize(self->batch, capacity);
    if (!resized) {
        return luaL_error(L, "can't resize batch %p to %d slots", self, capacity);
    }

    return 0;
}

static int batch_grow_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BATCH);
    size_t amount = (size_t)LUAX_INTEGER(L, 2);

    bool grown = GL_batch_grow(self->batch, amount);
    if (!grown) {
        return luaL_error(L, "can't grow batch %p by %d slots", self, amount);
    }

    return 0;
}

static int batch_clear_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BATCH);

    GL_batch_clear(self->batch);

    return 0;
}

static int batch_add_4onNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BATCH);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 2);
    int x = LUAX_OPTIONAL_INTEGER(L, 3, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 4, 0);

    GL_batch_add(self->batch, (GL_Batch_Sprite_t){
            .cell_id = cell_id,
            .position = (GL_Point_t){ .x = x, .y = y },
            .scale_x = 1.0f, .scale_y = 1.0f,
            .rotation = 0,
            .anchor_x = 0.5f, .anchor_y = 0.5f
        });

    return 0;
}

static int batch_add_5onnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BATCH);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    int rotation = LUAX_INTEGER(L, 5);

    GL_batch_add(self->batch, (GL_Batch_Sprite_t){
            .cell_id = cell_id,
            .position = (GL_Point_t){ .x = x, .y = y },
            .scale_x = 1.0f, .scale_y = 1.0f,
            .rotation = rotation,
            .anchor_x = 0.5f, .anchor_y = 0.5f
        });

    return 0;
}

static int batch_add_6onnnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BATCH);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    float scale_x = LUAX_NUMBER(L, 5);
    float scale_y = LUAX_NUMBER(L, 6);

    GL_batch_add(self->batch, (GL_Batch_Sprite_t){
            .cell_id = cell_id,
            .position = (GL_Point_t){ .x = x, .y = y },
            .scale_x = scale_x, .scale_y = scale_y,
            .rotation = 0,
            .anchor_x = 0.5f, .anchor_y = 0.5f
        });

    return 0;
}

static int batch_add_9onnnnnNNN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BATCH);
    GL_Cell_t cell_id = (GL_Cell_t)LUAX_INTEGER(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    float scale_x = LUAX_NUMBER(L, 5);
    float scale_y = LUAX_NUMBER(L, 6);
    int rotation = LUAX_OPTIONAL_NUMBER(L, 7, 0);
    float anchor_x = LUAX_OPTIONAL_NUMBER(L, 8, 0.5f);
    float anchor_y = LUAX_OPTIONAL_NUMBER(L, 9, anchor_x);

    GL_batch_add(self->batch, (GL_Batch_Sprite_t){
            .cell_id = cell_id,
            .position = (GL_Point_t){ .x = x, .y = y },
            .scale_x = scale_x, .scale_y = scale_y,
            .rotation = rotation,
            .anchor_x = anchor_x, .anchor_y = anchor_y
        });

    return 0;
}

static int batch_add_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(4, batch_add_4onNN_0)
        LUAX_OVERLOAD_ARITY(5, batch_add_5onnnn_0)
        LUAX_OVERLOAD_ARITY(6, batch_add_6onnnnn_0)
        LUAX_OVERLOAD_ARITY(7, batch_add_9onnnnnNNN_0)
        LUAX_OVERLOAD_ARITY(8, batch_add_9onnnnnNNN_0)
        LUAX_OVERLOAD_ARITY(9, batch_add_9onnnnnNNN_0)
    LUAX_OVERLOAD_END
}

static int batch_blit_3ooS_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Batch_Object_t *self = (const Batch_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BATCH);
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_CANVAS);
    const char *mode = LUAX_OPTIONAL_STRING(L, 3, "fast");

    const GL_Batch_t *batch = self->batch;
    const GL_Surface_t *surface = canvas->surface;
    if (mode[0] == 'f') { // FIXME: translate all these into map-lookups?
        GL_batch_blit(batch, surface);
    } else
    if (mode[0] == 's') {
        GL_batch_blit_s(batch, surface);
    } else
    if (mode[0] == 'c') {
        GL_batch_blit_sr(batch, surface);
    } else {
        return luaL_error(L, "unknown mode `%s`", mode);
    }

    return 0;
}