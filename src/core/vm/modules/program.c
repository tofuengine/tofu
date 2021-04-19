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
#include <libs/log.h>
#include <libs/luax.h>

#include "udt.h"

#define LOG_CONTEXT "program"
#define META_TABLE  "Tofu_Graphics_Program_mt"

static int program_new_0_1u(lua_State *L);
static int program_gc_1u_0(lua_State *L);
static int program_wait_3unn_0(lua_State *L);
static int program_modulo_2un_0(lua_State *L);
static int program_offset_2un_0(lua_State *L);
static int program_color_5unnnn_0(lua_State *L);
static int program_shift_v_0(lua_State *L);

static const struct luaL_Reg _program_functions[] = {
    { "new", program_new_0_1u },
    { "__gc", program_gc_1u_0 },
    { "wait", program_wait_3unn_0 },
    { "modulo", program_modulo_2un_0 },
    { "offset", program_offset_2un_0 },
    { "color", program_color_5unnnn_0 },
    { "shift", program_shift_v_0 },
    { NULL, NULL }
};

static const uint8_t _program_lua[] = {
#include "program.inc"
};

static luaX_Script _program_script = { (const char *)_program_lua, sizeof(_program_lua), "@program.lua" }; // Trace as filename internally.

int program_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_program_script, _program_functions, NULL, nup, META_TABLE);
}

static int program_new_0_1u(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    GL_Program_t *program = GL_program_create();
    if (!program) {
        return luaL_error(L, "can't create program");
    }

    Program_Object_t *self = (Program_Object_t *)lua_newuserdatauv(L, sizeof(Program_Object_t), 1);
    *self = (Program_Object_t){
            .program = program
        };
//    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program %p allocated", self);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int program_gc_1u_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_USERDATA(L, 1);

    GL_program_destroy(self->program);
//    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "program %p finalized", self);

    return 0;
}

static int program_wait_3unn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_USERDATA(L, 1);
    size_t x = (size_t)LUAX_INTEGER(L, 2);
    size_t y = (size_t)LUAX_INTEGER(L, 3);

    GL_program_wait(self->program, x, y);

    return 0;
}

static int program_modulo_2un_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_USERDATA(L, 1);
    int amount = LUAX_INTEGER(L, 2);

    GL_program_modulo(self->program, amount);

    return 0;
}

static int program_offset_2un_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_USERDATA(L, 1);
    int amount = LUAX_INTEGER(L, 2);

    GL_program_offset(self->program, amount);

    return 0;
}

static int program_color_5unnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    uint8_t r = (uint8_t)LUAX_INTEGER(L, 3);
    uint8_t g = (uint8_t)LUAX_INTEGER(L, 4);
    uint8_t b = (uint8_t)LUAX_INTEGER(L, 5);

    const GL_Color_t color = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };

    GL_program_color(self->program, index, color);

    return 0;
}

static int program_shift_2ut_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        const GL_Pixel_t from = (GL_Pixel_t)LUAX_INTEGER(L, -2);
        const GL_Pixel_t to = (GL_Pixel_t)LUAX_INTEGER(L, -1);

        GL_program_shift(self->program, from, to);

        lua_pop(L, 1);
    }

    return 0;
}

static int program_shift_3unn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Program_Object_t *self = (Program_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pixel_t from = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    GL_Pixel_t to = (GL_Pixel_t)LUAX_INTEGER(L, 3);

    GL_program_shift(self->program, from, to);

    return 0;
}

static int program_shift_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, program_shift_2ut_0)
        LUAX_OVERLOAD_ARITY(3, program_shift_3unn_0)
    LUAX_OVERLOAD_END
}
