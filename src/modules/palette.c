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
#include <libs/imath.h>
#include <libs/log.h>

static int palette_new_v_1o(lua_State *L);
static int palette_gc_1o_0(lua_State *L);
static int palette_colors_1o_1t(lua_State *L);
static int palette_size_1o_1n(lua_State *L);
static int palette_peek_2on_3nnn(lua_State *L);
static int palette_poke_5onnnn_0(lua_State *L);
static int palette_lerp_5onnnN_0(lua_State *L);
static int palette_merge_6ononnB_0(lua_State *L);
static int palette_resize_2on(lua_State *L);
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
            { "resize", palette_resize_2on },
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
            .palette = { 0 },
            .used_colors = 0
        }, OBJECT_TYPE_PALETTE);

    GL_palette_set_greyscale(self->palette, GL_PALETTE_MAX_COLORS);

    self->used_colors = GL_PALETTE_MAX_COLORS;

    LOG_D("greyscale palette %p allocated w/ %d color(s)", self, self->used_colors);

    return 1;
}

static int palette_new_1n_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t levels = LUAX_UNSIGNED_RANGE(L, 1, 2, GL_PALETTE_MAX_COLORS);

    Palette_Object_t *self = (Palette_Object_t *)udt_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .palette = { 0 },
            .used_colors = 0
        }, OBJECT_TYPE_PALETTE);

    GL_palette_set_greyscale(self->palette, levels);

    self->used_colors = levels; // Levels are actually the amount of colors in the palette.

    LOG_D("greyscale palette %p allocated w/ %d color(s)", self, self->used_colors);

    return 1;
}

static int palette_new_1t_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    int colors_table = LUAX_TABLE(L, 1);

    size_t size = lua_rawlen(L, colors_table);
    LOG_D("setting custom palette of %d color(s)", size);

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (size > GL_PALETTE_MAX_COLORS) {
        return luaL_error(L, "palette has too many colors (%d) - max is %d", size, GL_PALETTE_MAX_COLORS);
    }
#endif  /* TOFU_CORE_DEFENSIVE_CHECKS */

    Palette_Object_t *self = (Palette_Object_t *)udt_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .palette = { 0 }
        }, OBJECT_TYPE_PALETTE);

    lua_pushnil(L); // T O -> T O N
    for (size_t i = 0; lua_next(L, colors_table); ++i) { // T O N -> T O N T
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

        uint8_t r = (uint8_t)LUAX_UNSIGNED_RANGE(L, -3, 0, GL_COLOR_LAST_VALUE);
        uint8_t g = (uint8_t)LUAX_UNSIGNED_RANGE(L, -2, 0, GL_COLOR_LAST_VALUE);
        uint8_t b = (uint8_t)LUAX_UNSIGNED_RANGE(L, -1, 0, GL_COLOR_LAST_VALUE);

        lua_pop(L, 3); // T O N T I I I -> T O N T

        self->palette[i] = gl_color_from_rgb(r, g, b);

        lua_pop(L, 1); // T O N T -> T O N
    }

    self->used_colors = size;

    LOG_D("palette %p allocated w/ %d color(s)", self, self->used_colors);

    return 1;
}

static int palette_new_1o_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Palette_Object_t *other = (const Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);

    LOG_D("cloning palette %p w/ %d color(s)", other, other->used_colors);

    Palette_Object_t *self = (Palette_Object_t *)udt_newobject(L, sizeof(Palette_Object_t),
        other, OBJECT_TYPE_PALETTE);

    LOG_D("palette %p allocated w/ %d color(s)", self, self->used_colors);

    return 1;
}

static int palette_new_3n_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    size_t red_bits = LUAX_UNSIGNED_RANGE(L, 1, 0, 8);
    size_t green_bits = LUAX_UNSIGNED_RANGE(L, 2, 0, 8);
    size_t blue_bits = LUAX_UNSIGNED_RANGE(L, 3, 0, 8);

    size_t bits = red_bits + green_bits + blue_bits;
    const size_t size = 1 << bits;

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (size == 0) {
        return luaL_error(L, "at least one bit is required (R%dG%dB%d == %d bits)", red_bits, green_bits, blue_bits, bits);
    } else
    if (size > GL_PALETTE_MAX_COLORS) {
        return luaL_error(L, "too many bits to fit palette (R%dG%dB%d == %d bits)", red_bits, green_bits, blue_bits, bits);
    }
#endif  /* TOFU_CORE_DEFENSIVE_CHECKS */

    Palette_Object_t *self = (Palette_Object_t *)udt_newobject(L, sizeof(Palette_Object_t), &(Palette_Object_t){
            .palette = { 0 },
            .used_colors = 0
        }, OBJECT_TYPE_PALETTE);

    GL_palette_set_quantized(self->palette, red_bits, green_bits, blue_bits);

    self->used_colors = size;

    LOG_D("quantized palette %p R%d:G%d:B%d generated (%d color(s))", self, red_bits, green_bits, blue_bits, size);

    return 1;
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

    lua_createtable(L, self->used_colors, 0);
    for (size_t i = 0; i < self->used_colors; ++i) {
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

    lua_pushinteger(L, (lua_Integer)self->used_colors);

    return 1;
}

int palette_peek_2on_3nnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const Palette_Object_t *self = (const Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED_RANGE(L, 2, 0, self->used_colors - 1);

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
    GL_Pixel_t index = (GL_Pixel_t)LUAX_UNSIGNED_RANGE(L, 2, 0, GL_PALETTE_LAST_INDEX);
    uint8_t r = (uint8_t)LUAX_UNSIGNED_RANGE(L, 3, 0, GL_COLOR_LAST_VALUE);
    uint8_t g = (uint8_t)LUAX_UNSIGNED_RANGE(L, 4, 0, GL_COLOR_LAST_VALUE);
    uint8_t b = (uint8_t)LUAX_UNSIGNED_RANGE(L, 5, 0, GL_COLOR_LAST_VALUE);

    GL_Color_t *palette = self->palette;
    const GL_Color_t color = gl_color_from_rgb(r, g, b);
    palette[index] = color;

    if (index >= self->used_colors) { // Update the colors count if we are peeking beyond the current bounds.
        self->used_colors = index + 1;
    }

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
    uint8_t r = (uint8_t)LUAX_UNSIGNED_RANGE(L, 2, 0, GL_COLOR_LAST_VALUE);
    uint8_t g = (uint8_t)LUAX_UNSIGNED_RANGE(L, 3, 0, GL_COLOR_LAST_VALUE);
    uint8_t b = (uint8_t)LUAX_UNSIGNED_RANGE(L, 4, 0, GL_COLOR_LAST_VALUE);
    float ratio = LUAX_OPTIONAL_NUMBER(L, 5, 0.5f);

    const GL_Color_t color = gl_color_from_rgb(r, g, b);

    GL_Color_t *palette = self->palette;
    GL_palette_lerp(palette, 0, self->used_colors, color, ratio);

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
    size_t to = LUAX_UNSIGNED_RANGE(L, 2, 0, GL_PALETTE_LAST_INDEX);
    const Palette_Object_t *other = (const Palette_Object_t *)LUAX_OBJECT(L, 3, OBJECT_TYPE_PALETTE);
    size_t from = LUAX_UNSIGNED_RANGE(L, 4, 0, other->used_colors - 1);
    size_t count = LUAX_UNSIGNED_RANGE(L, 5, 0, GL_PALETTE_MAX_COLORS - IMAX(from, to));
    bool remove_duplicates = LUAX_OPTIONAL_BOOLEAN(L, 6, true);

#if defined(TOFU_CORE_DEFENSIVE_CHECKS)
    if (self == other) {
        return luaL_error(L, "can't merge palette into itself");
    }
    if (to + count > GL_PALETTE_MAX_COLORS) {
        return luaL_error(L, "too many colors to merge into target palette (to %d + count %d > max %d)", to, count, GL_PALETTE_MAX_COLORS);
    }
    if (from + count > other->used_colors) {
        return luaL_error(L, "out of bounds source palette range (from %d + count %d > used colors %d)", from, count, other->used_colors);
    }
#endif /* TOFU_CORE_DEFENSIVE_CHECKS */

    GL_Color_t *palette = self->palette;
    GL_palette_merge(palette, to, other->palette, from, count, remove_duplicates);

    self->used_colors = to + count;

    return 0;
}

static int palette_resize_2on(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Palette_Object_t *self = (Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);
    size_t size = LUAX_UNSIGNED_RANGE(L, 2, 0, GL_PALETTE_MAX_COLORS);

    // If the next size is smaller than the current one, we just update the used
    // colors count, effectively "truncating" the palette. The actual colors are
    // not removed, but they will be ignored by all the functions that rely on
    // the used colors count.
    //
    // Otherwise, if the new size is bigger than the current one we pad the
    // palette extending the last color until the new size is reached. This way
    // we ensure that other functions (such as color matching) will work as
    // expected. Placing in the new slots "empty" colors (e.g. black) would be
    // arbitrary and an issue since it "alters" the palette's characteristics.

    if (size > self->used_colors) {
        GL_Color_t *palette = self->palette;
        GL_palette_pad(palette, self->used_colors, size);
    }

    self->used_colors = size;

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
    uint8_t r = (uint8_t)LUAX_UNSIGNED_RANGE(L, 2, 0, GL_COLOR_LAST_VALUE);
    uint8_t g = (uint8_t)LUAX_UNSIGNED_RANGE(L, 3, 0, GL_COLOR_LAST_VALUE);
    uint8_t b = (uint8_t)LUAX_UNSIGNED_RANGE(L, 4, 0, GL_COLOR_LAST_VALUE);

    const GL_Color_t color = gl_color_from_rgb(r, g, b);

    const GL_Color_t *palette = self->palette;
    const GL_Pixel_t index = GL_palette_find_nearest_color(palette, 0, self->used_colors, color);

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
    uint8_t ar = (uint8_t)LUAX_UNSIGNED_RANGE(L, 1, 0, GL_COLOR_LAST_VALUE);
    uint8_t ag = (uint8_t)LUAX_UNSIGNED_RANGE(L, 2, 0, GL_COLOR_LAST_VALUE);
    uint8_t ab = (uint8_t)LUAX_UNSIGNED_RANGE(L, 3, 0, GL_COLOR_LAST_VALUE);
    uint8_t br = (uint8_t)LUAX_UNSIGNED_RANGE(L, 4, 0, GL_COLOR_LAST_VALUE);
    uint8_t bg = (uint8_t)LUAX_UNSIGNED_RANGE(L, 5, 0, GL_COLOR_LAST_VALUE);
    uint8_t bb = (uint8_t)LUAX_UNSIGNED_RANGE(L, 6, 0, GL_COLOR_LAST_VALUE);
    float ratio = LUAX_OPTIONAL_NUMBER(L, 7, 0.5f);

    const GL_Color_t a = gl_color_from_rgb(ar, ag, ab);
    const GL_Color_t b = gl_color_from_rgb(br, bg, bb);

    const GL_Color_t color = GL_palette_mix(a, b, ratio);

    lua_pushinteger(L, (lua_Integer)gl_color_get_r(color));
    lua_pushinteger(L, (lua_Integer)gl_color_get_g(color));
    lua_pushinteger(L, (lua_Integer)gl_color_get_b(color));

    return 3;
}
