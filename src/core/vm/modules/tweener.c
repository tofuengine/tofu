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

#include "tweener.h"

#include <config.h>
#include <libs/fmath.h>
#include <libs/log.h>
#include <libs/luax.h>

#include "udt.h"

#define LOG_CONTEXT "tweener"
#define META_TABLE  "Tofu_Math_Tweener_mt"

static int tweener_new_4sNNN_1o(lua_State *L);
static int tweener_gc_1o_0(lua_State *L);
static int tweener_easing_v_v(lua_State *L);
static int tweener_duration_v_v(lua_State *L);
static int tweener_from_v_v(lua_State *L);
static int tweener_to_v_v(lua_State *L);
static int tweener_evaluate_2on_1n(lua_State *L);

int tweener_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", tweener_new_4sNNN_1o },
            { "__gc", tweener_gc_1o_0 },
            { "__call", tweener_evaluate_2on_1n }, // Call meta-method, mapped to `at(...)`.
            { "easing", tweener_easing_v_v },
            { "duration", tweener_duration_v_v },
            { "from", tweener_from_v_v },
            { "to", tweener_to_v_v },
            { "evaluate", tweener_evaluate_2on_1n },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int tweener_new_4sNNN_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);
    float duration = LUAX_OPTIONAL_NUMBER(L, 2, 1.0f);
    float from = LUAX_OPTIONAL_NUMBER(L, 3, 0.0f);
    float to = LUAX_OPTIONAL_NUMBER(L, 4, 1.0f);

    const Easing_t *easing = easing_from_id(id);
    if (!easing) {
        return luaL_error(L, "can't find easing w/ id `%s`", id);
    }

    Tweener_Object_t *self = (Tweener_Object_t *)luaX_newobject(L, sizeof(Tweener_Object_t), &(Tweener_Object_t){
            .function = easing->function,
            .duration = duration,
            .from = from,
            .to = to
        }, OBJECT_TYPE_TWEENER, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "tweener %p allocated", self);

    return 1;
}

static int tweener_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Tweener_Object_t *self = (Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);

    // Nothing to dispose.

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "tweener %p finalized", self);

    return 0;
}

// FIXME: implement only the observers? (also for `Wave`s)
static int tweener_easing_1o_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    //const Tweener_Object_t *self = (const Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);

    // FIXME: move to `map_find_XXX()` usage.

    lua_pushstring(L, "<undefined>");

    return 1;
}

static int tweener_easing_2os_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    Tweener_Object_t *self = (Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);
    const char *id = LUAX_STRING(L, 2);

    const Easing_t *easing = easing_from_id(id);
    if (!easing) {
        return luaL_error(L, "can't find easing w/ id `%s`", id);
    }

    self->function = easing->function;

    return 0;
}

static int tweener_easing_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, tweener_easing_1o_1s)
        LUAX_OVERLOAD_ARITY(2, tweener_easing_2os_0)
    LUAX_OVERLOAD_END
}

static int tweener_duration_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Tweener_Object_t *self = (const Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);

    const float duration = self->duration;

    lua_pushnumber(L, (lua_Number)duration);

    return 1;
}

static int tweener_duration_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Tweener_Object_t *self = (Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);
    float duration = LUAX_NUMBER(L, 2);

    self->duration = duration;

    return 0;
}

static int tweener_duration_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, tweener_duration_1o_1n)
        LUAX_OVERLOAD_ARITY(2, tweener_duration_2on_0)
    LUAX_OVERLOAD_END
}

static int tweener_from_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Tweener_Object_t *self = (const Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);

    const float from = self->from;

    lua_pushnumber(L, (lua_Number)from);

    return 1;
}

static int tweener_from_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Tweener_Object_t *self = (Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);
    float from = LUAX_NUMBER(L, 2);

    self->from = from;

    return 0;
}

static int tweener_from_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, tweener_from_1o_1n)
        LUAX_OVERLOAD_ARITY(2, tweener_from_2on_0)
    LUAX_OVERLOAD_END
}

static int tweener_to_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Tweener_Object_t *self = (const Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);

    const float to = self->to;

    lua_pushnumber(L, (lua_Number)to);

    return 1;
}

static int tweener_to_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Tweener_Object_t *self = (Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);
    float to = LUAX_NUMBER(L, 2);

    self->to = to;

    return 0;
}

static int tweener_to_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, tweener_to_1o_1n)
        LUAX_OVERLOAD_ARITY(2, tweener_to_2on_0)
    LUAX_OVERLOAD_END
}

static int tweener_evaluate_2on_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Tweener_Object_t *self = (const Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);
    float time = LUAX_NUMBER(L, 2);

    float ratio = time / self->duration;
#ifdef __TWEENER_CLAMP__
    float eased_ratio = self->function(FCLAMP(ratio, 0.0f, 1.0f));
#else   /* __TWEENER_CLAMP__ */
    float eased_ratio = self->function(ratio);
#endif  /* __TWEENER_CLAMP__ */
    float value = FLERP(self->from, self->to, eased_ratio);

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}
