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

#define FNL_IMPL
#include <FastNoiseLite/FastNoiseLite.h>

#include <config.h>
#include <libs/log.h>
#include <libs/luax.h>

#include "udt.h"
#include "utils/map.h"

#define LOG_CONTEXT "noise"
#define META_TABLE  "Tofu_Math_Noise_mt"

static int noise_new_1S_1o(lua_State *L);
static int noise_gc_1o_0(lua_State *L);
static int noise_generate_v_1n(lua_State *L);
// TODO: add noise parameters.

int noise_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", noise_new_1S_1o },
            { "__gc", noise_gc_1o_0 },
            { "generate", noise_generate_v_1n },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static const Map_Entry_t _types[6] = {
    { "cellular", FNL_NOISE_CELLULAR },
    { "open-simplex-2", FNL_NOISE_OPENSIMPLEX2 },
    { "open-simplex-2s", FNL_NOISE_OPENSIMPLEX2S },
    { "perlin", FNL_NOISE_PERLIN },
    { "value", FNL_NOISE_VALUE },
    { "value-cubic", FNL_NOISE_VALUE_CUBIC }
};

static int noise_new_1S_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *type = LUAX_OPTIONAL_STRING(L, 1, "perlin");

    const Map_Entry_t *entry = map_find(L, type, _types, 6);

    fnl_state state = fnlCreateState();
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

    float noise = fnlGetNoise2D(&self->state, (FNLfloat)x, (FNLfloat)y);
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

    float noise = fnlGetNoise3D(&self->state, (FNLfloat)x, (FNLfloat)y, (FNLfloat)z);
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
