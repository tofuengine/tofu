/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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

#include "wave.h"

#include "internal/udt.h"

#include <core/config.h>
#include <libs/log.h>
#include <libs/map.h>

#define LOG_CONTEXT "wave"
#define META_TABLE  "Tofu_Generators_Wave_mt"

static int wave_new_3sNN_1o(lua_State *L);
static int wave_gc_1o_0(lua_State *L);
static int wave_form_v_v(lua_State *L);
static int wave_period_v_v(lua_State *L);
static int wave_amplitude_v_v(lua_State *L);
static int wave_at_2on_1n(lua_State *L);

int wave_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", wave_new_3sNN_1o },
            { "__gc", wave_gc_1o_0 },
            { "__call", wave_at_2on_1n }, // Call meta-method, mapped to `at(...)`.
            { "form", wave_form_v_v },
            { "period", wave_period_v_v },
            { "amplitude", wave_amplitude_v_v },
            { "at", wave_at_2on_1n },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static const Map_Entry_t _forms[Wave_Types_t_CountOf + 1] = {
    { "sine", WAVE_TYPE_SINE },
    { "square", WAVE_TYPE_SQUARE },
    { "triangle", WAVE_TYPE_TRIANGLE },
    { "sawtooth", WAVE_TYPE_SAWTOOTH },
    { NULL, 0 }
};

static const Wave_Function_t _functions[Wave_Types_t_CountOf] = {
    wave_sine,
    wave_square,
    wave_triangle,
    wave_sawtooth
};

static int wave_new_3sNN_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *form = LUAX_STRING(L, 1);
    float period = LUAX_OPTIONAL_NUMBER(L, 2, 1.0f);
    float amplitude = LUAX_OPTIONAL_NUMBER(L, 3, 1.0f);

    const Map_Entry_t *entry = map_find_key(form, _forms);
    if (!entry) {
        return luaL_error(L, "unknown wave form type `%s`", form);
    }

    Wave_Object_t *self = (Wave_Object_t *)luaX_newobject(L, sizeof(Wave_Object_t), &(Wave_Object_t){
            .function = _functions[entry->value],
            .period = period,
            .amplitude = amplitude
        }, OBJECT_TYPE_WAVE, META_TABLE);

    LOG_D(LOG_CONTEXT, "wave %p allocated", self);

    return 1;
}

static int wave_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Wave_Object_t *self = (Wave_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WAVE);

    // Nothing to dispose.

    LOG_D(LOG_CONTEXT, "wave %p finalized", self);

    return 0;
}

// FIXME: implement only the observers? (also for `Tweener`s)
static int wave_form_1o_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const Wave_Object_t *self = (const Wave_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WAVE);

    const Map_Entry_t *entry = map_find_value(self->form, _forms);
    if (!entry) {
        return luaL_error(L, "wave uses unknown form %d", self->form);
    }

    lua_pushstring(L, entry->key);

    return 1;
}

static int wave_form_2os_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    Wave_Object_t *self = (Wave_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WAVE);
    const char *form = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(form, _forms);
    if (!entry) {
        return luaL_error(L, "unknown wave form type `%s`", form);
    }

    self->function = _functions[entry->value];

    return 0;
}

static int wave_form_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, wave_form_1o_1s)
        LUAX_OVERLOAD_ARITY(2, wave_form_2os_0)
    LUAX_OVERLOAD_END
}

static int wave_period_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Wave_Object_t *self = (const Wave_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WAVE);

    const float period = self->period;

    lua_pushnumber(L, (lua_Number)period);

    return 1;
}

static int wave_period_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Wave_Object_t *self = (Wave_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WAVE);
    float period = LUAX_NUMBER(L, 2);

    self->period = period;

    return 0;
}

static int wave_period_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, wave_period_1o_1n)
        LUAX_OVERLOAD_ARITY(2, wave_period_2on_0)
    LUAX_OVERLOAD_END
}

static int wave_amplitude_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Wave_Object_t *self = (const Wave_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WAVE);

    const float amplitude = self->amplitude;

    lua_pushnumber(L, (lua_Number)amplitude);

    return 1;
}

static int wave_amplitude_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Wave_Object_t *self = (Wave_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WAVE);
    float amplitude = LUAX_NUMBER(L, 2);

    self->amplitude = amplitude;

    return 0;
}

static int wave_amplitude_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, wave_amplitude_1o_1n)
        LUAX_OVERLOAD_ARITY(2, wave_amplitude_2on_0)
    LUAX_OVERLOAD_END
}

static int wave_at_2on_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Wave_Object_t *self = (const Wave_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WAVE);
    float time = LUAX_NUMBER(L, 2);

    float ratio = time / self->period;
    float value = self->function(ratio) * self->amplitude;

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}
