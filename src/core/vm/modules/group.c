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

#include "group.h"

#include <config.h>
#include <core/io/audio.h>
#include <libs/log.h>

#include "udt.h"

#define LOG_CONTEXT "group"
#define META_TABLE  "Tofu_Sound_Group_mt"

static int group_new(lua_State *L);
static int group_gc(lua_State *L);
static int group_gain(lua_State *L);
static int group_pan(lua_State *L);
static int group_reset(lua_State *L);

static const struct luaL_Reg _group_functions[] = {
    { "new", group_new },
    { "__gc", group_gc },
    { "gain", group_gain },
    { "pan", group_pan },
    { "reset", group_reset },
    { NULL, NULL }
};

static const uint8_t _group_lua[] = {
#include "group.inc"
};

static luaX_Script _group_script = { (const char *)_group_lua, sizeof(_group_lua), "@group.lua" }; // Trace as filename internally.

int group_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_group_script, _group_functions, NULL, nup, NULL);
}

static int group_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    SL_Group_t *group = SL_group_create();
    if (!group) {
        return luaL_error(L, "can't create group");
    }

    SL_context_track(audio->sl, group);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group %p tracked for context %p", group, audio->context);

    Group_Class_t *self = (Group_Class_t *)lua_newuserdata(L, sizeof(Group_Class_t));
    *self = (Group_Class_t){
            .group = group
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group %p allocated w/ group %p", self, group);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int group_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Group_Class_t *self = (Group_Class_t *)LUAX_USERDATA(L, 1);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    SL_context_untrack(audio->sl, self->group);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group %p untracked", self->group);

    SL_group_destroy(self->group);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group %p freed", self->group);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group %p finalized", self);

    return 0;
}

static int group_gain(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Group_Class_t *self = (Group_Class_t *)LUAX_USERDATA(L, 1);
    float gain = LUAX_NUMBER(L, 2);

    SL_group_gain(self->group, gain);

    return 0;
}

static int group_pan(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Group_Class_t *self = (Group_Class_t *)LUAX_USERDATA(L, 1);
    float pan = LUAX_NUMBER(L, 2);

    SL_group_pan(self->group, pan);

    return 0;
}

static int group_reset(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Group_Class_t *self = (Group_Class_t *)LUAX_USERDATA(L, 1);

    SL_group_reset(self->group);

    return 0;
}
