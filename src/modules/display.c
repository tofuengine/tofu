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

#include "display.h"

#include <config.h>
#include <libs/log.h>
#include <libs/stb.h>
#include <systems/display.h>

#include "udt.h"

#define LOG_CONTEXT "display"

static int display_palette_1o_0(lua_State *L);
static int display_offset_2NN_0(lua_State *L);
static int display_shift_v_0(lua_State *L);
static int display_program_1O_0(lua_State *L);
static int display_reset_0_0(lua_State *L);

int display_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "palette", display_palette_1o_0 },
            { "offset", display_offset_2NN_0 },
            { "shift", display_shift_v_0 },
            { "program", display_program_1O_0 },
            { "reset", display_reset_0_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, NULL);
}

static int display_palette_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Palette_Object_t *palette = (const Palette_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_PALETTE);

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_set_palette(display, palette->palette);

    return 0;
}

static int display_offset_2NN_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    int x = LUAX_OPTIONAL_INTEGER(L, 1, 0);
    int y = LUAX_OPTIONAL_INTEGER(L, 2, 0);

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_set_offset(display, (GL_Point_t){ .x = x, .y = y });

    return 0;
}

static int display_shift_0_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_set_shifting(display, NULL, NULL, 0);

    return 0;
}

static int display_shift_1t_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    // idx #1: LUA_TTABLE

    GL_Pixel_t *from = NULL;
    GL_Pixel_t *to = NULL;

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        arrpush(from, (GL_Pixel_t)LUAX_INTEGER(L, -2));
        arrpush(to, (GL_Pixel_t)LUAX_INTEGER(L, -1));

        lua_pop(L, 1);
    }

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_set_shifting(display, from, to, arrlenu(from));

    arrfree(from);
    arrfree(to);

    return 0;
}

static int display_shift_2nn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    GL_Pixel_t from = (GL_Pixel_t)LUAX_INTEGER(L, 1);
    GL_Pixel_t to = (GL_Pixel_t)LUAX_INTEGER(L, 2);

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_set_shifting(display, &from, &to, 1);

    return 0;
}

static int display_shift_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, display_shift_0_0)
        LUAX_OVERLOAD_ARITY(1, display_shift_1t_0)
        LUAX_OVERLOAD_ARITY(2, display_shift_2nn_0)
    LUAX_OVERLOAD_END
}

static int display_program_1O_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Program_Object_t *program = (const Program_Object_t *)LUAX_OPTIONAL_OBJECT(L, 1, OBJECT_TYPE_PROGRAM, NULL);

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    if (program) {
        Display_set_program(display, program->program);
    } else {
        Display_set_program(display, NULL);
    }

    return 0;
}

static int display_reset_0_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Display_t *display = (Display_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_DISPLAY));

    Display_reset(display);

    return 0;
}
