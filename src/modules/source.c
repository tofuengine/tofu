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

#include "source.h"

#include "internal/udt.h"

#include <core/config.h>
#define _LOG_TAG "source"
#include <libs/log.h>
#include <systems/audio.h>
#include <systems/storage.h>

typedef enum Source_Types_e {
    SOURCE_TYPE_MUSIC,
    SOURCE_TYPE_SAMPLE,
    SOURCE_TYPE_MODULE,
    Source_Type_t_CountOf
} Source_Type_t;

typedef SL_Source_t *(*Source_Create_Function_t)(const SL_Context_t *context, SL_Callbacks_t callbacks);

static int source_new_2sE_1o(lua_State *L);
static int source_gc_1o_0(lua_State *L);
static int source_looped_v_v(lua_State *L);
static int source_group_v_v(lua_State *L);
static int source_mix_v_v(lua_State *L);
static int source_pan_v_0(lua_State *L);
static int source_balance_2on_0(lua_State *L);
static int source_gain_v_v(lua_State *L);
static int source_speed_v_v(lua_State *L);
static int source_is_playing_1o_1b(lua_State *L);
static int source_play_1o_0(lua_State *L);
static int source_resume_1o_0(lua_State *L);
static int source_stop_1o_0(lua_State *L);

int source_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", source_new_2sE_1o },
            { "__gc", source_gc_1o_0 },
            // -- getters/setters --
            { "looped", source_looped_v_v },
            { "group", source_group_v_v },
            { "mix", source_mix_v_v },
            { "pan", source_pan_v_0 },
            { "balance", source_balance_2on_0 },
            { "gain", source_gain_v_v },
            { "speed", source_speed_v_v },
            // -- accessors --
            { "is_playing", source_is_playing_1o_1b },
            // -- operations --
            { "play", source_play_1o_0 },
            { "resume", source_resume_1o_0 },
            { "stop", source_stop_1o_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static size_t _handle_read(void *user_data, void *buffer, size_t bytes_to_read)
{
    FS_Handle_t *handle = (FS_Handle_t *)user_data;
    return FS_read(handle, buffer, bytes_to_read);
}

static bool _handle_seek(void *user_data, long offset, int whence)
{
    FS_Handle_t *handle = (FS_Handle_t *)user_data;
    return FS_seek(handle, offset, whence);
}

static long _handle_tell(void *user_data)
{
    FS_Handle_t *handle = (FS_Handle_t *)user_data;
    return FS_tell(handle);
}

static int _handle_eof(void *user_data)
{
    FS_Handle_t *handle = (FS_Handle_t *)user_data;
    return FS_eof(handle) ? 1 : 0;
}

static const char *_types[Source_Type_t_CountOf + 1] = {
    "music",
    "sample",
    "module",
    NULL
};

static const Source_Create_Function_t _create_functions[Source_Type_t_CountOf] = {
    SL_music_create,
    SL_sample_create,
    SL_module_create
};

static int source_new_2sE_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TENUM)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    Source_Type_t type = (Source_Type_t)LUAX_OPTIONAL_ENUM(L, 2, _types, SOURCE_TYPE_MUSIC);

    const Storage_t *storage = (const Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));
    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    FS_Handle_t *handle = Storage_open(storage, name); // The handle is kept open, the source could require it.
    if (!handle) {
        return luaL_error(L, "can't access file `%s`", name);
    }
    LOG_D("handle %p opened for file `%s`", handle, name);

    SL_Source_t *source = _create_functions[type](audio->context, (SL_Callbacks_t){
            .read = _handle_read,
            .seek = _handle_seek,
            .tell = _handle_tell,
            .eof = _handle_eof,
            .user_data = (void *)handle
        });
    if (!source) {
        FS_close(handle);
        return luaL_error(L, "can't create source");
    }
    LOG_D("source %p created, type #%d", source, type);

    Source_Object_t *self = (Source_Object_t *)udt_newobject(L, sizeof(Source_Object_t), &(Source_Object_t){
            .handle = handle,
            .source = source
        }, OBJECT_TYPE_SOURCE);

    LOG_D("source %p allocated", self);

    return 1;
}

static int source_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_untrack(audio, self->source); // Make sure we aren't leaving dangling pointers...

    SL_source_destroy(self->source);
    LOG_D("source %p destroyed", self->source);

    FS_close(self->handle);
    LOG_D("handle %p closed", self->handle);

    LOG_D("source %p finalized", self);

    return 0;
}

static int source_looped_1o_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Source_Object_t *self = (const Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);

    lua_pushboolean(L, SL_source_get_looped(self->source));

    return 1;
}

static int source_looped_2ob_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);
    bool looped = (bool)LUAX_BOOLEAN(L, 2);

    SL_source_set_looped(self->source, looped);

    return 0;
}

static int source_looped_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(source_looped_1o_1b, 1)
        LUAX_OVERLOAD_BY_ARITY(source_looped_2ob_0, 2)
    LUAX_OVERLOAD_END
}

static int source_group_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Source_Object_t *self = (const Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);

    lua_pushinteger(L, (lua_Integer)SL_source_get_group(self->source));

    return 1;
}

static int source_group_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);
    size_t group_id = LUAX_UNSIGNED(L, 2);

    SL_source_set_group(self->source, group_id);

    return 0;
}

static int source_group_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(source_group_1o_1n, 1)
        LUAX_OVERLOAD_BY_ARITY(source_group_2on_0, 2)
    LUAX_OVERLOAD_END
}

static int source_mix_1o_4nnnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Source_Object_t *self = (const Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);

    SL_Mix_t mix = SL_source_get_mix(self->source);

    lua_pushnumber(L, (lua_Number)mix.left_to_left);
    lua_pushnumber(L, (lua_Number)mix.left_to_right);
    lua_pushnumber(L, (lua_Number)mix.right_to_left);
    lua_pushnumber(L, (lua_Number)mix.right_to_right);

    return 4;
}

static int source_mix_5onnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);
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

static int source_mix_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(source_mix_1o_4nnnn, 1)
        LUAX_OVERLOAD_BY_ARITY(source_mix_5onnnn_0, 5)
    LUAX_OVERLOAD_END
}

static int source_pan_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);
    float pan = LUAX_NUMBER(L, 2);

    SL_source_set_pan(self->source, pan);

    return 0;
}

static int source_pan_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);
    float left_pan = LUAX_NUMBER(L, 2);
    float right_pan = LUAX_NUMBER(L, 3);

    SL_source_set_twin_pan(self->source, left_pan, right_pan);

    return 0;
}

static int source_pan_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(source_pan_2on_0, 2)
        LUAX_OVERLOAD_BY_ARITY(source_pan_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static int source_balance_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);
    float balance = LUAX_NUMBER(L, 2);

    SL_source_set_balance(self->source, balance);

    return 0;
}

static int source_gain_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Source_Object_t *self = (const Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);

    lua_pushnumber(L, (lua_Number)SL_source_get_gain(self->source));

    return 1;
}

static int source_gain_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);
    float gain = LUAX_NUMBER(L, 2);

    SL_source_set_gain(self->source, gain);

    return 0;
}

static int source_gain_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(source_gain_1o_1n, 1)
        LUAX_OVERLOAD_BY_ARITY(source_gain_2on_0, 2)
    LUAX_OVERLOAD_END
}

static int source_speed_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Source_Object_t *self = (const Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);

    lua_pushnumber(L, (lua_Number)SL_source_get_speed(self->source));

    return 1;
}

static int source_speed_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);
    float speed = LUAX_NUMBER(L, 2);

    SL_source_set_speed(self->source, speed);

    return 0;
}

static int source_speed_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(source_speed_1o_1n, 1)
        LUAX_OVERLOAD_BY_ARITY(source_speed_2on_0, 2)
    LUAX_OVERLOAD_END
}

static int source_is_playing_1o_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Source_Object_t *self = (const Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    lua_pushboolean(L, Audio_is_tracked(audio, self->source));

    return 1;
}

static int source_play_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_track(audio, self->source, true);

    return 0;
}

static int source_resume_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_track(audio, self->source, false);

    return 0;
}

static int source_stop_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_SOURCE);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    Audio_untrack(audio, self->source);

    return 0;
}
