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

#include "source.h"

#include <config.h>
#include <core/io/audio.h>
#include <libs/log.h>

#include "udt.h"

#define DR_WAV_IMPLEMENTATION
#include <miniaudio/extras/dr_wav.h>

#define LOG_CONTEXT "source"
#define META_TABLE  "Tofu_Sound_Source_mt"

static int source_new(lua_State *L);
static int source_gc(lua_State *L);
static int source_looped(lua_State *L);
static int source_gain(lua_State *L);
static int source_pan(lua_State *L);
static int source_speed(lua_State *L);
static int source_delay(lua_State *L);
static int source_pause(lua_State *L);
static int source_resume(lua_State *L);
static int source_stop(lua_State *L);

static const struct luaL_Reg _source_functions[] = {
    { "new", source_new },
    { "__gc", source_gc },
    { "looped", source_looped },
    { "gain", source_gain },
    { "pan", source_pan },
    { "speed", source_speed },
    { "delay", source_delay },
    { "pause", source_pause },
    { "resume", source_resume },
    { "stop", source_stop },
    { NULL, NULL }
};

int source_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _source_functions, NULL, nup, NULL);
}

static size_t _source_read(void *user_data, void *output, size_t bytes_requested)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user_data;

    return FS_read(handle, output, bytes_requested);
}

static void _source_seek(void *user_data, long position, int whence)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user_data;

    FS_skip(handle, position);
}

static int source_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const char *file = LUAX_STRING(L, 1);
    Group_Object_t *group = (Group_Object_t *)LUAX_USERDATA(L, 2);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));
    File_System_t *file_system = (File_System_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));

    File_System_Handle_t *handle = FS_locate_and_open(file_system, file);
    if (!handle) {
        return luaL_error(L, "can't access file `%s`", file);
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p opened for file `%s`", handle, file);

    SL_Source_t *source = SL_source_create(_source_read, _source_seek, (void *)handle);
    if (!source) {
        FS_close(handle);
        return luaL_error(L, "can't create source");
    }

    SL_Context_t *context = Audio_lock(audio);
    SL_group_track(group->group, source);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p tracked for group %p (context %p)", source, group->group, context);
    Audio_unlock(audio, context);

    Source_Object_t *self = (Source_Object_t *)lua_newuserdata(L, sizeof(Source_Object_t));
    *self = (Source_Object_t){
            .group = group->group,
            .group_reference = luaX_ref(L, 2),
            .handle = handle,
            .source = source
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p allocated w/ group %p", self, group);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int source_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    SL_Context_t *context = Audio_lock(audio);
    SL_group_untrack(self->group, self->source);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p untracked", self->source);
    Audio_unlock(audio, context);

    if (self->group_reference != LUAX_REFERENCE_NIL) { // TODO: handle default group case?
        luaX_unref(L, self->group_reference);
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "group reference #%d released", self->group_reference);
    }

    SL_source_destroy(self->source);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p destroyed", self->source);

    FS_close(self->handle);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p closed", self->handle);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p finalized", self);

    return 0;
}

static int source_looped(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    bool looped = LUAX_BOOLEAN(L, 2);

    SL_source_looped(self->source, looped);

    return 0;
}

static int source_gain(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    float gain = LUAX_NUMBER(L, 2);

    SL_source_gain(self->source, gain);

    return 0;
}

static int source_pan(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    float pan = LUAX_NUMBER(L, 2);

    SL_source_pan(self->source, pan);

    return 0;
}

static int source_speed(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    float speed = LUAX_NUMBER(L, 2);

    SL_source_speed(self->source, speed);

    return 0;
}

static int source_delay(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    float delay = LUAX_NUMBER(L, 2);

    SL_source_delay(self->source, delay);

    return 0;
}

static int source_pause(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    SL_source_pause(self->source);

    return 0;
}

static int source_resume(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    SL_source_resume(self->source);

    return 0;
}

static int source_stop(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    SL_source_stop(self->source);

    return 0;
}