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
#define META_TABLE  "Tofu_Math_Noise_mt"

static int noise_new_1S_1o(lua_State *L);
static int noise_gc_1o_0(lua_State *L);
static int noise_seed_v_v(lua_State *L);
static int noise_frequency_v_v(lua_State *L);
static int noise_rotation_v_v(lua_State *L);
static int noise_fractal_v_v(lua_State *L);
static int noise_warp_v_1n(lua_State *L);
static int noise_generate_v_1n(lua_State *L);
// TODO: add noise parameters.

int noise_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", noise_new_1S_1o },
            { "__gc", noise_gc_1o_0 },
            { "__call", noise_generate_v_1n }, // Call meta-method, mapped to `generate(...)`.
            { "seed", noise_seed_v_v },
            { "frequency", noise_frequency_v_v },
            { "rotation", noise_rotation_v_v },
            { "fractal", noise_fractal_v_v },
            { "warp", noise_warp_v_1n },
            { "generate", noise_generate_v_1n },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static const Map_Entry_t _types[6] = {
    // TODO: add a "default" case?
    { "open-simplex-2", FNL_NOISE_OPENSIMPLEX2 },
    { "open-simplex-2s", FNL_NOISE_OPENSIMPLEX2S },
    { "cellular", FNL_NOISE_CELLULAR },
    { "perlin", FNL_NOISE_PERLIN },
    { "value-cubic", FNL_NOISE_VALUE_CUBIC },
    { "value", FNL_NOISE_VALUE }
};

static int noise_new_1S_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *type = LUAX_OPTIONAL_STRING(L, 1, "perlin");

    fnl_state state = fnlCreateState();
    const Map_Entry_t *entry = map_find_key(L, type, _types, 6);
    state.noise_type = (fnl_noise_type)entry->value;

    Noise_Object_t *self = (Noise_Object_t *)luaX_newobject(L, sizeof(Noise_Object_t), &(Noise_Object_t){
            .state = state
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

static int noise_seed_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Noise_Object_t *self = (const Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);

    const fnl_state *state = &self->state;
    const int seed = state->seed;

    lua_pushinteger(L, (lua_Integer)seed);

    return 1;
}

static int noise_seed_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Noise_Object_t *self = (Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);
    int seed = LUAX_INTEGER(L, 2);

    fnl_state *state = &self->state;
    state->seed = seed;

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
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Noise_Object_t *self = (const Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);

    const fnl_state *state = &self->state;
    const float frequency = state->frequency;

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

    fnl_state *state = &self->state;
    state->frequency = frequency;

    return 0;
}

static int noise_frequency_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, noise_frequency_1o_1n)
        LUAX_OVERLOAD_ARITY(2, noise_frequency_2on_0)
    LUAX_OVERLOAD_END
}

static const Map_Entry_t _rotations[3] = {
    // TODO: add a "default" case?
    { "none", FNL_ROTATION_NONE },
    { "improve-xy", FNL_ROTATION_IMPROVE_XY_PLANES },
    { "improve-xz", FNL_ROTATION_IMPROVE_XZ_PLANES }
};

static int noise_rotation_1o_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Noise_Object_t *self = (const Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);

    const fnl_state *state = &self->state;
    const fnl_rotation_type_3d rotation = state->rotation_type_3d;
    const Map_Entry_t *entry = map_find_value(L, (Map_Entry_Value_t)rotation, _rotations, 3);

    lua_pushstring(L, entry->key);

    return 1;
}

static int noise_rotation_2os_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    Noise_Object_t *self = (Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);
    const char *rotation = LUAX_STRING(L, 2);

    fnl_state *state = &self->state;
    const Map_Entry_t *entry = map_find_key(L, rotation, _rotations, 3);
    state->rotation_type_3d = (fnl_rotation_type_3d)entry->value;

    return 0;
}

static int noise_rotation_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, noise_rotation_1o_1s)
        LUAX_OVERLOAD_ARITY(2, noise_rotation_2os_0)
    LUAX_OVERLOAD_END
}

static const Map_Entry_t _fractals[6] = {
    // TODO: add a "default" case?
    { "none", FNL_FRACTAL_NONE },
    { "fbm", FNL_FRACTAL_FBM },
    { "ridged", FNL_FRACTAL_RIDGED },
    { "ping-pong", FNL_FRACTAL_PINGPONG },
    { "domain-warp-progressive", FNL_FRACTAL_DOMAIN_WARP_PROGRESSIVE },
    { "domain-warp-independent", FNL_FRACTAL_DOMAIN_WARP_INDEPENDENT }
};

static int noise_fractal_1o_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Noise_Object_t *self = (const Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);

    const fnl_state *state = &self->state;
    const fnl_fractal_type fractal = state->fractal_type;
    const Map_Entry_t *entry = map_find_value(L, (Map_Entry_Value_t)fractal, _fractals, 6);

    lua_pushstring(L, entry->key);

    return 1;
}

static int noise_fractal_2os_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    Noise_Object_t *self = (Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);
    const char *fractal = LUAX_STRING(L, 2);

    fnl_state *state = &self->state;
    const Map_Entry_t *entry = map_find_key(L, fractal, _fractals, 6);
    state->fractal_type = (fnl_fractal_type)entry->value;

    return 0;
}

static int noise_fractal_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, noise_fractal_1o_1s)
        LUAX_OVERLOAD_ARITY(2, noise_fractal_2os_0)
    LUAX_OVERLOAD_END
}

static int noise_warp_3onn_2n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Noise_Object_t *self = (Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);
    float x = LUAX_NUMBER(L, 2);
    float y = LUAX_NUMBER(L, 3);

    fnl_state *state = &self->state;
    fnlDomainWarp2D(state, &x, &y); // FIXME: could be const?

    lua_pushnumber(L, (lua_Number)x);
    lua_pushnumber(L, (lua_Number)y);

    return 2;
}

static int noise_warp_4onnn_3n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Noise_Object_t *self = (Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);
    float x = LUAX_NUMBER(L, 2);
    float y = LUAX_NUMBER(L, 3);
    float z = LUAX_NUMBER(L, 4);

    fnl_state *state = &self->state;
    fnlDomainWarp3D(state, &x, &y, &z);

    lua_pushnumber(L, (lua_Number)x);
    lua_pushnumber(L, (lua_Number)y);
    lua_pushnumber(L, (lua_Number)z);

    return 3;
}

static int noise_warp_v_1n(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, noise_warp_3onn_2n)
        LUAX_OVERLOAD_ARITY(4, noise_warp_4onnn_3n)
    LUAX_OVERLOAD_END
}

static int noise_generate_3onn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Noise_Object_t *self = (Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);
    float x = LUAX_NUMBER(L, 2);
    float y = LUAX_NUMBER(L, 3);

    fnl_state *state = &self->state;
    float noise = fnlGetNoise2D(state, (FNLfloat)x, (FNLfloat)y);
    float value = (noise + 1.0f) * 0.5f;

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int noise_generate_4onnn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Noise_Object_t *self = (Noise_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_NOISE);
    float x = LUAX_NUMBER(L, 2);
    float y = LUAX_NUMBER(L, 3);
    float z = LUAX_NUMBER(L, 4);

    fnl_state *state = &self->state;
    float noise = fnlGetNoise3D(state, (FNLfloat)x, (FNLfloat)y, (FNLfloat)z);
    float value = (noise + 1.0f) * 0.5f;

    lua_pushnumber(L, (lua_Number)value);

    return 1;
}

static int noise_generate_v_1n(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(3, noise_generate_3onn_1n)
        LUAX_OVERLOAD_ARITY(4, noise_generate_4onnn_1n)
    LUAX_OVERLOAD_END
}
