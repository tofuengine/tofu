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

#include "wave.h"

#include <config.h>
#include <libs/log.h>
#include <libs/luax.h>

#include "udt.h"

#define LOG_CONTEXT "wave"
#define META_TABLE  "Tofu_Math_Wave_mt"

static int wave_new_3sNN_1o(lua_State *L);
static int wave_gc_1o_0(lua_State *L);
static int wave_at_2on_1n(lua_State *L);
// TODO: add wave parameters.

int wave_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", wave_new_3sNN_1o },
            { "__gc", wave_gc_1o_0 },
            { "__call", wave_at_2on_1n }, // Call meta-method, mapped to `at(...)`.
            { "at", wave_at_2on_1n },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int wave_new_3sNN_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    float period = LUAX_OPTIONAL_NUMBER(L, 2, 1.0f);
    float amplitude = LUAX_OPTIONAL_NUMBER(L, 3, 1.0f);

    const Wave_t *wave = wave_from_name(name);
    if (!wave) {
        return luaL_error(L, "can't find wave w/ name `%s`", wave);
    }

    Wave_Object_t *self = (Wave_Object_t *)luaX_newobject(L, sizeof(Wave_Object_t), &(Wave_Object_t){
            .function = wave->function,
            .period = period,
            .amplitude = amplitude
        }, OBJECT_TYPE_WAVE, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "wave %p allocated", self);

    return 1;
}

static int wave_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Wave_Object_t *self = (Wave_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WAVE);

    // Nothing to dispose.

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "wave %p finalized", self);

    return 0;
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
