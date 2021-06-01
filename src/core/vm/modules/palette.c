/*
 * MIT License
 * 
 * Copyright (c) 2019-2021 Marco Lizza
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

#include <config.h>
#include <libs/log.h>
#include <libs/luax.h>
#include <libs/stb.h>
#include <resources/palettes.h>

#include "udt.h"

#define LOG_CONTEXT "palette"
#define META_TABLE  "Tofu_Graphics_Palette_mt"

static int palette_new_v_1u(lua_State *L);
static int palette_gc_1u_0(lua_State *L);
static int palette_mix_7nnnnnnN_3nnn(lua_State *L);
static int palette_colors_1u_1t(lua_State *L);
static int palette_size_1u_1n(lua_State *L);
static int palette_color_to_index_4unnn_1n(lua_State *L);
static int palette_index_to_color_2un_3nnn(lua_State *L);
static int palette_lerp_5unnnN_0(lua_State *L);
static int palette_merge_3uuB_0(lua_State *L);

int palette_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", palette_new_v_1u },
            { "__gc", palette_gc_1u_0 },
            { "mix", palette_mix_7nnnnnnN_3nnn },
            { "colors", palette_colors_1u_1t },
            { "size", palette_size_1u_1n },
            { "color_to_index", palette_color_to_index_4unnn_1n },
            { "index_to_color", palette_index_to_color_2un_3nnn },
            { "lerp", palette_lerp_5unnnN_0 },
            { "merge", palette_merge_3uuB_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int palette_new_0_1u(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    GL_Palette_t palette = { 0 };
    GL_palette_generate_greyscale(&palette, GL_MAX_PALETTE_COLORS);

    Palette_Object_t *self = (Palette_Object_t *)lua_newuserdatauv(L, sizeof(Palette_Object_t), 1);
    *self = (Palette_Object_t){
            .palette = palette
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "greyscale palette %p allocated w/ %d color(s)", self, palette.size);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int palette_new_1s_1u(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
    LUAX_SIGNATURE_END
    const char *id = LUAX_STRING(L, 1);

    const GL_Palette_t *predefined_palette = resources_palettes_find(id);
    if (predefined_palette == NULL) {
        return luaL_error(L, "unknown predefined palette w/ id `%s`", id);
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "setting predefined palette `%s` w/ %d color(s)", id, predefined_palette->size);

    Palette_Object_t *self = (Palette_Object_t *)lua_newuserdatauv(L, sizeof(Palette_Object_t), 1);
    *self = (Palette_Object_t){
            .palette = *predefined_palette
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "palette %p allocated w/ %d color(s)", self, self->palette.size); // FIXME: mark alloc/dealloc as TRACE.

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int palette_new_1n_1u(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t colors = (size_t)LUAX_INTEGER(L, 1);

    if (colors == 0) {
        return luaL_error(L, "palette can't be empty!");
    }

    GL_Palette_t palette = { 0 };
    GL_palette_generate_greyscale(&palette, colors);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "generating greyscale palette w/ %d color(s)", colors);

    Palette_Object_t *self = (Palette_Object_t *)lua_newuserdatauv(L, sizeof(Palette_Object_t), 1);
    *self = (Palette_Object_t){
            .palette = palette
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "palette %p allocated w/ %d color(s)", self, self->palette.size);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int palette_new_1t_1u(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END

    GL_Palette_t palette = { 0 };

    palette.size = lua_rawlen(L, 1);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "setting custom palette of %d color(s)", palette.size);

    if (palette.size == 0) {
        return luaL_error(L, "palette can't be empty!");
    } else
    if (palette.size > GL_MAX_PALETTE_COLORS) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "palette has too many colors (%d) - clamping", palette.size);
        palette.size = GL_MAX_PALETTE_COLORS;
    }

    lua_pushnil(L); // T -> T N
    for (size_t i = 0; lua_next(L, 1); ++i) { // T N -> T N T
#ifdef __DEFENSIVE_CHECKS__
        int count = lua_rawlen(L, 3);
        if (count != 3) {
            luaL_error(L, "palette entry #%d has %d components (out of 3 required)", i, count);
        }
#endif /* __DEFENSIVE_CHECKS__ */
        lua_rawgeti(L, 3, 1); // T N T -> T N T I
        lua_rawgeti(L, 3, 2); // T N T I -> T N T I I
        lua_rawgeti(L, 3, 3); // T N T I I -> T N T I I I

        uint8_t r = (uint8_t)LUAX_INTEGER(L, -3);
        uint8_t g = (uint8_t)LUAX_INTEGER(L, -2);
        uint8_t b = (uint8_t)LUAX_INTEGER(L, -1);

        lua_pop(L, 3); // T N T I I I -> T N T

        palette.colors[i] = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };

        lua_pop(L, 1); // T N T -> T N
    }

    Palette_Object_t *self = (Palette_Object_t *)lua_newuserdatauv(L, sizeof(Palette_Object_t), 1);
    *self = (Palette_Object_t){
            .palette = palette
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "palette %p allocated w/ %d color(s)", self, self->palette.size);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int palette_new_3n_1u(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t red_bits = (size_t)LUAX_INTEGER(L, 1);
    size_t green_bits = (size_t)LUAX_INTEGER(L, 2);
    size_t blue_bits = (size_t)LUAX_INTEGER(L, 3);

    size_t bits = red_bits + green_bits + blue_bits;
    if (bits == 0) {
        return luaL_error(L, "at least one bit is required (R%dG%dB%d == %d bits)", red_bits, green_bits, blue_bits, bits);
    } else
    if (bits > (sizeof(GL_Pixel_t) * 8)) {
        return luaL_error(L, "too many bits to fit a pixel (R%dG%dB%d == %d bits)", red_bits, green_bits, blue_bits, bits);
    }

    GL_Palette_t palette = { 0 };
    GL_palette_generate_quantized(&palette, red_bits, green_bits, blue_bits);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "generating quantized palette R%d:G%d:B%d (%d color(s))", red_bits, green_bits, blue_bits, 1 << bits);

    Palette_Object_t *self = (Palette_Object_t *)lua_newuserdatauv(L, sizeof(Palette_Object_t), 1);
    *self = (Palette_Object_t){
            .palette = palette
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "palette %p allocated w/ %d color(s)", self, self->palette.size);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int palette_new_v_1u(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, palette_new_0_1u)
        LUAX_OVERLOAD_SIGNATURE(palette_new_1s_1u, LUA_TSTRING)
        LUAX_OVERLOAD_SIGNATURE(palette_new_1n_1u, LUA_TNUMBER)
        LUAX_OVERLOAD_SIGNATURE(palette_new_1t_1u, LUA_TTABLE)
        LUAX_OVERLOAD_ARITY(3, palette_new_3n_1u)
    LUAX_OVERLOAD_END
}

static int palette_gc_1u_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Palette_Object_t *self = (Palette_Object_t *)LUAX_USERDATA(L, 1);

#ifdef __PALETTE_COLOR_MEMOIZATION__
    hmfree(self->cache);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "memoizing cache %p freed", self->cache);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "palette %p finalized", self);

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
    float ratio = (float)LUAX_OPTIONAL_NUMBER(L, 7, 0.5);

    const GL_Color_t a = (GL_Color_t){ .r = ar, .g = ag, .b = ab, .a = 255 };
    const GL_Color_t b = (GL_Color_t){ .r = br, .g = bg, .b = bb, .a = 255 };

    const GL_Color_t color = GL_palette_lerp(a, b, ratio);

    lua_pushinteger(L, (lua_Integer)color.r);
    lua_pushinteger(L, (lua_Integer)color.g);
    lua_pushinteger(L, (lua_Integer)color.b);

    return 3;
}

static int palette_colors_1u_1t(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Palette_Object_t *self = (Palette_Object_t *)LUAX_USERDATA(L, 1);

    const GL_Palette_t *palette = &self->palette;

    lua_createtable(L, (int)palette->size, 0);
    for (size_t i = 0; i < palette->size; ++i) {
        const GL_Color_t color = palette->colors[i];

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

static int palette_size_1u_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Palette_Object_t *self = (Palette_Object_t *)LUAX_USERDATA(L, 1);

    const GL_Palette_t *palette = &self->palette;

    lua_pushinteger(L, (lua_Integer)palette->size);

    return 1;
}

static int palette_color_to_index_4unnn_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
#ifdef __PALETTE_COLOR_MEMOIZATION__
    Palette_Object_t *self = (Palette_Object_t *)LUAX_USERDATA(L, 1);
#else
    const Palette_Object_t *self = (const Palette_Object_t *)LUAX_USERDATA(L, 1);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */
    uint8_t r = (uint8_t)LUAX_INTEGER(L, 2);
    uint8_t g = (uint8_t)LUAX_INTEGER(L, 3);
    uint8_t b = (uint8_t)LUAX_INTEGER(L, 4);

    const GL_Color_t color = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };

    const GL_Palette_t *palette = &self->palette;
#ifdef __PALETTE_COLOR_MEMOIZATION__
    GL_Pixel_t index;
    int position = hmgeti(self->cache, color);
    if (position == -1) {
        index = GL_palette_find_nearest_color(palette, color);
        hmput(self->cache, color, index);
        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "color <%d, %d, %d> stored into memoizing cache w/ index #%d", r, g, b, index);
    } else {
        index = self->cache[position].value;
        Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "color <%d, %d, %d> found into memoizing cache at #%d (index #%d)", r, g, b, position, index);
    }
#else
    const GL_Pixel_t index = GL_palette_find_nearest_color(palette, color);
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "color <%d, %d, %d> matched w/ palette index #%d", r, g, b, index);
#endif  /* __PALETTE_COLOR_MEMOIZATION__ */

    lua_pushinteger(L, (lua_Integer)index);

    return 1;
}

static int palette_index_to_color_2un_3nnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Palette_Object_t *self = (const Palette_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 2);

    const GL_Palette_t *palette = &self->palette;
    const GL_Color_t color = palette->colors[index];

    lua_pushinteger(L, (lua_Integer)color.r);
    lua_pushinteger(L, (lua_Integer)color.g);
    lua_pushinteger(L, (lua_Integer)color.b);

    return 3;
}

static int palette_lerp_5unnnN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Palette_Object_t *self = (Palette_Object_t *)LUAX_USERDATA(L, 1);
    uint8_t r = (uint8_t)LUAX_INTEGER(L, 2);
    uint8_t g = (uint8_t)LUAX_INTEGER(L, 3);
    uint8_t b = (uint8_t)LUAX_INTEGER(L, 4);
    float ratio = (float)LUAX_OPTIONAL_NUMBER(L, 5, 0.5);

    const GL_Color_t color = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };

    GL_Palette_t *palette = &self->palette;
    GL_Color_t *colors = palette->colors;
    for (size_t i = 0; i < palette->size; ++i) {
        colors[i] = GL_palette_lerp(colors[i], color, ratio);
    }

    return 0;
}

static bool _contains(const GL_Palette_t *palette, GL_Color_t color)
{
    for (size_t i = 0; i < palette->size; ++i) {
        if (memcmp(&palette->colors[i], &color, sizeof(GL_Color_t)) == 0) {
            return true;
        }
    }
    return false;
}

static int palette_merge_3uuB_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_OPTIONAL(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    Palette_Object_t *self = (Palette_Object_t *)LUAX_USERDATA(L, 1);
    const Palette_Object_t *other = (const Palette_Object_t *)LUAX_USERDATA(L, 2);
    bool remove_duplicates = LUAX_OPTIONAL_BOOLEAN(L, 3, true);

    for (size_t i = 0; i < other->palette.size; ++i) {
        if (self->palette.size == GL_MAX_PALETTE_COLORS) {
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "maximum palette size reached when merging palette %p w/ %p", self, other);
            break;
        }
        if (remove_duplicates && _contains(&self->palette, other->palette.colors[i])) {
            continue;
        }
        self->palette.colors[++self->palette.size] = other->palette.colors[i];
    }

    return 0;
}
