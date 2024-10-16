/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "image.h"

#include "internal/callbacks.h"
#include "internal/udt.h"

#include <core/config.h>
#define _LOG_TAG "image"
#include <libs/log.h>
#include <libs/stopwatch.h>
#include <libs/stb.h>
#include <systems/display.h>
#include <systems/storage.h>

static int image_new_v_1o(lua_State *L);
static int image_gc_1o_0(lua_State *L);
static int image_size_1o_2nn(lua_State *L);
static int image_center_1o_2nn(lua_State *L);
static int image_peek_3onn_1n(lua_State *L);
static int image_poke_4onnn_0(lua_State *L);
static int image_clear_2oN_0(lua_State *L);
//static int image_grab(lua_State *L);

int image_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", image_new_v_1o },
            { "__gc", image_gc_1o_0 },
            // -- accessors --
            { "size", image_size_1o_2nn },
            { "center", image_center_1o_2nn },
            { "peek", image_peek_3onn_1n },
            // -- mutators --
            { "poke", image_poke_4onnn_0 },
            { "clear", image_clear_2oN_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static int image_new_0_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Display_t *display = (const Display_t *)udt_get_userdata(L, USERDATA_DISPLAY);

    GL_Surface_t *surface = Display_get_surface(display);
    LOG_D("default surface %p retrieved", surface);

    Image_Object_t *self = (Image_Object_t *)udt_newobject(L, sizeof(Image_Object_t), &(Image_Object_t){
            .surface = surface,
            .allocated = false
        }, OBJECT_TYPE_IMAGE);

    LOG_D("image %p allocated w/ default surface", self);

    return 1;
}

static int image_new_2nn_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t width = LUAX_UNSIGNED(L, 1);
    size_t height = LUAX_UNSIGNED(L, 2);

    GL_Surface_t *surface = GL_surface_create(width, height);
    if (!surface) {
        return luaL_error(L, "can't create %dx%d surface", width, height);
    }
    LOG_D("%dx%d surface allocate at %p", width, height, surface);

    Image_Object_t *self = (Image_Object_t *)udt_newobject(L, sizeof(Image_Object_t), &(Image_Object_t){
            .surface = surface,
            .allocated = true
        }, OBJECT_TYPE_IMAGE);

    LOG_D("image %p allocated w/ surface %p", self, surface);

    return 1;
}

static int image_new_3sNO_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    GL_Pixel_t transparent_index = (GL_Pixel_t)LUAX_OPTIONAL_UNSIGNED(L, 2, 0);
    const Palette_Object_t *palette = (const Palette_Object_t *)LUAX_OPTIONAL_OBJECT(L, 3, OBJECT_TYPE_PALETTE, NULL);

    Storage_t *storage = (Storage_t *)udt_get_userdata(L, USERDATA_STORAGE);
    const Display_t *display = (const Display_t *)udt_get_userdata(L, USERDATA_DISPLAY);

    Callback_Palette_Closure_t closure = (Callback_Palette_Closure_t){
            .palette = palette ? palette->palette : Display_get_palette(display), // Use current display's if not passed.
            .transparent = transparent_index,
            .threshold = 0
        };

#if defined(TOFU_CORE_PROFILING_ENABLED)
    StopWatch_t stopwatch = stopwatch_init();
    LOG_I("profiling loading and decoding for image `%s`", name);
#endif
    const Storage_Resource_t *image = Storage_load(storage, name, STORAGE_RESOURCE_IMAGE);
    if (!image) {
        return luaL_error(L, "can't load file `%s`", name);
    }
    GL_Surface_t *surface = GL_surface_decode(SR_IWIDTH(image), SR_IHEIGHT(image), SR_IPIXELS(image), surface_callback_palette, (void *)&closure);
    if (!surface) {
        return luaL_error(L, "can't decode file `%s`", name);
    }
#if defined(TOFU_CORE_PROFILING_ENABLED)
    LOG_I("loading and decoding image `%s` took %.3fs", name, stopwatch_elapsed(&stopwatch));
#endif
    LOG_D("surface %p loaded and decoded from file `%s`", surface, name);

    Image_Object_t *self = (Image_Object_t *)udt_newobject(L, sizeof(Image_Object_t), &(Image_Object_t){
            .surface = surface,
            .allocated = true
        }, OBJECT_TYPE_IMAGE);

    LOG_D("image %p allocated w/ surface %p", self, surface);

    return 1;
}

static int image_new_3snn_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *name = LUAX_STRING(L, 1);
    GL_Pixel_t background_index = (GL_Pixel_t)LUAX_UNSIGNED(L, 2);
    GL_Pixel_t foreground_index = (GL_Pixel_t)LUAX_UNSIGNED(L, 3);

    Storage_t *storage = (Storage_t *)udt_get_userdata(L, USERDATA_STORAGE);

    Callback_Indexes_Closure_t closure = (Callback_Indexes_Closure_t){
            .background = background_index,
            .foreground = foreground_index
        };

#if defined(TOFU_CORE_PROFILING_ENABLED)
    StopWatch_t stopwatch = stopwatch_init();
    LOG_I("profiling loading and decoding for image `%s`", name);
#endif
    const Storage_Resource_t *image = Storage_load(storage, name, STORAGE_RESOURCE_IMAGE);
    if (!image) {
        return luaL_error(L, "can't load file `%s`", name);
    }
    GL_Surface_t *surface = GL_surface_decode(SR_IWIDTH(image), SR_IHEIGHT(image), SR_IPIXELS(image), surface_callback_indexes, (void *)&closure);
    if (!surface) {
        return luaL_error(L, "can't decode file `%s`", name);
    }
#if defined(TOFU_CORE_PROFILING_ENABLED)
    LOG_I("loading and decoding image `%s` took %.3fs", name, stopwatch_elapsed(&stopwatch));
#endif
    LOG_D("surface %p loaded and decoded from file `%s`", surface, name);

    Image_Object_t *self = (Image_Object_t *)udt_newobject(L, sizeof(Image_Object_t), &(Image_Object_t){
            .surface = surface,
            .allocated = true
        }, OBJECT_TYPE_IMAGE);

    LOG_D("image %p allocated w/ surface %p", self, surface);

    return 1;
}

static int image_new_v_1o(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(image_new_0_1o, 0)
        LUAX_OVERLOAD_BY_TYPES(image_new_3sNO_1o, LUA_TSTRING)
        LUAX_OVERLOAD_BY_TYPES(image_new_3sNO_1o, LUA_TSTRING, LUA_TNUMBER)
        LUAX_OVERLOAD_BY_ARITY(image_new_2nn_1o, 2)
        LUAX_OVERLOAD_BY_TYPES(image_new_3sNO_1o, LUA_TSTRING, LUA_TNUMBER, LUA_TOBJECT)
        LUAX_OVERLOAD_BY_ARITY(image_new_3snn_1o, 3)
    LUAX_OVERLOAD_END
}

static int image_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Image_Object_t *self = (Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);

    if (self->allocated) {
        GL_surface_destroy(self->surface);
        LOG_D("surface %p destroyed", self->surface);
    }

    LOG_D("image %p finalized", self);

    return 0;
}

static int image_size_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Image_Object_t *self = (const Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);

    const GL_Surface_t *surface = self->surface;
    lua_pushinteger(L, (lua_Integer)surface->width);
    lua_pushinteger(L, (lua_Integer)surface->height);

    return 2;
}

static int image_center_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Image_Object_t *self = (const Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);

    const GL_Surface_t *surface = self->surface;
    lua_pushinteger(L, (lua_Integer)(surface->width / 2));
    lua_pushinteger(L, (lua_Integer)(surface->height / 2));

    return 2;
}

static int image_peek_3onn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Image_Object_t *self = (const Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);

    const GL_Pixel_t index = GL_surface_peek(self->surface, (GL_Point_t){ .x = x, .y = y });

    lua_pushinteger(L, (lua_Integer)index);

    return 1;
}

static int image_poke_4onnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Image_Object_t *self = (const Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);
    int x = LUAX_INTEGER(L, 2);
    int y = LUAX_INTEGER(L, 3);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 4);

    GL_surface_poke(self->surface, (GL_Point_t){ .x = x, .y = y }, index);

    return 0;
}

static int image_clear_2oN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Image_Object_t *self = (const Image_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_IMAGE);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 2);

    GL_surface_clear(self->surface, index);

    return 0;
}
