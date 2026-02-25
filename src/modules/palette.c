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
 * Copyright (c) 2019-2026 Marco Lizza
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
#include <libs/hex.h>
#define _LOG_TAG "palette"
#include <libs/log.h>

static int palette_new_v_1o(lua_State *L);
static int palette_gc_1o_0(lua_State *L);
static int palette_colors_1o_1t(lua_State *L);
static int palette_size_1o_1n(lua_State *L);
static int palette_peek_2on_3nnn(lua_State *L);
static int palette_poke_5onnnn_0(lua_State *L);
static int palette_lerp_5onnnN_0(lua_State *L);
static int palette_merge_6ononnB_0(lua_State *L);
static int palette_match_4onnn_1n(lua_State *L);
static int palette_mix_7nnnnnnN_3nnn(lua_State *L);

int palette_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", palette_new_v_1o },
            { "__gc", palette_gc_1o_0 },
            // -- accessors --
            { "colors", palette_colors_1o_1t },
            { "size", palette_size_1o_1n },
            { "peek", palette_peek_2on_3nnn }, // TODO: rename to `peek` and `poke`? Or override?
            // -- mutators --
            { "poke", palette_poke_5onnnn_0 },
            { "lerp", palette_lerp_5onnnN_0 },
            { "merge", palette_merge_6ononnB_0 },
            // -- operations --
            { "match", palette_match_4onnn_1n },
            { "mix", palette_mix_7nnnnnnN_3nnn },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static int palette_new_0_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Palette_Object_t *self = (Palette_Object_t *)udt_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .palette = { 0 }
        }, OBJECT_TYPE_PALETTE);

    GL_palette_set_greyscale(self->palette, GL_PALETTE_MAX_COLORS);

    LOG_D("greyscale palette %p allocated w/ %d color(s)", self, GL_PALETTE_MAX_COLORS);

    return 1;
}

// Fill the trailing part of the palette with the last color. This is useful
// to fix color matching when the palette has less than GL_PALETTE_MAX_COLORS
// colors actually defined. Otherwise, the undefined colors would be BLACK,
// compromising the color matching.
static inline void _pad_palette(GL_Color_t *palette, size_t size)
{
    for (size_t i = size; i < GL_PALETTE_MAX_COLORS; ++i) {
        palette[i] = palette[size - 1];
    }
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
    if (levels > GL_PALETTE_MAX_COLORS) {
        return luaL_error(L, "palette has too many colors (%d) - max is %d", levels, GL_PALETTE_MAX_COLORS);
    }

    Palette_Object_t *self = (Palette_Object_t *)udt_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .palette = { 0 }
        }, OBJECT_TYPE_PALETTE);

    GL_palette_set_greyscale(self->palette, levels);

    _pad_palette(self->palette, levels);

    LOG_D("palette %p allocated w/ %d color(s)", self, levels);

    return 1;
}

static int palette_new_1t_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    // idx #1: LUA_TTABLE

    size_t size = lua_rawlen(L, 1);
    LOG_D("setting custom palette of %d color(s)", size);

    if (size == 0) {
        return luaL_error(L, "palette can't be empty!");
    } else
    if (size > GL_PALETTE_MAX_COLORS) {
        return luaL_error(L, "palette has too many colors (%d) - max is %d", size, GL_PALETTE_MAX_COLORS);
    }

    Palette_Object_t *self = (Palette_Object_t *)udt_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .palette = { 0 }
        }, OBJECT_TYPE_PALETTE);

    lua_pushnil(L); // T O -> T O N
    for (size_t i = 0; lua_next(L, 1); ++i) { // T O N -> T O N T
        int item_stack_index = lua_gettop(L); // Obtain the stack index of the current item, to access it balistically. :D
#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
        size_t components = lua_rawlen(L, item_stack_index);
        if (components != 3) {
            luaL_error(L, "palette entry #%d has %d components (out of 3 required)", i, components);
        }
#endif /* TOFU_CORE_DEFENSIVE_CHECKS */
        lua_rawgeti(L, item_stack_index, 1); // T O N T -> T O N T I
        lua_rawgeti(L, item_stack_index, 2); // T O N T I -> T O N T I I
        lua_rawgeti(L, item_stack_index, 3); // T O N T I I -> T O N T I I I

        uint8_t r = (uint8_t)LUAX_INTEGER(L, -3);
        uint8_t g = (uint8_t)LUAX_INTEGER(L, -2);
        uint8_t b = (uint8_t)LUAX_INTEGER(L, -1);

        lua_pop(L, 3); // T O N T I I I -> T O N T

        self->palette[i] = gl_color_from_rgb(r, g, b);

        lua_pop(L, 1); // T O N T -> T O N
    }

    _pad_palette(self->palette, size);

    LOG_D("palette %p allocated w/ %d color(s)", self, size);

    return 1;
}

static int palette_new_1o_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Palette_Object_t *other = (const Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);

    LOG_D("cloning palette %p", other);

    Palette_Object_t *self = (Palette_Object_t *)udt_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .palette = { 0 }
        }, OBJECT_TYPE_PALETTE);

    GL_palette_copy(self->palette, other->palette);
    LOG_D("palette %p allocated", self);

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
    if (size > GL_PALETTE_MAX_COLORS) {
        return luaL_error(L, "too many bits to fit palette (R%dG%dB%d == %d bits)", red_bits, green_bits, blue_bits, bits);
    }

    LOG_D("generating quantized palette R%d:G%d:B%d (%d color(s))", red_bits, green_bits, blue_bits, size);

    Palette_Object_t *self = (Palette_Object_t *)udt_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .palette = { 0 }
        }, OBJECT_TYPE_PALETTE);

    GL_palette_set_quantized(self->palette, red_bits, green_bits, blue_bits);
    LOG_D("palette %p allocated w/ %d colors(s)", self, size);

    lua_pushinteger(L, size);

    return 2;
}

static int palette_new_v_1o(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(palette_new_0_1o, 0)
        LUAX_OVERLOAD_BY_TYPES(palette_new_1n_1o, LUA_TNUMBER)
        LUAX_OVERLOAD_BY_TYPES(palette_new_1t_1o, LUA_TTABLE)
        LUAX_OVERLOAD_BY_TYPES(palette_new_1o_1o, LUA_TOBJECT)
        LUAX_OVERLOAD_BY_ARITY(palette_new_3n_1o, 3)
    LUAX_OVERLOAD_END
}

static int palette_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Palette_Object_t *self = (Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);

    LOG_D("palette %p finalized", self);

    return 0;
}

static int palette_colors_1o_1t(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Palette_Object_t *self = (const Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);

    const GL_Color_t *palette = self->palette;

    lua_createtable(L, GL_PALETTE_MAX_COLORS, 0);
    for (size_t i = 0; i < GL_PALETTE_MAX_COLORS; ++i) {
        const GL_Color_t color = palette[i];

        lua_createtable(L, 3, 0);
        lua_pushinteger(L, (lua_Integer)gl_color_get_r(color));
        lua_rawseti(L, -2, 1);
        lua_pushinteger(L, (lua_Integer)gl_color_get_g(color));
        lua_rawseti(L, -2, 2);
        lua_pushinteger(L, (lua_Integer)gl_color_get_b(color));
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
    LUAX_UNUSED(self);

    lua_pushinteger(L, (lua_Integer)GL_PALETTE_MAX_COLORS);

    return 1;
}

int palette_peek_2on_3nnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Palette_Object_t *self = (const Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED(L, 2);

    const GL_Color_t *palette = self->palette;
    GL_Color_t color = palette[index];

    lua_pushinteger(L, (lua_Integer)gl_color_get_r(color));
    lua_pushinteger(L, (lua_Integer)gl_color_get_g(color));
    lua_pushinteger(L, (lua_Integer)gl_color_get_b(color));

    return 3;
}

int palette_poke_5onnnn_0(lua_State *L)
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

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (index >= GL_PALETTE_MAX_COLORS) {
        return luaL_error(L, "palette index %d is out of bounds (max is %d)", index, GL_PALETTE_MAX_COLORS - 1);
    }
#endif  /* TOFU_CORE_DEFENSIVE_CHECKS */

    GL_Color_t *palette = self->palette;
    const GL_Color_t color = gl_color_from_rgb(r, g, b);
    palette[index] = color;

    return 0;
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

    const GL_Color_t color = gl_color_from_rgb(r, g, b);

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

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (to >= GL_PALETTE_MAX_COLORS) {
        return luaL_error(L, "target palette index %d is out of bounds (max is %d)", to, GL_PALETTE_MAX_COLORS - 1);
    }
    if (from >= GL_PALETTE_MAX_COLORS) {
        return luaL_error(L, "source palette index %d is out of bounds (max is %d)", from, GL_PALETTE_MAX_COLORS - 1);
    }
    if (count == 0) {
        return luaL_error(L, "at least one color is required to merge");
    }
    if (to + count > GL_PALETTE_MAX_COLORS) {
        return luaL_error(L, "too many colors to merge into target palette (to %d + count %d > max %d)", to, count, GL_PALETTE_MAX_COLORS);
    }
    if (from + count > GL_PALETTE_MAX_COLORS) {
        return luaL_error(L, "too many colors to merge from source palette (from %d + count %d > max %d)", from, count, GL_PALETTE_MAX_COLORS);
    }
#endif /* TOFU_CORE_DEFENSIVE_CHECKS */

    GL_Color_t *palette = self->palette;
    GL_palette_merge(palette, to, other->palette, from, count, remove_duplicates);

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

    const GL_Color_t color = gl_color_from_rgb(r, g, b);

    const GL_Color_t *palette = self->palette;
    const GL_Pixel_t index = GL_palette_find_nearest_color(palette, color);

    lua_pushinteger(L, (lua_Integer)index);

    return 1;
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

    const GL_Color_t a = gl_color_from_rgb(ar, ag, ab);
    const GL_Color_t b = gl_color_from_rgb(br, bg, bb);

    const GL_Color_t color = GL_palette_mix(a, b, ratio);

    lua_pushinteger(L, (lua_Integer)gl_color_get_r(color));
    lua_pushinteger(L, (lua_Integer)gl_color_get_g(color));
    lua_pushinteger(L, (lua_Integer)gl_color_get_b(color));

    return 3;
}
