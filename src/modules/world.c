/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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
#include <libs/log.h>
#include <libs/stb.h>

#include <chipmunk/chipmunk.h>

#include "udt.h"

#define LOG_CONTEXT "world"
#define META_TABLE  "Tofu_World_Body_mt"

static int world_new_0_1o(lua_State *L);
static int world_gc_1o_0(lua_State *L);
static int world_gravity_v_v(lua_State *L);
static int world_damping_v_v(lua_State *L);
static int world_update_2on_0(lua_State *L);
static int world_add_2oo_0(lua_State *L);
static int world_remove_2oo_0(lua_State *L);
static int world_clear_1o_0(lua_State *L);

int world_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", world_new_0_1o },
            { "__gc", world_gc_1o_0 },
            { "gravity", world_gravity_v_v },
            { "damping", world_damping_v_v },
            { "update", world_update_2on_0 },
            { "add", world_add_2oo_0 },
            { "remove", world_remove_2oo_0 },
            { "clear", world_clear_1o_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static int world_new_0_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
    LUAX_SIGNATURE_END

    cpSpace *space = cpSpaceNew();
    if (!space) {
        return luaL_error(L, "can't create space");
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "space %p created", space);

    World_Object_t *self = (World_Object_t *)luaX_newobject(L, sizeof(World_Object_t), &(World_Object_t){
            .space = space
        }, OBJECT_TYPE_WORLD, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p created", self);

    return 1;
}

static inline void _release(lua_State *L, World_Object_t *world)
{
    cpSpace *space = world->space;
    for (size_t i = 0; i < hmlenu(world->entries); ++i) {
        World_Object_Entry_t *entry = &world->entries[i];

        const Body_Object_t *body = entry->key;
        cpSpaceRemoveShape(space, body->shape);
        cpSpaceRemoveBody(space, body->body);

        luaX_Reference reference = entry->value;
        luaX_unref(L, reference);
    }
    hmfree(world->entries);
}

static int world_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    World_Object_t *self = (World_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WORLD);

    _release(L, self);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world entries freed");

    cpSpaceFree(self->space);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world space %p destroyed", self->space);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p finalized", self);

    return 0;
}

static int world_gravity_1o_2n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    World_Object_t *self = (World_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WORLD);

    const cpSpace *space = self->space;
    const cpVect position = cpSpaceGetGravity(space);

    lua_pushnumber(L, (lua_Number)position.x);
    lua_pushnumber(L, (lua_Number)position.y);

    return 2;
}

static int world_gravity_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    World_Object_t *self = (World_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WORLD);
    cpFloat x = (cpFloat)LUAX_NUMBER(L, 2);
    cpFloat y = (cpFloat)LUAX_NUMBER(L, 3);

    cpSpace *space = self->space;
    cpSpaceSetGravity(space, (cpVect){ .x = x, .y = y });

    return 0;
}

static int world_gravity_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, world_gravity_1o_2n)
        LUAX_OVERLOAD_ARITY(3, world_gravity_3onn_0)
    LUAX_OVERLOAD_END
}

static int world_damping_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    World_Object_t *self = (World_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WORLD);

    const cpSpace *space = self->space;
    const cpFloat damping = cpSpaceGetDamping(space);

    lua_pushnumber(L, (lua_Number)damping);

    return 1;
}

static int world_damping_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    World_Object_t *self = (World_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WORLD);
    cpFloat damping = (cpFloat)LUAX_NUMBER(L, 2);

    cpSpace *space = self->space;
    cpSpaceSetDamping(space, damping);

    return 0;
}

static int world_damping_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, world_damping_1o_1n)
        LUAX_OVERLOAD_ARITY(2, world_damping_2on_0)
    LUAX_OVERLOAD_END
}

static int world_update_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    World_Object_t *self = (World_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WORLD);
    cpFloat delta_time = (cpFloat)LUAX_NUMBER(L, 2);

    cpSpace *space = self->space;
    cpSpaceStep(space, delta_time);

    return 0;
}

static int world_add_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    World_Object_t *self = (World_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WORLD);
    const Body_Object_t *body = (const Body_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_BODY);

    cpSpace *space = self->space;
    if (cpSpaceContainsBody(space, body->body) == cpTrue) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p already bound to world %p", body, self);
        return 0;
    }

    cpSpaceAddBody(space, body->body);
    cpSpaceAddShape(space, body->shape);

    hmput(self->entries, body, luaX_ref(L, 2));
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p bound to world %p", body, self);

    return 0;
}

static int world_remove_2oo_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    World_Object_t *self = (World_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WORLD);
    const Body_Object_t *body = (const Body_Object_t *)LUAX_OBJECT(L, 2, OBJECT_TYPE_BODY);

    cpSpace *space = self->space;
    if (cpSpaceContainsBody(space, body->body) == cpFalse) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p already not bound to world %p", body, self);
        return 0;
    }

    int index = hmgeti(self->entries, body);
    if (index == -1) {
        luaL_error(L, "body %p not found in reference cache", body);
    }
    World_Object_Entry_t *entry = &self->entries[index];

    cpSpaceRemoveShape(space, body->shape);
    cpSpaceRemoveBody(space, body->body);

    luaX_Reference reference = entry->value;
    luaX_unref(L, reference);

    int deleted = hmdel(self->entries, body);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p found and removed from world %p (%d)", body, self, deleted);

    return 0;
}

static int world_clear_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    World_Object_t *self = (World_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_WORLD);

    _release(L, self);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world entries cleared");

    return 0;
}
