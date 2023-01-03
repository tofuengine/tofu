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

#include "palette.h"

#include "internal/udt.h"

#include <core/config.h>
#include <libs/log.h>
#include <libs/path.h>
#include <systems/storage.h>

#define LOG_CONTEXT "palette"
#define MODULE_NAME "tofu.graphics.palette"
#define META_TABLE  "Tofu_Graphics_Palette_mt"

static int palette_new_v_1o(lua_State *L);
static int palette_gc_1o_0(lua_State *L);
static int palette_mix_7nnnnnnN_3nnn(lua_State *L);
static int palette_colors_1o_1t(lua_State *L);
static int palette_size_1o_1n(lua_State *L);
static int palette_get_2on_3nnn(lua_State *L);
static int palette_set_5onnnn_0(lua_State *L);
static int palette_match_4onnn_1n(lua_State *L);
static int palette_lerp_5onnnN_0(lua_State *L);
static int palette_merge_6ononnB_0(lua_State *L);

int palette_loader(lua_State *L)
{
    char file[PATH_MAX] = { 0 };
    path_lua_to_fs(file, MODULE_NAME);

    Storage_t *storage = (Storage_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_STORAGE));
    Storage_Resource_t *script = Storage_load(storage, file + 1, STORAGE_RESOURCE_STRING);

    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){
            .data = S_SCHARS(script),
            .size = S_SLENTGH(script),
            .name = file
        },
        (const struct luaL_Reg[]){
            { "new", palette_new_v_1o },
            { "__gc", palette_gc_1o_0 },
            { "mix", palette_mix_7nnnnnnN_3nnn },
            { "colors", palette_colors_1o_1t },
            { "size", palette_size_1o_1n },
            { "get", palette_get_2on_3nnn },
            { "set", palette_set_5onnnn_0 },
            { "match", palette_match_4onnn_1n },
            { "lerp", palette_lerp_5onnnN_0 },
            { "merge", palette_merge_6ononnB_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int palette_new_0_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Palette_Object_t *self = (Palette_Object_t *)luaX_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .size = GL_MAX_PALETTE_COLORS
        },
        OBJECT_TYPE_PALETTE, META_TABLE);

    GL_palette_set_greyscale(self->palette, GL_MAX_PALETTE_COLORS);
    LOG_D(LOG_CONTEXT, "greyscale palette %p allocated w/ %d color(s)", self, GL_MAX_PALETTE_COLORS);

    return 1;
}

static int palette_new_1n_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t levels = LUAX_UNSIGNED(L, 1);

    if (levels == 0) {
        return luaL_error(L, "palette can't be empty!");
    }

    Palette_Object_t *self = (Palette_Object_t *)luaX_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .size = levels
        },
        OBJECT_TYPE_PALETTE, META_TABLE);

    GL_palette_set_greyscale(self->palette, levels);
    LOG_D(LOG_CONTEXT, "palette %p allocated w/ %d color(s)", self, levels);

    return 1;
}

static int palette_new_1t_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    // idx #1: LUA_TTABLE

    size_t size = lua_rawlen(L, 1);
    LOG_D(LOG_CONTEXT, "setting custom palette of %d color(s)", size);

    if (size == 0) {
        return luaL_error(L, "palette can't be empty!");
    } else
    if (size > GL_MAX_PALETTE_COLORS) {
        LOG_W(LOG_CONTEXT, "palette has too many colors (%d) - clamping to %d", size, GL_MAX_PALETTE_COLORS);
        size = GL_MAX_PALETTE_COLORS;
    }

    GL_Color_t palette[GL_MAX_PALETTE_COLORS];
    lua_pushnil(L); // T -> T N
    for (size_t i = 0; lua_next(L, 1); ++i) { // T N -> T N T
#ifdef __DEFENSIVE_CHECKS__
        size_t components = lua_rawlen(L, 3);
        if (components != 3) {
            luaL_error(L, "palette entry #%d has %d components (out of 3 required)", i, components);
        }
#endif /* __DEFENSIVE_CHECKS__ */
        lua_rawgeti(L, 3, 1); // T N T -> T N T I
        lua_rawgeti(L, 3, 2); // T N T I -> T N T I I
        lua_rawgeti(L, 3, 3); // T N T I I -> T N T I I I

        uint8_t r = (uint8_t)LUAX_INTEGER(L, -3);
        uint8_t g = (uint8_t)LUAX_INTEGER(L, -2);
        uint8_t b = (uint8_t)LUAX_INTEGER(L, -1);

        lua_pop(L, 3); // T N T I I I -> T N T

        palette[i] = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };

        lua_pop(L, 1); // T N T -> T N
    }

    for (size_t i = size; i < GL_MAX_PALETTE_COLORS; ++i) { // Fill the remaining part of the palette with the last color (to fix color matching).
        palette[i] = palette[size - 1];
    }

    Palette_Object_t *self = (Palette_Object_t *)luaX_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .size = size
        },
        OBJECT_TYPE_PALETTE, META_TABLE);

    GL_palette_copy(self->palette, palette);
    LOG_D(LOG_CONTEXT, "palette %p allocated w/ %d color(s)", self, size);

    return 1;
}

static int palette_new_1o_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Palette_Object_t *other = (const Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);

    LOG_D(LOG_CONTEXT, "cloning palette %p", other);

    Palette_Object_t *self = (Palette_Object_t *)luaX_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .size = other->size
        },
        OBJECT_TYPE_PALETTE, META_TABLE);

    GL_palette_copy(self->palette, other->palette);
    LOG_D(LOG_CONTEXT, "palette %p allocated", self);

    return 1;
}

static int palette_new_3n_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t red_bits = LUAX_UNSIGNED(L, 1);
    size_t green_bits = LUAX_UNSIGNED(L, 2);
    size_t blue_bits = LUAX_UNSIGNED(L, 3);

    size_t bits = red_bits + green_bits + blue_bits;
    const size_t size = 1 << bits;

    if (size == 0) {
        return luaL_error(L, "at least one bit is required (R%dG%dB%d == %d bits)", red_bits, green_bits, blue_bits, bits);
    } else
    if (size > GL_MAX_PALETTE_COLORS) {
        return luaL_error(L, "too many bits to fit palette (R%dG%dB%d == %d bits)", red_bits, green_bits, blue_bits, bits);
    }

    LOG_D(LOG_CONTEXT, "generating quantized palette R%d:G%d:B%d (%d color(s))", red_bits, green_bits, blue_bits, size);

    Palette_Object_t *self = (Palette_Object_t *)luaX_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .size = size
        },
        OBJECT_TYPE_PALETTE, META_TABLE);

    GL_palette_set_quantized(self->palette, red_bits, green_bits, blue_bits);
    LOG_D(LOG_CONTEXT, "palette %p allocated w/ %d colors(s)", self, size);

    lua_pushinteger(L, size);

    return 2;
}

static int palette_new_v_1o(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, palette_new_0_1o)
        LUAX_OVERLOAD_SIGNATURE(palette_new_1n_1o, LUA_TNUMBER)
        LUAX_OVERLOAD_SIGNATURE(palette_new_1t_1o, LUA_TTABLE)
        LUAX_OVERLOAD_SIGNATURE(palette_new_1o_1o, LUA_TOBJECT)
        LUAX_OVERLOAD_ARITY(3, palette_new_3n_1o)
    LUAX_OVERLOAD_END
}

static int palette_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Palette_Object_t *self = (Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);

    LOG_D(LOG_CONTEXT, "palette %p finalized", self);

    return 0;
}

static int palette_mix_7nnnnnnN_3nnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    uint8_t ar = (uint8_t)LUAX_INTEGER(L, 1);
    uint8_t ag = (uint8_t)LUAX_INTEGER(L, 2);
    uint8_t ab = (uint8_t)LUAX_INTEGER(L, 3);
    uint8_t br = (uint8_t)LUAX_INTEGER(L, 4);
    uint8_t bg = (uint8_t)LUAX_INTEGER(L, 5);
    uint8_t bb = (uint8_t)LUAX_INTEGER(L, 6);
    float ratio = LUAX_OPTIONAL_NUMBER(L, 7, 0.5f);

    const GL_Color_t a = (GL_Color_t){ .r = ar, .g = ag, .b = ab, .a = 255 };
    const GL_Color_t b = (GL_Color_t){ .r = br, .g = bg, .b = bb, .a = 255 };

    const GL_Color_t color = GL_palette_mix(a, b, ratio);

    lua_pushinteger(L, (lua_Integer)color.r);
    lua_pushinteger(L, (lua_Integer)color.g);
    lua_pushinteger(L, (lua_Integer)color.b);

    return 3;
}

static int palette_colors_1o_1t(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Palette_Object_t *self = (const Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);

    const GL_Color_t *palette = self->palette;

    lua_createtable(L, GL_MAX_PALETTE_COLORS, 0);
    for (size_t i = 0; i < GL_MAX_PALETTE_COLORS; ++i) {
        const GL_Color_t color = palette[i];

        lua_createtable(L, 3, 0);
        lua_pushinteger(L, (lua_Integer)color.r);
        lua_rawseti(L, -2, 1);
        lua_pushinteger(L, (lua_Integer)color.g);
        lua_rawseti(L, -2, 2);
        lua_pushinteger(L, (lua_Integer)color.b);
        lua_rawseti(L, -2, 3);

        lua_rawseti(L, -2, (lua_Integer)(i + 1));
    }

    return 1;
}

static int palette_size_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Palette_Object_t *self = (const Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);

    lua_pushinteger(L, (lua_Integer)self->size);

    return 1;
}

int palette_get_2on_3nnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Palette_Object_t *self = (const Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 2);

    const GL_Color_t *palette = self->palette;
    GL_Color_t color = palette[index];

    lua_pushinteger(L, (lua_Integer)color.r);
    lua_pushinteger(L, (lua_Integer)color.g);
    lua_pushinteger(L, (lua_Integer)color.b);

    return 3;
}

int palette_set_5onnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Palette_Object_t *self = (Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 2);
    uint8_t r = (uint8_t)LUAX_INTEGER(L, 3);
    uint8_t g = (uint8_t)LUAX_INTEGER(L, 4);
    uint8_t b = (uint8_t)LUAX_INTEGER(L, 5);

    GL_Color_t *palette = self->palette;
    const GL_Color_t color = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };
    palette[index] = color;

    return 0;
}

static int palette_match_4onnn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Palette_Object_t *self = (const Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);
    uint8_t r = (uint8_t)LUAX_INTEGER(L, 2);
    uint8_t g = (uint8_t)LUAX_INTEGER(L, 3);
    uint8_t b = (uint8_t)LUAX_INTEGER(L, 4);

    const GL_Color_t color = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };

    const GL_Color_t *palette = self->palette;
    const GL_Pixel_t index = GL_palette_find_nearest_color(palette, color);

    lua_pushinteger(L, (lua_Integer)index);

    return 1;
}

static int palette_lerp_5onnnN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Palette_Object_t *self = (Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);
    uint8_t r = (uint8_t)LUAX_INTEGER(L, 2);
    uint8_t g = (uint8_t)LUAX_INTEGER(L, 3);
    uint8_t b = (uint8_t)LUAX_INTEGER(L, 4);
    float ratio = LUAX_OPTIONAL_NUMBER(L, 5, 0.5f);

    const GL_Color_t color = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };

    GL_Color_t *palette = self->palette;
    GL_palette_lerp(palette, color, ratio);

    return 0;
}

static int palette_merge_6ononnB_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    Palette_Object_t *self = (Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);
    size_t to = LUAX_UNSIGNED(L, 2);
    const Palette_Object_t *other = (const Palette_Object_t *)LUAX_OBJECT(L, 3, OBJECT_TYPE_PALETTE);
    size_t from = LUAX_UNSIGNED(L, 4);
    size_t count = LUAX_UNSIGNED(L, 5);
    bool remove_duplicates = LUAX_OPTIONAL_BOOLEAN(L, 3, true);

    GL_Color_t *palette = self->palette;
    size_t size = GL_palette_merge(palette, to, other->palette, from, count, remove_duplicates);

    self->size = size;
    LOG_D(LOG_CONTEXT, "palette %p has now %d color(s)", self, size);

    return 0;
}
