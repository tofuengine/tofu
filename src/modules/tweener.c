/*
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "internal/udt.h"

#include <core/config.h>
#include <libs/fmath.h>
#define _LOG_TAG "tweener"
#include <libs/log.h>

static int tweener_new_5eNNNE_1o(lua_State *L);
static int tweener_gc_1o_0(lua_State *L);
static int tweener_clamp_v_v(lua_State *L);
static int tweener_easing_v_v(lua_State *L);
static int tweener_duration_v_v(lua_State *L);
static int tweener_range_v_v(lua_State *L);
static int tweener_evaluate_2on_1n(lua_State *L);

int tweener_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", tweener_new_5eNNNE_1o },
            { "__gc", tweener_gc_1o_0 },
            // -- metamethods --
            { "__call", tweener_evaluate_2on_1n }, // Call metamethod, mapped to `evaluate(...)`.
            // -- getters/setters --
            { "clamp", tweener_clamp_v_v },
            { "easing", tweener_easing_v_v },
            { "duration", tweener_duration_v_v },
            { "range", tweener_range_v_v },
            // -- operations --
            { "evaluate", tweener_evaluate_2on_1n },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static const char *_easing_types[Easing_Types_t_CountOf + 1] = {
    "linear",
    "quadratic-in",
    "quadratic-out",
    "quadratic-in-out",
    "cubic-in",
    "cubic-out",
    "cubic-in-out",
    "quartic-in",
    "quartic-out",
    "quartic-in-out",
    "quintic-in",
    "quintic-out",
    "quintic-in-out",
    "sine-in",
    "sine-out",
    "sine-in-out",
    "circular-in",
    "circular-out",
    "circular-in-out",
    "exponential-in",
    "exponential-out",
    "exponential-in-out",
    "elastic-in",
    "elastic-out",
    "elastic-in-out",
    "back-in",
    "back-out",
    "back-in-out",
    "bounce-in",
    "bounce-out",
    "bounce-in-out",
    NULL
};

static const Easing_Function_t _easing_functions[Easing_Types_t_CountOf] = {
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

static float _clamp_none(float value)
{
    return value;
}

static float _clamp_lower(float value)
{
    return FCLAMP(value, 0.0f, value);
}

static float _clamp_upper(float value)
{
    return FCLAMP(value, value, 1.0f);
}

static float _clamp_both(float value)
{
    return FCLAMP(value, 0.0f, 1.0f);
}

static const char *_clamp_modes[Clamp_Modes_t_CountOf + 1] = {
    "none",
    "lower",
    "upper",
    "both",
    NULL
};

static const Clamp_Function_t _clamp_functions[Clamp_Modes_t_CountOf] = {
    _clamp_none,
    _clamp_lower,
    _clamp_upper,
    _clamp_both
};

static int tweener_new_5eNNNE_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TENUM)
    LUAX_SIGNATURE_END
    Easing_Types_t easing = (Easing_Types_t)LUAX_ENUM(L, 1, _easing_types);
    float duration = LUAX_OPTIONAL_NUMBER(L, 2, 1.0f);
    float from = LUAX_OPTIONAL_NUMBER(L, 3, 0.0f);
    float to = LUAX_OPTIONAL_NUMBER(L, 4, 1.0f);
    Clamp_Modes_t clamp = (Clamp_Modes_t)LUAX_OPTIONAL_ENUM(L, 5, _clamp_modes, CLAMP_MODE_BOTH);

    Tweener_Object_t *self = (Tweener_Object_t *)udt_newobject(L, sizeof(Tweener_Object_t), &(Tweener_Object_t){
            .clamp = clamp,
            .clamp_function = _clamp_functions[clamp],
            .easing = easing,
            .easing_function = _easing_functions[easing],
            .duration = duration,
            .from = from,
            .to = to
        }, OBJECT_TYPE_TWEENER);

    LOG_D("tweener %p allocated", self);

    return 1;
}

static int tweener_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Tweener_Object_t *self = (Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);

    // Nothing to dispose.

    LOG_D("tweener %p finalized", self);

    return 0;
}

// FIXME: implement only the observers? (also for `Wave`s)
static int tweener_easing_1o_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Tweener_Object_t *self = (const Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);

    lua_pushstring(L, _easing_types[self->easing]);

    return 1;
}

static int tweener_easing_2os_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    Tweener_Object_t *self = (Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);
    Easing_Types_t easing = (Easing_Types_t)LUAX_ENUM(L, 2, _easing_types);

    self->easing = easing;
    self->easing_function = _easing_functions[easing];

    return 0;
}

static int tweener_easing_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(tweener_easing_1o_1s, 1)
        LUAX_OVERLOAD_BY_ARITY(tweener_easing_2os_0, 2)
    LUAX_OVERLOAD_END
}

static int tweener_clamp_1o_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Tweener_Object_t *self = (const Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);

    lua_pushstring(L, _clamp_modes[self->clamp]);

    return 1;
}

static int tweener_clamp_2oe_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    Tweener_Object_t *self = (Tweener_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_TWEENER);
    Clamp_Modes_t clamp = (Clamp_Modes_t)LUAX_ENUM(L, 2, _clamp_modes);

    self->clamp = clamp;
    self->clamp_function = _clamp_functions[clamp];

    return 0;
}

static int tweener_clamp_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(tweener_clamp_1o_1s, 1)
        LUAX_OVERLOAD_BY_ARITY(tweener_clamp_2oe_0, 2)
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
        LUAX_OVERLOAD_BY_ARITY(tweener_duration_1o_1n, 1)
        LUAX_OVERLOAD_BY_ARITY(tweener_duration_2on_0, 2)
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
        LUAX_OVERLOAD_BY_ARITY(tweener_range_1o_2nn, 1)
        LUAX_OVERLOAD_BY_ARITY(tweener_range_3onn_0, 3)
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
    float clamped_ratio = self->clamp_function(ratio);
    float eased_ratio = self->easing_function(clamped_ratio);
    float value = FLERP(self->from, self->to, eased_ratio);

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}
