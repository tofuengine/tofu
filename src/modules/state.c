/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2026 Marco Lizza
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

#include "state.h"

#include "internal/udt.h"

#include <core/config.h>
#define _LOG_TAG "state"
#include <libs/log.h>
#include <libs/stb.h>
#include <systems/display.h>
#include <systems/interpreter.h>

static int state_new_1o_1o(lua_State *L);
static int state_gc_1o_0(lua_State *L);
static int state_remember_2os_0(lua_State *L);
static int state_recall_2os_0(lua_State *L);
static int state_forget_2oS_0(lua_State *L);
static int state_push_1o_0(lua_State *L);
static int state_pop_2oN_0(lua_State *L);
static int state_reset_1o_0(lua_State *L);
static int state_clipping_v_0(lua_State *L);
static int state_shift_v_0(lua_State *L);
static int state_transparent_v_0(lua_State *L);
static int state_bank_2on_0(lua_State *L);

int state_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", state_new_1o_1o },
            { "__gc", state_gc_1o_0 },
            // -- metamethods --
            // -- getters/setters --
            // -- accessors --
            // -- mutators --
            { "remember", state_remember_2os_0 },
            { "recall", state_recall_2os_0 },
            { "forget", state_forget_2oS_0 },
            { "push", state_push_1o_0 },
            { "pop", state_pop_2oN_0 },
            { "reset", state_reset_1o_0 },
            { "clipping", state_clipping_v_0 },
            { "shift", state_shift_v_0 },
            { "transparent", state_transparent_v_0 },
            { "bank", state_bank_2on_0 },
            // -- operations --
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static int state_new_1o_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Canvas_Object_t *canvas = (const Canvas_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

    GL_Context_t *context = canvas->context;

    State_Object_t *self = (State_Object_t *)udt_newobject(L, sizeof(State_Object_t), &(State_Object_t){
            .context = context,
            .canvas = {
                .instance = canvas,
                .reference = luaX_ref(L, 1)
            }
        }, OBJECT_TYPE_STATE);

    LOG_D("state %p allocated w/ context %p for canvas %p", self, context, canvas);

    return 1;
}

static int state_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);

    luaX_unref(L, self->canvas.reference);
    LOG_D("canvas reference #%d released", self->canvas.reference);

    LOG_D("state %p finalized", self);

    return 0;
}

static int state_remember_2os_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);
    const char *id = LUAX_STRING(L, 2);

    GL_context_remember(self->context, id);

    return 0;
}

static int state_recall_2os_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);
    const char *id = LUAX_STRING(L, 2);

    if (!GL_context_recall(self->context, id)) {
        lua_pushnil(L);
    }

    return 1;
}

static int state_forget_2oS_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);
    const char *id = LUAX_OPTIONAL_STRING(L, 2, NULL);

    GL_context_forget(self->context, id);

    return 0;
}

static int state_push_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);

    GL_context_push(self->context);

    return 0;
}

static int state_pop_2oN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);
    size_t levels = LUAX_OPTIONAL_UNSIGNED(L, 2, 1);

    GL_context_pop(self->context, levels > 0 ? levels : SIZE_MAX);

    return 0;
}

static int state_reset_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);

    GL_context_reset(self->context);

    return 0;
}

static int state_clipping_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);

    GL_context_set_clipping(self->context, NULL);

    return 0;
}

static int state_clipping_5onnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    size_t width = LUAX_UNSIGNED(L, 4);
    size_t height = LUAX_UNSIGNED(L, 5);

    GL_context_set_clipping(self->context, &(GL_Rectangle_t){ .x = x, .y = y, .width = width, .height = height });

    return 0;
}

static int state_clipping_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(state_clipping_1o_0, 1)
        LUAX_OVERLOAD_BY_ARITY(state_clipping_5onnnn_0, 5)
    LUAX_OVERLOAD_END
}

static int state_shift_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);

    GL_context_set_shifting(self->context, NULL, NULL, 0);

    return 0;
}

static int state_shift_2ot_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);
    int shift_table = LUAX_TABLE(L, 2);

    GL_Pixel_t *from = NULL;
    GL_Pixel_t *to = NULL;

    lua_pushnil(L);
    while (lua_next(L, shift_table)) {
        arrpush(from, (GL_Pixel_t)LUAX_UNSIGNED(L, -2));
        arrpush(to, (GL_Pixel_t)LUAX_UNSIGNED(L, -1));

        lua_pop(L, 1);
    }

    GL_context_set_shifting(self->context, from, to, arrlenu(from));

    arrfree(from);
    arrfree(to);

    return 0;
}

static int state_shift_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);
    GL_Pixel_t from = (GL_Pixel_t)LUAX_UNSIGNED(L, 2);
    GL_Pixel_t to = (GL_Pixel_t)LUAX_UNSIGNED(L, 3);

    GL_context_set_shifting(self->context, &from, &to, 1);

    return 0;
}

static int state_shift_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(state_shift_1o_0, 1)
        LUAX_OVERLOAD_BY_ARITY(state_shift_2ot_0, 2)
        LUAX_OVERLOAD_BY_ARITY(state_shift_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static int state_transparent_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);

    GL_context_set_transparent(self->context, NULL, NULL, 0);

    return 0;
}

static int state_transparent_2ot_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);
    int transparency_table = LUAX_TABLE(L, 2);

    GL_Pixel_t *indexes = NULL;
    GL_Bool_t *transparent = NULL;

    lua_pushnil(L);
    while (lua_next(L, transparency_table)) {
        arrpush(indexes, (GL_Pixel_t)LUAX_UNSIGNED(L, -2));
        arrpush(transparent, LUAX_BOOLEAN(L, -1) ? GL_BOOL_TRUE : GL_BOOL_FALSE);

        lua_pop(L, 1);
    }

    GL_context_set_transparent(self->context, indexes, transparent, arrlenu(indexes));

    arrfree(indexes);
    arrfree(transparent);

    return 0;
}

static int state_transparent_3onb_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED_RANGE(L, 2, 0, GL_PALETTE_MAX_COLORS - 1);
    GL_Bool_t transparent = LUAX_BOOLEAN(L, 3) ? GL_BOOL_TRUE : GL_BOOL_FALSE;

    GL_context_set_transparent(self->context, &index, &transparent, 1);

    return 0;
}

static int state_transparent_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(state_transparent_1o_0, 1)
        LUAX_OVERLOAD_BY_ARITY(state_transparent_2ot_0, 2)
        LUAX_OVERLOAD_BY_ARITY(state_transparent_3onb_0, 3)
    LUAX_OVERLOAD_END
}

static int state_bank_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    State_Object_t *self = (State_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_STATE);
    size_t bank = (size_t)LUAX_UNSIGNED(L, 2);

    GL_context_set_bank(self->context, bank);

    return 0;
}
