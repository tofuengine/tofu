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

#include "world.h"

#include <config.h>
#include <core/physics.h>
#include <libs/log.h>

#include "udt.h"

#define LOG_CONTEXT "world"
#define META_TABLE  "Tofu_World_Body_mt"

static int world_gravity_v_v(lua_State *L);
static int world_damping_v_v(lua_State *L);

int world_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "gravity", world_gravity_v_v },
            { "damping", world_damping_v_v },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int world_gravity_0_2n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Physics_t *physics = (const Physics_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_PHYSICS));

    const PL_World_t *world = physics->world;
    const PL_Vector_t position = PL_world_get_gravity(world);

    lua_pushnumber(L, (lua_Number)position.x);
    lua_pushnumber(L, (lua_Number)position.y);

    return 2;
}

static int world_gravity_2nn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    PL_Float_t x = (PL_Float_t)LUAX_NUMBER(L, 1);
    PL_Float_t y = (PL_Float_t)LUAX_NUMBER(L, 2);

    Physics_t *physics = (Physics_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_PHYSICS));

    PL_World_t *world = physics->world;
    PL_world_set_gravity(world, (PL_Vector_t){ .x = x, .y = y });

    return 0;
}

static int world_gravity_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, world_gravity_0_2n)
        LUAX_OVERLOAD_ARITY(2, world_gravity_2nn_0)
    LUAX_OVERLOAD_END
}

static int world_damping_0_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    const Physics_t *physics = (const Physics_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_PHYSICS));

    const PL_World_t *world = physics->world;
    const PL_Float_t damping = PL_world_get_damping(world);

    lua_pushnumber(L, (lua_Number)damping);

    return 1;
}

static int world_damping_1n_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    PL_Float_t damping = (PL_Float_t)LUAX_NUMBER(L, 1);

    Physics_t *physics = (Physics_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_PHYSICS));

    PL_World_t *world = physics->world;
    PL_world_set_damping(world, damping);

    return 0;
}

static int world_damping_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, world_damping_0_1n)
        LUAX_OVERLOAD_ARITY(1, world_damping_1n_0)
    LUAX_OVERLOAD_END
}
