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

#include "world.h"

#include "internal/udt.h"

#include <core/config.h>
#include <libs/log.h>
#include <libs/stb.h>
#include <libs/path.h>
#include <systems/storage.h>

#include <chipmunk/chipmunk.h>

#define LOG_CONTEXT "world"
#define MODULE_NAME "tofu.physics.world"
#define META_TABLE  "Tofu_Physics_World_mt"

static int world_new_v_1o(lua_State *L);
static int world_gc_1o_0(lua_State *L);
static int world_gravity_v_v(lua_State *L);
static int world_damping_v_v(lua_State *L);
static int world_update_2on_0(lua_State *L);
static int world_add_2oo_0(lua_State *L);
static int world_remove_2oo_0(lua_State *L);
static int world_clear_1o_0(lua_State *L);

int world_loader(lua_State *L)
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
            { "new", world_new_v_1o },
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

static int world_new_2NN_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
        LUAX_SIGNATURE_OPTIONAL(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    cpFloat x = (cpFloat)LUAX_OPTIONAL_NUMBER(L, 1, 0.0);
    cpFloat y = (cpFloat)LUAX_OPTIONAL_NUMBER(L, 2, 0.0);

    cpSpace *space = cpSpaceNew();
    if (!space) {
        return luaL_error(L, "can't create space");
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "space %p created", space);

    cpSpaceSetGravity(space, (cpVect){ .x = x, .y = y });
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "gravity set to <%.3f, %.3f> for space %p", x, y, space);

    World_Object_t *self = (World_Object_t *)luaX_newobject(L, sizeof(World_Object_t), &(World_Object_t){
            .space = space
        }, OBJECT_TYPE_WORLD, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p created", self);

    return 1;
}

static int world_new_v_1o(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(0, world_new_2NN_1o)
        LUAX_OVERLOAD_ARITY(2, world_new_2NN_1o)
    LUAX_OVERLOAD_END
}

static inline void _release(lua_State *L, World_Object_t *world)
{
    for (size_t i = 0; i < hmlenu(world->entries); ++i) {
        World_Object_Entry_t *entry = &world->entries[i];

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
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p entries cleared", self);

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

    int index = hmgeti(self->entries, body);
    if (index != -1) {
        luaL_error(L, "body %p already in world %p", body, self);
    }

    cpSpace *space = self->space;
    cpSpaceAddBody(space, body->body);
    cpSpaceAddShape(space, body->shape);

    luaX_Reference reference = luaX_ref(L, 2);

    hmput(self->entries, body, reference);
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

    int index = hmgeti(self->entries, body);
    if (index == -1) {
        luaL_error(L, "body %p not in world %p", body, self);
    }
    World_Object_Entry_t *entry = &self->entries[index];

    cpSpace *space = self->space;
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

    cpSpaceDestroy(self->space);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world space %p destroyed", self->space);

    _release(L, self);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p entries cleared", self);

    return 0;
}
