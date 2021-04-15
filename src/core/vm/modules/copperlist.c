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

#include "xform.h"

#include <config.h>
#include <core/io/display.h>
#include <libs/log.h>
#include <libs/luax.h>
#include <libs/stb.h>

#include "udt.h"

#define LOG_CONTEXT "copperlist"
#define META_TABLE  "Tofu_Graphics_Copperlist_mt"

static int copperlist_new_0_1u(lua_State *L);
static int copperlist_gc_1u_0(lua_State *L);
static int copperlist_wait_3unn_0(lua_State *L);
static int copperlist_modulo_2un_0(lua_State *L);
static int copperlist_offset_2un_0(lua_State *L);
static int copperlist_palette_2un_0(lua_State *L);
static int copperlist_color_5unnnn_0(lua_State *L);
static int copperlist_bias_2un_0(lua_State *L);
static int copperlist_shift_v_0(lua_State *L);

static const struct luaL_Reg _copperlist_functions[] = {
    { "new", copperlist_new_0_1u },
    { "__gc", copperlist_gc_1u_0 },
    { "wait", copperlist_wait_3unn_0 },
    { "modulo", copperlist_modulo_2un_0 },
    { "offset", copperlist_offset_2un_0 },
    { "palette", copperlist_palette_2un_0 },
    { "color", copperlist_color_5unnnn_0 },
    { "bias", copperlist_bias_2un_0 },
    { "shift", copperlist_shift_v_0 },
    { NULL, NULL }
};

static const uint8_t _copperlist_lua[] = {
#include "copperlist.inc"
};

static luaX_Script _copperlist_script = { (const char *)_copperlist_lua, sizeof(_copperlist_lua), "@copperlist.lua" }; // Trace as filename internally.


int copperlist_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, &_copperlist_script, _copperlist_functions, NULL, nup, META_TABLE);
}

static int copperlist_new_0_1u(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    Copperlist_Object_t *self = (Copperlist_Object_t *)lua_newuserdatauv(L, sizeof(Copperlist_Object_t), 1);
    *self = (Copperlist_Object_t){
            .program = NULL
        };
//    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist %p allocated", self);

    luaL_setmetatable(L, META_TABLE);

    return 1;
}

static int copperlist_gc_1u_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
    LUAX_SIGNATURE_END
    Copperlist_Object_t *self = (Copperlist_Object_t *)LUAX_USERDATA(L, 1);

    if (self->program) {
        arrfree(self->program);
//        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist program %p freed", self->program);
    }

//    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "copperlist %p finalized", self);

    return 0;
}

static int copperlist_wait_3unn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Copperlist_Object_t *self = (Copperlist_Object_t *)LUAX_USERDATA(L, 1);
    size_t x = (size_t)LUAX_INTEGER(L, 2);
    size_t y = (size_t)LUAX_INTEGER(L, 3);

    arrpush(self->program, (Display_CopperList_Entry_t){ .command = WAIT }); // TODO: create MACROs/functions to populate copperlist.
    arrpush(self->program, (Display_CopperList_Entry_t){ .size = x }); // TODO: move the copperlist to `GL` and add functions for rendering?
    arrpush(self->program, (Display_CopperList_Entry_t){ .size = y });

    return 0;
}

static int copperlist_modulo_2un_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Copperlist_Object_t *self = (Copperlist_Object_t *)LUAX_USERDATA(L, 1);
    int amount = LUAX_INTEGER(L, 2);

    arrpush(self->program, (Display_CopperList_Entry_t){ .command = MODULO });
    arrpush(self->program, (Display_CopperList_Entry_t){ .integer = amount });

    return 0;
}

static int copperlist_offset_2un_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Copperlist_Object_t *self = (Copperlist_Object_t *)LUAX_USERDATA(L, 1);
    int amount = LUAX_INTEGER(L, 2);

    arrpush(self->program, (Display_CopperList_Entry_t){ .command = OFFSET });
    arrpush(self->program, (Display_CopperList_Entry_t){ .integer = amount });

    return 0;
}

static int copperlist_palette_2un_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Copperlist_Object_t *self = (Copperlist_Object_t *)LUAX_USERDATA(L, 1);
    size_t id = (size_t)LUAX_INTEGER(L, 2);

    arrpush(self->program, (Display_CopperList_Entry_t){ .command = PALETTE });
    arrpush(self->program, (Display_CopperList_Entry_t){ .size = id });

    return 0;
}

static int copperlist_color_5unnnn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Copperlist_Object_t *self = (Copperlist_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pixel_t index = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    uint8_t r = (uint8_t)LUAX_INTEGER(L, 3);
    uint8_t g = (uint8_t)LUAX_INTEGER(L, 4);
    uint8_t b = (uint8_t)LUAX_INTEGER(L, 5);

    const GL_Color_t color = (GL_Color_t){ .r = r, .g = g, .b = b, .a = 255 };

    arrpush(self->program, (Display_CopperList_Entry_t){ .command = COLOR });
    arrpush(self->program, (Display_CopperList_Entry_t){ .pixel = index });
    arrpush(self->program, (Display_CopperList_Entry_t){ .color = color });

    return 0;
}

static int copperlist_bias_2un_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Copperlist_Object_t *self = (Copperlist_Object_t *)LUAX_USERDATA(L, 1);
    int value = LUAX_INTEGER(L, 2);

    arrpush(self->program, (Display_CopperList_Entry_t){ .command = BIAS });
    arrpush(self->program, (Display_CopperList_Entry_t){ .integer = value });

    return 0;
}

static int copperlist_shift_2ut_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TTABLE)
    LUAX_SIGNATURE_END
    Copperlist_Object_t *self = (Copperlist_Object_t *)LUAX_USERDATA(L, 1);

    lua_pushnil(L);
    while (lua_next(L, 2)) {
        const GL_Pixel_t from = (GL_Pixel_t)LUAX_INTEGER(L, -2);
        const GL_Pixel_t to = (GL_Pixel_t)LUAX_INTEGER(L, -1);

        arrpush(self->program, (Display_CopperList_Entry_t){ .command = SHIFT });
        arrpush(self->program, (Display_CopperList_Entry_t){ .pixel = from });
        arrpush(self->program, (Display_CopperList_Entry_t){ .pixel = to });

        lua_pop(L, 1);
    }

    return 0;
}

static int copperlist_shift_3unn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TUSERDATA)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Copperlist_Object_t *self = (Copperlist_Object_t *)LUAX_USERDATA(L, 1);
    GL_Pixel_t from = (GL_Pixel_t)LUAX_INTEGER(L, 2);
    GL_Pixel_t to = (GL_Pixel_t)LUAX_INTEGER(L, 3);

    arrpush(self->program, (Display_CopperList_Entry_t){ .command = SHIFT });
    arrpush(self->program, (Display_CopperList_Entry_t){ .pixel = from });
    arrpush(self->program, (Display_CopperList_Entry_t){ .pixel = to });

    return 0;
}

static int copperlist_shift_v_0(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, copperlist_shift_2ut_0)
        LUAX_OVERLOAD_ARITY(3, copperlist_shift_3unn_0)
    LUAX_OVERLOAD_END
}
