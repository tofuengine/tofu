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

#include "batch.h"

#include <config.h>
#include <core/io/display.h>
#include <libs/fs/fsaux.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "callbacks.h"
#include "structs.h"
#include "udt.h"

#include <math.h>

#define LOG_CONTEXT "batch"
#define META_TABLE  "Tofu_Graphics_Batch_mt"

static int batch_new(lua_State *L);
static int batch_gc(lua_State *L);
static int batch_grow(lua_State *L);
static int batch_clear(lua_State *L);
static int batch_add(lua_State *L);
static int batch_blit(lua_State *L);

static const struct luaL_Reg _batch_functions[] = {
    { "new", batch_new },
    {"__gc", batch_gc },
    { "grow", batch_grow },
    { "clear", batch_clear },
    { "add", batch_add },
    { "blit", batch_blit },
    { NULL, NULL }
};

static const luaX_Const _batch_constants[] = {
    // { "FAST", LUA_CT_INTEGER, { .i = MODE_FAST } },
    // { "SIMPLE", LUA_CT_INTEGER, { .i = MODE_SIMPLE } },
    // { "COMPLETE", LUA_CT_INTEGER, { .i = MODE_COMPLETE } },
    { NULL }
};

int batch_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _batch_functions, _batch_constants, nup, META_TABLE);
}

static int batch_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Bank_Object_t *bank = (const Bank_Object_t *)LUAX_USERDATA(L, 1);
    size_t slots = LUAX_INTEGER(L, 2);

    GL_Batch_t *batch = GL_batch_create(bank->sheet, slots);
    if (!batch) {
        return luaL_error(L, "can't create batch");
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch %p created for bank %p w/ #%d slots", batch, bank, slots);

    Batch_Object_t *self = (Batch_Object_t *)lua_newuserdatauv(L, sizeof(Batch_Object_t), 1);
    *self = (Batch_Object_t){
            .bank = bank,
            .bank_reference = luaX_ref(L, 1),
            .batch = batch
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch %p created w/ bank %p", self, bank);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int batch_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_USERDATA(L, 1);

    if (self->bank_reference != LUAX_REFERENCE_NIL) {
        luaX_unref(L, self->bank_reference);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "bank reference #%d released", self->bank_reference);
    }

    GL_batch_destroy(self->batch);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch %p destroyed", self->batch);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "batch %p finalized", self);

    return 0;
}

static int batch_grow(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_USERDATA(L, 1);
    size_t amount = LUAX_INTEGER(L, 2);

    bool grown = GL_batch_grow(self->batch, amount);
    if (!grown) {
        return luaL_error(L, "can't grow batch %p by %d slots", self, amount);
    }

    return 0;
}

static int batch_clear(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_USERDATA(L, 1);

    GL_batch_clear(self->batch);

    return 0;
}

static int batch_add4(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_USERDATA(L, 1);
    int cell_id = LUAX_INTEGER(L, 2); // FIXME: make cell-id a `size_t' or a generic uint?
    int x = LUAX_OPTIONAL_INTEGER(L, 3, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 4, 0);

    GL_batch_add(self->batch, (GL_Batch_Sprite_t){
            .cell_id = cell_id,
            .position = (GL_Point_t){ .x = x, .y = y },
            .sx = 1.0f, .sy = 1.0f
        });

    return 0;
}

static int batch_add5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_USERDATA(L, 1);
    int cell_id = LUAX_INTEGER(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    float scale = LUAX_NUMBER(L, 5);

    GL_batch_add(self->batch, (GL_Batch_Sprite_t){
            .cell_id = cell_id,
            .position = (GL_Point_t){ .x = x, .y = y },
            .sx = scale, .sy = scale
        });

    return 0;
}

static int batch_add6_7_8_9(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Batch_Object_t *self = (Batch_Object_t *)LUAX_USERDATA(L, 1);
    int cell_id = LUAX_INTEGER(L, 2);
    int x = LUAX_INTEGER(L, 3);
    int y = LUAX_INTEGER(L, 4);
    int rotation = LUAX_INTEGER(L, 5);
    float scale_x = LUAX_NUMBER(L, 6);
    float scale_y = LUAX_OPTIONAL_NUMBER(L, 7, scale_x);
    float anchor_x = LUAX_OPTIONAL_NUMBER(L, 8, 0.5f);
    float anchor_y = LUAX_OPTIONAL_NUMBER(L, 9, anchor_x);

    GL_batch_add(self->batch, (GL_Batch_Sprite_t){
            .cell_id = cell_id,
            .position = (GL_Point_t){ .x = x, .y = y },
            .sx = scale_x, .sy = scale_y,
            .rotation = rotation,
            .ax = anchor_x,
            .ay = anchor_y
        });

    return 0;
}

static int batch_add(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(4, batch_add4)
        LUAX_OVERLOAD_ARITY(7, batch_add6_7_8_9)
        LUAX_OVERLOAD_ARITY(8, batch_add6_7_8_9)
        LUAX_OVERLOAD_ARITY(6, batch_add6_7_8_9)
        LUAX_OVERLOAD_ARITY(9, batch_add6_7_8_9)
        LUAX_OVERLOAD_ARITY(5, batch_add5)
    LUAX_OVERLOAD_END
}

static int batch_blit(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Batch_Object_t *self = (const Batch_Object_t *)LUAX_USERDATA(L, 1);
    const char *mode = LUAX_OPTIONAL_STRING(L, 2, "fast");

    const GL_Batch_t *batch = self->batch;
    const GL_Context_t *context = self->bank->context;
    if (mode[0] == 'f') {
        GL_batch_blit(batch, context);
    } else
    if (mode[0] == 's') {
        GL_batch_blit_s(batch, context);
    } else
    if (mode[0] == 'c') {
        GL_batch_blit_sr(batch, context);
    } else {
        return luaL_error(L, "unknown mode `%s`", mode);
    }

    return 0;
}
