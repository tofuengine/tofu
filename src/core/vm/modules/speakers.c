/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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
#include <core/io/audio.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "udt.h"

#include <math.h>
#include <string.h>
#include <time.h>

#define LOG_CONTEXT "speakers"

static int speakers_volume(lua_State *L);
static int speakers_balance(lua_State *L);
static int speakers_gain(lua_State *L);
static int speakers_halt(lua_State *L);

static const struct luaL_Reg _speakers_functions[] = {
    { "volume", speakers_volume },
    { "balance", speakers_balance },
    { "gain", speakers_gain },
    { "halt", speakers_halt },
    { NULL, NULL }
};

static const luaX_Const _speaker_constants[] = {
    { "DEFAULT_GROUP", LUA_CT_INTEGER, { .i = SL_DEFAULT_GROUP } },
    { NULL }
};

int speakers_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _speakers_functions, _speaker_constants, nup, NULL);
}

static int speakers_volume0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    lua_pushnumber(L, audio->volume);

    return 1;
}

static int speakers_volume1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    float volume = LUAX_NUMBER(L, 1);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_volume(audio, volume);

    return 0;
}

static int speakers_volume(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, speakers_volume0)
        LUAX_OVERLOAD_ARITY(1, speakers_volume1)
    LUAX_OVERLOAD_END
}

static int speakers_balance(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t group_id = (size_t)LUAX_INTEGER(L, 1);
    float balance = LUAX_NUMBER(L, 2);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    SL_context_balance(audio->sl, group_id, balance);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group #%d balance is %.f)", group_id, balance);

    return 0;
}

static int speakers_gain(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t group_id = (size_t)LUAX_INTEGER(L, 1);
    float gain = LUAX_NUMBER(L, 2);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    SL_context_gain(audio->sl, group_id, gain);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group #%d gain is %.f", group_id, gain);

    return 0;
}

static int speakers_halt(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_halt(audio);

    return 0;
}