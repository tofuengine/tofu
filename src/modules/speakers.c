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

#include "speakers.h"

#include <config.h>
#include <libs/log.h>
#include <libs/luax.h>
#include <systems/audio.h>

#include "udt.h"

#define LOG_CONTEXT "speakers"

static int speakers_volume_v_v(lua_State *L);
static int speakers_mix_v_v(lua_State *L);
static int speakers_pan_2nn_0(lua_State *L);
static int speakers_balance_2nn_0(lua_State *L);
static int speakers_gain_v_v(lua_State *L);
static int speakers_halt_0_0(lua_State *L);

int speakers_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "volume", speakers_volume_v_v },
            { "mix", speakers_mix_v_v },
            { "pan", speakers_pan_2nn_0 },
            { "balance", speakers_balance_2nn_0 },
            { "gain", speakers_gain_v_v },
            { "halt", speakers_halt_0_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { "DEFAULT_GROUP", LUA_CT_INTEGER, { .i = SL_DEFAULT_GROUP } },
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, NULL);
}

static int speakers_volume_0_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Audio_t *audio = (const Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    lua_pushnumber(L, (lua_Number)Audio_get_volume(audio));

    return 1;
}

static int speakers_volume_1n_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float volume = LUAX_NUMBER(L, 1);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_set_volume(audio, volume);

    return 0;
}

static int speakers_volume_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, speakers_volume_0_1n)
        LUAX_OVERLOAD_ARITY(1, speakers_volume_1n_0)
    LUAX_OVERLOAD_END
}

static int speakers_mix_1n_4nnnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t group_id = LUAX_UNSIGNED(L, 1);

    const Audio_t *audio = (const Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    SL_Mix_t mix = Audio_get_mix(audio, group_id);

    lua_pushnumber(L, (lua_Number)mix.left_to_left);
    lua_pushnumber(L, (lua_Number)mix.left_to_right);
    lua_pushnumber(L, (lua_Number)mix.right_to_left);
    lua_pushnumber(L, (lua_Number)mix.right_to_right);

    return 4;
}

static int speakers_mix_5nnnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t group_id = LUAX_UNSIGNED(L, 1);
    float left_to_left = LUAX_NUMBER(L, 2);
    float left_to_right = LUAX_NUMBER(L, 3);
    float right_to_left = LUAX_NUMBER(L, 4);
    float right_to_right = LUAX_NUMBER(L, 5);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_set_mix(audio, group_id, (SL_Mix_t){
            .left_to_left = left_to_left,
            .left_to_right = left_to_right,
            .right_to_left = right_to_left,
            .right_to_right = right_to_right
        });
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group #%d mix is [%.f, %.f, %.f, %.f]", group_id, left_to_left, left_to_right, right_to_left, right_to_right);

    return 0;
}

static int speakers_mix_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, speakers_mix_1n_4nnnn)
        LUAX_OVERLOAD_ARITY(5, speakers_mix_5nnnnn_0)
    LUAX_OVERLOAD_END
}

static int speakers_pan_2nn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t group_id = LUAX_UNSIGNED(L, 1);
    float pan = LUAX_NUMBER(L, 2);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_set_pan(audio, group_id, pan);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group #%d pan is %.f", group_id, pan);

    return 0;
}

static int speakers_balance_2nn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t group_id = LUAX_UNSIGNED(L, 1);
    float balance = LUAX_NUMBER(L, 2);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_set_balance(audio, group_id, balance);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group #%d balance is %.f", group_id, balance);

    return 0;
}

static int speakers_gain_1n_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t group_id = LUAX_UNSIGNED(L, 1);

    const Audio_t *audio = (const Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    lua_pushnumber(L, (lua_Number)Audio_get_gain(audio, group_id));

    return 1;
}

static int speakers_gain_2nn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t group_id = LUAX_UNSIGNED(L, 1);
    float gain = LUAX_NUMBER(L, 2);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_set_gain(audio, group_id, gain);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group #%d gain is %.f", group_id, gain);

    return 0;
}

static int speakers_gain_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, speakers_gain_1n_1n)
        LUAX_OVERLOAD_ARITY(2, speakers_gain_2nn_0)
    LUAX_OVERLOAD_END
}

static int speakers_halt_0_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_halt(audio);

    return 0;
}
