/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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

#include "internal/map.h"
#include "internal/udt.h"

#include <core/config.h>
#include <libs/fmath.h>
#include <libs/log.h>

#define LOG_CONTEXT "tweener"
#define META_TABLE  "Tofu_Generators_Tweener_mt"

static int tweener_new_4sNNN_1o(lua_State *L);
static int tweener_gc_1o_0(lua_State *L);
static int tweener_easing_v_v(lua_State *L);
static int tweener_duration_v_v(lua_State *L);
static int tweener_range_v_v(lua_State *L);
static int tweener_evaluate_2on_1n(lua_State *L);

int tweener_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", tweener_new_4sNNN_1o },
            { "__gc", tweener_gc_1o_0 },
            { "__call", tweener_evaluate_2on_1n }, // Call meta-method, mapped to `at(...)`.
            { "easing", tweener_easing_v_v },
            { "duration", tweener_duration_v_v },
            { "range", tweener_range_v_v },
            { "evaluate", tweener_evaluate_2on_1n },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static const Map_Entry_t _easings[Easing_Types_t_CountOf] = {
    { "linear", EASING_TYPE_LINEAR },
    { "quadratic-in", EASING_TYPE_QUADRATIC_IN },
    { "quadratic-out", EASING_TYPE_QUADRATIC_OUT },
    { "quadratic-in-out", EASING_TYPE_QUADRATIC_IN_OUT },
    { "cubic-in", EASING_TYPE_CUBIC_IN },
    { "cubic-out", EASING_TYPE_CUBIC_OUT },
    { "cubic-in-out", EASING_TYPE_CUBIC_IN_OUT },
    { "quartic-in", EASING_TYPE_QUARTIC_IN },
    { "quartic-out", EASING_TYPE_QUARTIC_OUT },
    { "quartic-in-out", EASING_TYPE_QUARTIC_IN_OUT },
    { "quintic-in", EASING_TYPE_QUINTIC_IN },
    { "quintic-out", EASING_TYPE_QUINTIC_OUT },
    { "quintic-in-out", EASING_TYPE_QUINTIC_IN_OUT },
    { "sine-in", EASING_TYPE_SINE_IN },
    { "sine-out", EASING_TYPE_SINE_OUT },
    { "sine-in-out", EASING_TYPE_SINE_IN_OUT },
    { "circular-in", EASING_TYPE_CIRCULAR_IN },
    { "circular-out", EASING_TYPE_CIRCULAR_OUT },
    { "circular-in-out", EASING_TYPE_CIRCULAR_IN_OUT },
    { "exponential-in", EASING_TYPE_EXPONENTIAL_IN },
    { "exponential-out", EASING_TYPE_EXPONENTIAL_OUT },
    { "exponential-in-out", EASING_TYPE_EXPONENTIAL_IN_OUT },
    { "elastic-in", EASING_TYPE_ELASTIC_IN },
    { "elastic-out", EASING_TYPE_ELASTIC_OUT },
    { "elastic-in-out", EASING_TYPE_ELASTIC_IN_OUT },
    { "back-in", EASING_TYPE_BACK_IN },
    { "back-out", EASING_TYPE_BACK_OUT },
    { "back-in-out", EASING_TYPE_BACK_IN_OUT },
    { "bounce-in", EASING_TYPE_BOUNCE_IN },
    { "bounce-out", EASING_TYPE_BOUNCE_OUT },
    { "bounce-in-out", EASING_TYPE_BOUNCE_IN_OUT }
};

static Easing_Function_t _functions[Easing_Types_t_CountOf] = {
    easing_linear,
    easing_quadratic_in,
    easing_quadratic_out,
    easing_quadratic_in_out,
    easing_cubic_in,
    easing_cubic_out,
    easing_cubic_in_out,
    easing_quartic_in,
    easing_quartic_out,
    easing_quartic_in_out,
    easing_quintic_in,
    easing_quintic_out,
    easing_quintic_in_out,
    easing_sine_in,
    easing_sine_out,
    easing_sine_in_out,
    easing_circular_in,
    easing_circular_out,
    easing_circular_in_out,
    easing_exponential_in,
    easing_exponential_out,
    easing_exponential_in_out,
    easing_elastic_in,
    easing_elastic_out,
    easing_elastic_in_out,
    easing_back_in,
    easing_back_out,
    easing_back_in_out,
    easing_bounce_out,
    easing_bounce_in,
    easing_bounce_in_out
};

static int tweener_new_4sNNN_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *easing = LUAX_STRING(L, 1);
    float duration = LUAX_OPTIONAL_NUMBER(L, 2, 1.0f);
    float from = LUAX_OPTIONAL_NUMBER(L, 3, 0.0f);
    float to = LUAX_OPTIONAL_NUMBER(L, 4, 1.0f);

    const Map_Entry_t *entry = map_find_key(L, easing, _easings, Easing_Types_t_CountOf);
    Tweener_Object_t *self = (Tweener_Object_t *)luaX_newobject(L, sizeof(Tweener_Object_t), &(Tweener_Object_t){
            .easing = (Easing_Types_t)entry->value,
            .function = _functions[entry->value],
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
    const Tweener_Object_t *self = (const Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);

    const Map_Entry_t *entry = map_find_value(L, self->easing, _easings, Easing_Types_t_CountOf);
    lua_pushstring(L, entry->key);

    return 1;
}

static int tweener_easing_2os_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    Tweener_Object_t *self = (Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);
    const char *easing = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(L, easing, _easings, Easing_Types_t_CountOf);
    self->easing = (Easing_Types_t)entry->value;
    self->function = _functions[entry->value];

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

static int tweener_range_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Tweener_Object_t *self = (const Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);

    const float from = self->from;
    const float to = self->to;

    lua_pushnumber(L, (lua_Number)from);
    lua_pushnumber(L, (lua_Number)to);

    return 2;
}

static int tweener_range_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Tweener_Object_t *self = (Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);
    float from = LUAX_NUMBER(L, 2);
    float to = LUAX_NUMBER(L, 3);

    self->from = from;
    self->to = to;

    return 0;
}

static int tweener_range_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, tweener_range_1o_2nn)
        LUAX_OVERLOAD_ARITY(3, tweener_range_3onn_0)
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
