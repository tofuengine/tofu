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

#include "noise.h"

#include <config.h>
#include <libs/log.h>
#include <libs/luax.h>

#include "udt.h"
#include "utils/map.h"

#define LOG_CONTEXT "noise"
#define META_TABLE  "Tofu_Generators_Noise_mt"

static int noise_new_1sNN_1o(lua_State *L);
static int noise_gc_1o_0(lua_State *L);
static int noise_type_v_v(lua_State *L);
static int noise_seed_v_v(lua_State *L);
static int noise_frequency_v_v(lua_State *L);
static int noise_generate_3onNN_1n(lua_State *L);

int noise_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", noise_new_1sNN_1o },
            { "__gc", noise_gc_1o_0 },
            { "__call", noise_generate_3onNN_1n }, // Call meta-method, mapped to `generate(...)`.
            { "type", noise_type_v_v },
            { "seed", noise_seed_v_v },
            { "frequency", noise_frequency_v_v },
            { "generate", noise_generate_3onNN_1n },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static const Noise_Function_t _functions[Noise_Types_t_CountOf] = {
    noise_perlin,
    noise_simplex,
    noise_cellular
};

static const Map_Entry_t _types[Noise_Types_t_CountOf] = {
    { "perlin", NOISE_TYPE_PERLIN },
    { "simplex", NOISE_TYPE_SIMPLEX },
    { "cellular", NOISE_TYPE_CELLULAR }
};

static int noise_new_1sNN_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *type = LUAX_STRING(L, 1);
    float seed = LUAX_OPTIONAL_NUMBER(L, 2, 0.0f);
    float frequency = LUAX_OPTIONAL_NUMBER(L, 3, 1.0f);

    const Map_Entry_t *entry = map_find_key(L, type, _types, Noise_Types_t_CountOf);

    Noise_Object_t *self = (Noise_Object_t *)luaX_newobject(L, sizeof(Noise_Object_t), &(Noise_Object_t){
            .type = (Noise_Types_t)entry->value,
            .function = _functions[entry->value],
            .seed = seed,
            .frequency = frequency
        }, OBJECT_TYPE_NOISE, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "noise %p allocated", self);

    return 1;
}

static int noise_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Noise_Object_t *self = (Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);

    // Nothing to dispose.

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "noise %p finalized", self);

    return 0;
}

static int noise_type_1o_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Noise_Object_t *self = (const Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);

    const Map_Entry_t *entry = map_find_value(L, self->type, _types, 6);

    lua_pushstring(L, entry->key);

    return 1;
}

static int noise_type_2os_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    Noise_Object_t *self = (Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);
    const char *type = LUAX_STRING(L, 2);

    const Map_Entry_t *entry = map_find_key(L, type, _types, 6);
    self->type = (Noise_Types_t)entry->value;
    self->function = _functions[entry->value];

    return 0;
}

static int noise_type_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, noise_type_1o_1s)
        LUAX_OVERLOAD_ARITY(2, noise_type_2os_0)
    LUAX_OVERLOAD_END
}

static int noise_seed_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Noise_Object_t *self = (const Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);

    const float seed = self->seed;
    lua_pushnumber(L, (lua_Number)seed);

    return 1;
}

static int noise_seed_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Noise_Object_t *self = (Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);
    float seed = LUAX_NUMBER(L, 2);

    self->seed = seed;

    return 0;
}

static int noise_seed_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, noise_seed_1o_1n)
        LUAX_OVERLOAD_ARITY(2, noise_seed_2on_0)
    LUAX_OVERLOAD_END
}

static int noise_frequency_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Noise_Object_t *self = (const Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);

    const float frequency = self->frequency;
    lua_pushnumber(L, (lua_Number)frequency);

    return 1;
}

static int noise_frequency_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Noise_Object_t *self = (Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);
    float frequency = LUAX_NUMBER(L, 2);

    self->frequency = frequency;

    return 0;
}

static int noise_frequency_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, noise_frequency_1o_1n)
        LUAX_OVERLOAD_ARITY(2, noise_frequency_2on_0)
    LUAX_OVERLOAD_END
}

static int noise_generate_3onNN_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Noise_Object_t *self = (const Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);
    float x = LUAX_NUMBER(L, 2);
    float y = LUAX_OPTIONAL_NUMBER(L, 3, 0.0f);
    float z = LUAX_OPTIONAL_NUMBER(L, 4, 0.0f);

    float seed = self->seed;
    float frequency = self->frequency;
    float noise = self->function(x * frequency + seed, y * frequency + seed, z * frequency + seed);
    float value = (noise + 1.0f) * 0.5f;

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}