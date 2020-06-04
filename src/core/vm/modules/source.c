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
#include <libs/stb.h>

#include "udt.h"

#define DR_FLAC_IMPLEMENTATION
#include <dr_libs/dr_flac.h>

typedef enum _Source_Types_t {
    SOURCE_TYPE_MUSIC,
    SOURCE_TYPE_SAMPLE,
} Source_Type_t;

#if SL_BYTES_PER_FRAME == 2
  #define INTERNAL_FORMAT   ma_format_s16
#elif SL_BYTES_PER_FRAME == 4
  #define INTERNAL_FORMAT   ma_format_f32
#else
  #error Wrong internal format.
#endif

#define LOG_CONTEXT "source"
#define META_TABLE  "Tofu_Sound_Source_mt"

static int source_new(lua_State *L);
static int source_gc(lua_State *L);
static int source_group(lua_State *L);
static int source_looping(lua_State *L);
static int source_gain(lua_State *L);
static int source_pan(lua_State *L);
static int source_speed(lua_State *L);
static int source_play(lua_State *L);
static int source_stop(lua_State *L);
static int source_rewind(lua_State *L);
static int source_is_playing(lua_State *L);

static const struct luaL_Reg _source_functions[] = {
    { "new", source_new },
    { "__gc", source_gc },
    { "group", source_group },
    { "looping", source_looping },
    { "gain", source_gain },
    { "pan", source_pan },
    { "speed", source_speed },
    { "play", source_play },
    { "stop", source_stop },
    { "rewind", source_rewind },
    { "is_playing", source_is_playing },
    { NULL, NULL }
};

static const luaX_Const _source_constants[] = {
    { "MUSIC", LUA_CT_INTEGER, { .i = SOURCE_TYPE_MUSIC } },
    { "SAMPLE", LUA_CT_INTEGER, { .i = SOURCE_TYPE_SAMPLE } },
    { NULL }
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
#if SL_BYTES_PER_FRAME == 2
    return drflac_read_pcm_frames_s16(decoder, frames_requested, output);
#elif SL_BYTES_PER_FRAME == 4
    return drflac_read_pcm_frames_f32(decoder, frames_requested, output);
#endif
}

static void _decoder_seek(void *user_data, size_t frame_offset)
{
    drflac *decoder = (drflac *)user_data;
    drflac_seek_to_pcm_frame(decoder, frame_offset);
}

static int source_new(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *file = LUAX_STRING(L, 1);
    Source_Type_t type = (Source_Type_t)LUAX_OPTIONAL_INTEGER(L, 2, SOURCE_TYPE_MUSIC);

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

    size_t length_in_frames = decoder->totalPCMFrameCount;
    if (length_in_frames == 0) {
        drflac_close(decoder);
        FS_close(handle);
        return luaL_error(L, "source w/ zero length");
    }
    size_t channels = decoder->channels;
    size_t sample_rate = decoder->sampleRate;
    size_t bits_per_sample = decoder->bitsPerSample;
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source `%s` w/ %d frames, %d channels, %dHz, %d bits", file, length_in_frames, channels, sample_rate, bits_per_sample);

    SL_Source_t *source = type == SOURCE_TYPE_MUSIC
        ? SL_music_create(_decoder_read, _decoder_seek, (void *)decoder, length_in_frames, INTERNAL_FORMAT, sample_rate, channels)
        : SL_sample_create(_decoder_read, (void *)decoder, length_in_frames, INTERNAL_FORMAT, sample_rate, channels);
    if (!source) {
        drflac_close(decoder);
        FS_close(handle);
        return luaL_error(L, "can't create source");
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p create, type #%d", source, type);

    Audio_track(audio, source);

    Source_Object_t *self = (Source_Object_t *)lua_newuserdata(L, sizeof(Source_Object_t));
    *self = (Source_Object_t){
            .handle = handle,
            .decoder = decoder,
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

    Audio_untrack(audio, self->source);

    SL_source_destroy(self->source);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p destroyed", self->source);

    FS_close(self->handle);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "handle %p closed", self->handle);

    drflac_close(self->decoder);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "decoder closed", self->decoder);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "source %p finalized", self);

    return 0;
}

static int source_looping1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushboolean(L, SL_source_get_looping(self->source));

    return 1;
}

static int source_looping2(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);
    bool looping = LUAX_BOOLEAN(L, 2);

    SL_source_set_looping(self->source, looping);

    return 0;
}

static int source_looping(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, source_looping1)
        LUAX_OVERLOAD_ARITY(2, source_looping2)
    LUAX_OVERLOAD_END
}

static int source_group1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

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
    size_t group = LUAX_INTEGER(L, 2);

    SL_source_set_group(self->source, group);

    return 0;
}

static int source_group(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, source_group1)
        LUAX_OVERLOAD_ARITY(2, source_group2)
    LUAX_OVERLOAD_END
}

static int source_gain1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

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

static int source_pan1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushnumber(L, SL_source_get_pan(self->source));

    return 1;
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

static int source_pan(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, source_pan1)
        LUAX_OVERLOAD_ARITY(2, source_pan2)
    LUAX_OVERLOAD_END
}

static int source_speed1(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

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

    SL_source_play(self->source);

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

static int source_rewind(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    SL_source_rewind(self->source);

    return 0;
}

static int source_is_playing(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Source_Object_t *self = (Source_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushboolean(L, SL_source_is_playing(self->source));

    return 1;
}