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
#include <core/io/storage.h>
#include <libs/luax.h>
#include <libs/log.h>

#include "udt.h"

typedef enum _Source_Types_t {
    SOURCE_TYPE_MUSIC,
    SOURCE_TYPE_SAMPLE,
    SOURCE_TYPE_MODULE,
    Source_Type_t_CountOf
} Source_Type_t;

typedef SL_Source_t *(*Source_Create_Function_t)(const SL_Context_t *context, SL_Callbacks_t callbacks);

#define LOG_CONTEXT "source"
#define META_TABLE  "Tofu_Sound_Source_mt"

static int source_new(lua_State *L);
static int source_gc(lua_State *L);
static int source_group(lua_State *L);
static int source_looped(lua_State *L);
static int source_mix(lua_State *L);
static int source_pan(lua_State *L);
static int source_balance(lua_State *L);
static int source_gain(lua_State *L);
static int source_speed(lua_State *L);
static int source_play(lua_State *L);
static int source_resume(lua_State *L);
static int source_stop(lua_State *L);
static int source_is_playing(lua_State *L);

static const struct luaL_Reg _source_functions[] = {
    { "new", source_new },
    { "__gc", source_gc },
    { "group", source_group },
    { "looped", source_looped },
    { "mix", source_mix },
    { "pan", source_pan },
    { "balance", source_balance },
    { "gain", source_gain },
    { "speed", source_speed },
    { "play", source_play },
    { "resume", source_resume },
    { "stop", source_stop },
    { "is_playing", source_is_playing },
    { NULL, NULL }
};

static const luaX_Const _source_constants[] = {
    { "MUSIC", LUA_CT_INTEGER, { .i = SOURCE_TYPE_MUSIC } },
    { "SAMPLE", LUA_CT_INTEGER, { .i = SOURCE_TYPE_SAMPLE } },
    { "MODULE", LUA_CT_INTEGER, { .i = SOURCE_TYPE_MODULE } },
    { NULL, LUA_CT_NIL, { 0 } }
};

int source_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _source_functions, _source_constants, nup, META_TABLE);
}

static size_t _handle_read(void *user_data, void *buffer, size_t bytes_to_read)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user_data;
    return FS_read(handle, buffer, bytes_to_read);
}

static bool _handle_seek(void *user_data, long offset, int whence)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user_data;
    return FS_seek(handle, offset, whence);
}

static long _handle_tell(void *user_data)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user_data;
    return FS_tell(handle);
}

static int _handle_eof(void *user_data)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user_data;
    return FS_eof(handle) ? 1 : 0;
}

static const Source_Create_Function_t _create_functions[Source_Type_t_CountOf] = {
    SL_music_create,
    SL_sample_create,
    SL_module_create
};

static int source_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *file = LUAX_STRING(L, 1);
    Source_Type_t type = (Source_Type_t)LUAX_OPTIONAL_INTEGER(L, 2, SOURCE_TYPE_MUSIC);

    const Storage_t *storage = (const Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));
    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    File_System_Handle_t *handle = Storage_open(storage, file); // The handle is kept open, the source could require it.
    if (!handle) {
        return luaL_error(L, "can't access file `%s`", file);
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p opened for file `%s`", handle, file);

    SL_Callbacks_t callbacks = (SL_Callbacks_t){
            .read = _handle_read,
            .seek = _handle_seek,
            .tell = _handle_tell,
            .eof = _handle_eof,
            .user_data = (void *)handle
        };
    SL_Source_t *source = _create_functions[type](audio->sl, callbacks);
    if (!source) {
        FS_close(handle);
        return luaL_error(L, "can't create source");
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p created, type #%d", source, type);

    Source_Object_t *self = (Source_Object_t *)lua_newuserdatauv(L, sizeof(Source_Object_t), 1);
    *self = (Source_Object_t){
            .handle = handle,
            .source = source
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p allocated", self);

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

    Audio_untrack(audio, self->source); // Make sure we aren't leaving dangling pointers...

    SL_source_destroy(self->source);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p destroyed", self->source);

    FS_close(self->handle);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p closed", self->handle);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p finalized", self);

    return 0;
}

static int source_looped1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const Source_Object_t *self = (const Source_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushboolean(L, SL_source_get_looped(self->source));

    return 1;
}

static int source_looped2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    bool looped = LUAX_BOOLEAN(L, 2);

    SL_source_set_looped(self->source, looped);

    return 0;
}

static int source_looped(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, source_looped1)
        LUAX_OVERLOAD_ARITY(2, source_looped2)
    LUAX_OVERLOAD_END
}

static int source_group1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const Source_Object_t *self = (const Source_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushinteger(L, SL_source_get_group(self->source));

    return 1;
}

static int source_group2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    size_t group_id = (size_t)LUAX_INTEGER(L, 2);

    SL_source_set_group(self->source, group_id);

    return 0;
}

static int source_group(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, source_group1)
        LUAX_OVERLOAD_ARITY(2, source_group2)
    LUAX_OVERLOAD_END
}

static int source_mix1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const Source_Object_t *self = (const Source_Object_t *)LUAX_USERDATA(L, 1);

    SL_Mix_t mix = SL_source_get_mix(self->source);

    lua_pushnumber(L, mix.left_to_left);
    lua_pushnumber(L, mix.left_to_right);
    lua_pushnumber(L, mix.right_to_left);
    lua_pushnumber(L, mix.right_to_right);

    return 4;
}

static int source_mix5(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    float left_to_left = LUAX_NUMBER(L, 2);
    float left_to_right = LUAX_NUMBER(L, 3);
    float right_to_left = LUAX_NUMBER(L, 4);
    float right_to_right = LUAX_NUMBER(L, 5);

    SL_source_set_mix(self->source, (SL_Mix_t){
            .left_to_left = left_to_left,
            .left_to_right = left_to_right,
            .right_to_left = right_to_left,
            .right_to_right = right_to_right
        });

    return 0;
}

static int source_mix(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, source_mix1)
        LUAX_OVERLOAD_ARITY(5, source_mix5)
    LUAX_OVERLOAD_END
}

static int source_pan2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    float pan = LUAX_NUMBER(L, 2);

    SL_source_set_pan(self->source, pan);

    return 0;
}

static int source_pan3(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    float left_pan = LUAX_NUMBER(L, 2);
    float right_pan = LUAX_NUMBER(L, 3);

    SL_source_set_twin_pan(self->source, left_pan, right_pan);

    return 0;
}

static int source_pan(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, source_pan2)
        LUAX_OVERLOAD_ARITY(3, source_pan3)
    LUAX_OVERLOAD_END
}

static int source_balance(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    float balance = LUAX_NUMBER(L, 2);

    SL_source_set_balance(self->source, balance);

    return 0;
}

static int source_gain1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const Source_Object_t *self = (const Source_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushnumber(L, SL_source_get_gain(self->source));

    return 1;
}

static int source_gain2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    float gain = LUAX_NUMBER(L, 2);

    SL_source_set_gain(self->source, gain);

    return 0;
}

static int source_gain(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, source_gain1)
        LUAX_OVERLOAD_ARITY(2, source_gain2)
    LUAX_OVERLOAD_END
}

static int source_speed1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    const Source_Object_t *self = (const Source_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushnumber(L, SL_source_get_speed(self->source));

    return 1;
}

static int source_speed2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    float speed = LUAX_NUMBER(L, 2);

    SL_source_set_speed(self->source, speed);

    return 0;
}

static int source_speed(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, source_speed1)
        LUAX_OVERLOAD_ARITY(2, source_speed2)
    LUAX_OVERLOAD_END
}

static int source_play(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_track(audio, self->source, true);

    return 0;
}

static int source_resume(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_track(audio, self->source, false);

    return 0;
}

static int source_stop(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_untrack(audio, self->source);

    return 0;
}

static int source_is_playing(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    lua_pushboolean(L, Audio_is_tracked(audio, self->source));

    return 1;
}
