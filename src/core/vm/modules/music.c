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

#include "music.h"

#include <config.h>
#include <core/io/audio.h>
#include <libs/log.h>
#include <libs/stb.h>

#include "udt.h"

#define DR_FLAC_IMPLEMENTATION
#include <dr_libs/dr_flac.h>

#define LOG_CONTEXT "source"
#define META_TABLE  "Tofu_Sound_Music_mt"

static int music_new(lua_State *L);
static int music_gc(lua_State *L);
static int music_group(lua_State *L);
static int music_looped(lua_State *L);
static int music_gain(lua_State *L);
static int music_pan(lua_State *L);
static int music_speed(lua_State *L);
static int music_play(lua_State *L);
static int music_stop(lua_State *L);
static int music_rewind(lua_State *L);
static int music_is_playing(lua_State *L);

static const struct luaL_Reg _music_functions[] = {
    { "new", music_new },
    { "__gc", music_gc },
    { "group", music_group },
    { "looped", music_looped },
    { "gain", music_gain },
    { "pan", music_pan },
    { "speed", music_speed },
    { "play", music_play },
    { "stop", music_stop },
    { "rewind", music_rewind },
    { "is_playing", music_is_playing },
    { NULL, NULL }
};

int music_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, NULL, _music_functions, NULL, nup, META_TABLE);
}

static size_t _handle_read(void *user_data, void *buffer, size_t bytes_to_read)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user_data;
    return FS_read(handle, buffer, bytes_to_read);
}

static drflac_bool32 _handle_seek(void *user_data, int offset, drflac_seek_origin origin)
{
    File_System_Handle_t *handle = (File_System_Handle_t *)user_data;
    if (origin == drflac_seek_origin_start) {
        FS_seek(handle, offset, SEEK_SET);
    } else
    if (origin == drflac_seek_origin_current) {
        FS_seek(handle, offset, SEEK_CUR);
    }
    return true;
}

static size_t _decoder_read(void *user_data, void *output, size_t frames_requested)
{
    drflac *decoder = (drflac *)user_data;
    return drflac_read_pcm_frames_s16(decoder, frames_requested, output); // Read from the internal format to s16, forced.
}

static void _decoder_seek(void *user_data, size_t frame_offset)
{
    drflac *decoder = (drflac *)user_data;
    drflac_seek_to_pcm_frame(decoder, frame_offset);
}

static int music_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *file = LUAX_STRING(L, 1);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));
    File_System_t *file_system = (File_System_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_FILE_SYSTEM));

    File_System_Handle_t *handle = FS_locate_and_open(file_system, file);
    if (!handle) {
        return luaL_error(L, "can't access file `%s`", file);
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p opened for file `%s`", handle, file);

    drflac *decoder = drflac_open(_handle_read, _handle_seek, (void *)handle, NULL);
    if (!decoder) {
        FS_close(handle);
        return luaL_error(L, "can't open decoder for file `%s`", file);
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "decoder %p opened", decoder);

    SL_Source_t *source = SL_music_create(_decoder_read, _decoder_seek, (void *)decoder, ma_format_s16, decoder->sampleRate, decoder->channels);
    if (!source) { // We are forcing 16 bits-per-sample.
        free(decoder);
        FS_close(handle);
        return luaL_error(L, "can't create source");
    }

    SL_Context_t *context = Audio_lock(audio);
    SL_context_track(context, source);
    Audio_unlock(audio, context);

    Music_Object_t *self = (Music_Object_t *)lua_newuserdata(L, sizeof(Music_Object_t));
    *self = (Music_Object_t){
            .handle = handle,
            .decoder = decoder,
            .source = source
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p allocated", self);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int music_gc(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);

    Audio_t *audio = (Audio_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_AUDIO));

    SL_Context_t *context = Audio_lock(audio);
    SL_context_untrack(context, self->source);
    Audio_unlock(audio, context);

    SL_source_destroy(self->source);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p destroyed", self->source);

    FS_close(self->handle);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p closed", self->handle);

    drflac_close(self->decoder);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "decoder closed", self->decoder);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "music %p finalized", self);

    return 0;
}

static int music_looped1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushboolean(L, SL_source_get_looped(self->source));

    return 1;
}

static int music_looped2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);
    bool looped = LUAX_BOOLEAN(L, 2);

    SL_source_set_looped(self->source, looped);

    return 0;
}

static int music_looped(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, music_looped1)
        LUAX_OVERLOAD_ARITY(2, music_looped2)
    LUAX_OVERLOAD_END
}

static int music_group1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushinteger(L, SL_source_get_group(self->source));

    return 1;
}

static int music_group2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);
    size_t group = LUAX_INTEGER(L, 2);

    SL_source_set_group(self->source, group);

    return 0;
}

static int music_group(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, music_group1)
        LUAX_OVERLOAD_ARITY(2, music_group2)
    LUAX_OVERLOAD_END
}

static int music_gain1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushnumber(L, SL_source_get_gain(self->source));

    return 1;
}

static int music_gain2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);
    float gain = LUAX_NUMBER(L, 2);

    SL_source_set_gain(self->source, gain);

    return 0;
}

static int music_gain(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, music_gain1)
        LUAX_OVERLOAD_ARITY(2, music_gain2)
    LUAX_OVERLOAD_END
}

static int music_pan1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushnumber(L, SL_source_get_pan(self->source));

    return 1;
}

static int music_pan2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);
    float pan = LUAX_NUMBER(L, 2);

    SL_source_set_pan(self->source, pan);

    return 0;
}

static int music_pan(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, music_pan1)
        LUAX_OVERLOAD_ARITY(2, music_pan2)
    LUAX_OVERLOAD_END
}

static int music_speed1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushnumber(L, SL_source_get_speed(self->music));

    return 1;
}

static int music_speed2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);
    float speed = LUAX_NUMBER(L, 2);

    SL_source_set_speed(self->source, speed);

    return 0;
}

static int music_speed(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, music_speed1)
        LUAX_OVERLOAD_ARITY(2, music_speed2)
    LUAX_OVERLOAD_END
}

static int music_play(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);

    SL_source_play(self->source);

    return 0;
}

static int music_stop(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);

    SL_source_stop(self->source);

    return 0;
}

static int music_rewind(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);

    SL_source_rewind(self->source);

    return 0;
}

static int music_is_playing(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Music_Object_t *self = (Music_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushboolean(L, SL_source_is_playing(self->source));

    return 1;
}