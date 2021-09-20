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

#include "program.h"

#include <config.h>
#include <core/io/display.h>
#include <libs/fmath.h>
#include <libs/log.h>
#include <libs/luax.h>

#include "udt.h"

#define LOG_CONTEXT "program"
#define META_TABLE  "Tofu_Graphics_Program_mt"
#define SCRIPT_NAME "@program.lua"

static int program_new_0_1o(lua_State *L);
static int program_gc_1o_0(lua_State *L);
static int program_clear_1o_0(lua_State *L);
static int program_wait_3onn_0(lua_State *L);
static int program_modulo_2on_0(lua_State *L);
static int program_offset_2on_0(lua_State *L);
static int program_color_5onnnn_0(lua_State *L);
static int program_shift_v_0(lua_State *L);
static int program_gradient_3ont_0(lua_State *L);
static int program_palette_4onnt_0(lua_State *L);
// TODO: add program `merging`
// TODO: add some helper functions to populate the program.

int program_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", program_new_0_1o },
            { "__gc", program_gc_1o_0 },
            { "clear", program_clear_1o_0 },
            { "wait", program_wait_3onn_0 },
            { "modulo", program_modulo_2on_0 },
            { "offset", program_offset_2on_0 },
            { "color", program_color_5onnnn_0 },
            { "shift", program_shift_v_0 },
            { "gradient", program_gradient_3ont_0 },
            { "palette", program_palette_4onnt_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int program_new_0_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    GL_Program_t *program = GL_program_create();
    if (!program) {
        return luaL_error(L, "can't create program");
    }

    Program_Object_t *self = (Program_Object_t *)luaX_newobject(L, sizeof(Program_Object_t), &(Program_Object_t){
            .program = program
        }, OBJECT_TYPE_PROGRAM, META_TABLE);

#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program %p allocated", self);
#else  /* VERBOSE_DEBUG */
    (void)self;
#endif  /* VERBOSE_DEBUG */

    return 1;
}

static int program_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PROGRAM);

    GL_program_destroy(self->program);

#ifdef VERBOSE_DEBUG
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program %p finalized", self);
#endif  /* VERBOSE_DEBUG */

    return 0;
}

static int program_clear_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PROGRAM);

    GL_program_clear(self->program);

    return 0;
}

static int program_wait_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PROGRAM);
    size_t x = (size_t)LUAX_INTEGER(L, 2);
    size_t y = (size_t)LUAX_INTEGER(L, 3);

    GL_program_wait(self->program, x, y);

    return 0;
}

static int program_modulo_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PROGRAM);
    int amount = LUAX_INTEGER(L, 2);

    GL_program_modulo(self->program, amount);

    return 0;
}

static int program_offset_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PROGRAM);
    int amount = LUAX_INTEGER(L, 2);

    GL_program_offset(self->program, amount);

    return 0;
}

static int program_color_5onnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PROGRAM);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    uint8_t r = (uint8_t)LUAX_INTEGER(L, 3);
    uint8_t g = (uint8_t)LUAX_INTEGER(L, 4);
    uint8_t b = (uint8_t)LUAX_INTEGER(L, 5);

    const GL_Color_t color = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };

    GL_program_color(self->program, index, color);

    return 0;
}

static int program_shift_2ot_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PROGRAM);
    // idx #2: LUA_TTABLE

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        const GL_Pixel_t from = (GL_Pixel_t)LUAX_INTEGER(L, -2);
        const GL_Pixel_t to = (GL_Pixel_t)LUAX_INTEGER(L, -1);

        GL_program_shift(self->program, from, to);

        lua_pop(L, 1);
    }

    return 0;
}

static int program_shift_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PROGRAM);
    GL_Pixel_t from = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    GL_Pixel_t to = (GL_Pixel_t)LUAX_INTEGER(L, 3);

    GL_program_shift(self->program, from, to);

    return 0;
}

static int program_shift_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, program_shift_2ot_0)
        LUAX_OVERLOAD_ARITY(3, program_shift_3onn_0)
    LUAX_OVERLOAD_END
}

static int program_gradient_3ont_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PROGRAM);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    // idx #3: LUA_TTABLE

    size_t current_y = 0;
    uint8_t current_r = 0, current_g = 0, current_b = 0;

    lua_pushnil(L); // O N T -> O N T N
    for (size_t i = 0; lua_next(L, 3); ++i) { // O N T N -> O N T N T
#ifdef __DEFENSIVE_CHECKS__
        int count = lua_rawlen(L, 5);
        if (count != 4) {
            luaL_error(L, "marker #%d has %d components (out of 4 required)", i, count);
        }
#endif /* __DEFENSIVE_CHECKS__ */
        lua_rawgeti(L, 5, 1); // O N T N T -> O N T N T I
        lua_rawgeti(L, 5, 2); // O N T N T I -> O N T N T I I
        lua_rawgeti(L, 5, 3); // O N T N T I I -> O N T N T I I I
        lua_rawgeti(L, 5, 4); // O N T N T I I -> O N T N T I I I I

        const size_t wait_y = (size_t)LUAX_INTEGER(L, -4);
        const uint8_t wait_r = (uint8_t)LUAX_INTEGER(L, -3);
        const uint8_t wait_g = (uint8_t)LUAX_INTEGER(L, -2);
        const uint8_t wait_b = (uint8_t)LUAX_INTEGER(L, -1);

        lua_pop(L, 4); // O N T N T I I I I -> O N T N T

        for (size_t y = current_y; y < wait_y; ++y) { // Skip target scanline, will be added on the next loop.
            const float ratio = (float)(y - current_y) / (float)(wait_y - current_y);
            const uint8_t r = (uint8_t)FLERP(current_r, wait_r, ratio);
            const uint8_t g = (uint8_t)FLERP(current_g, wait_g, ratio);
            const uint8_t b = (uint8_t)FLERP(current_b, wait_b, ratio);
            GL_program_wait(self->program, 0, y);
            const GL_Color_t color = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };
            GL_program_color(self->program, index, color);
        }

        current_y = wait_y;
        current_r = wait_r;
        current_g = wait_g;
        current_b = wait_b;

        lua_pop(L, 1); // O N T N T -> O N T N
    }

    GL_program_wait(self->program, 0, current_y);
    const GL_Color_t color = (GL_Color_t){ .r = current_r, .g = current_g, .b = current_b, .a = 255 };
    GL_program_color(self->program, index, color);

    return 0;
}

static int program_palette_4onnt_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PROGRAM);
    // idx #2: LUA_TTABLE
    size_t x = (size_t)LUAX_INTEGER(L, 3);
    size_t y = (size_t)LUAX_INTEGER(L, 4);

    GL_program_wait(self->program, x, y);

    lua_pushnil(L); // O T N N -> O T N N N
    for (size_t i = 0; lua_next(L, 2); ++i) { // O T N N N -> O T N N N T
        const GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, -2);

#ifdef __DEFENSIVE_CHECKS__
        int count = lua_rawlen(L, 6);
        if (count != 3) {
            luaL_error(L, "palette entry #%d has %d components (out of 3 required)", i, count);
        }
#endif /* __DEFENSIVE_CHECKS__ */
        lua_rawgeti(L, 6, 1); // O T N N N T -> O T N N N T I
        lua_rawgeti(L, 6, 2); // O T N N N T I -> O T N N N T I I
        lua_rawgeti(L, 6, 3); // O T N N N T I I -> O T N N N T I I I

        const uint8_t r = (uint8_t)LUAX_INTEGER(L, -3);
        const uint8_t g = (uint8_t)LUAX_INTEGER(L, -2);
        const uint8_t b = (uint8_t)LUAX_INTEGER(L, -1);

        lua_pop(L, 3); // O T N T I I I -> O T N T

        const GL_Color_t color = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };
        GL_program_color(self->program, index, color);

        lua_pop(L, 1); // O T N N N T -> O T N N N
    }

    return 0;
}