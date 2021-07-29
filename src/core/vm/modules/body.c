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

#include "body.h"

#include <config.h>
#include <core/physics.h>
#include <libs/log.h>

#include "udt.h"
#include "utils/map.h"

#define LOG_CONTEXT "body"
#define META_TABLE  "Tofu_Physics_Body_mt"

static int body_new_v_1o(lua_State *L);
static int body_gc_1o_0(lua_State *L);
static int body_mass_v_v(lua_State *L);
static int body_momentum_v_v(lua_State *L);
static int body_position_v_v(lua_State *L);
static int body_velocity_v_v(lua_State *L);
static int body_angle_v_v(lua_State *L);

int body_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L, (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            { "new", body_new_v_1o },
            { "__gc", body_gc_1o_0 },
            { "mass", body_mass_v_v },
            { "momentum", body_momentum_v_v },
            { "position", body_position_v_v },
            { "velocity", body_velocity_v_v },
            { "angle", body_angle_v_v },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, META_TABLE);
}

static const Map_Entry_t _types[3] = { // Ditto.
    { "dynamic", CP_BODY_TYPE_DYNAMIC },
    { "kinematic", CP_BODY_TYPE_KINEMATIC },
    { "static", CP_BODY_TYPE_STATIC }
};

static int body_new_4snnn_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *type = (const char *)LUAX_STRING(L, 1);
    size_t width = (size_t)LUAX_INTEGER(L, 2);
    size_t height = (size_t)LUAX_INTEGER(L, 3);
    size_t radius = (size_t)LUAX_INTEGER(L, 4);

    Physics_t *physics = (Physics_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_PHYSICS));

    cpBody *body = cpBodyNew(0.0, 0.0);
    if (!body) {
        return luaL_error(L, "can't create body");
    }
//    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p created for world %p", body, physics->world);

    const Map_Entry_t *entry = map_find(L, type, _types, 3);
    cpBodySetType(body, (cpBodyType)entry->value);

    cpShape *shape = cpBoxShapeNew(body, (cpFloat)width, (cpFloat)height, (cpFloat)radius);
    if (!shape) {
        cpBodyFree(body);
        return luaL_error(L, "can't create shape");
    }

    cpShapeSetElasticity(shape, 0.90);

    cpSpaceAddBody(physics->space, body);
    cpSpaceAddShape(physics->space, shape);

    Body_Object_t *self = (Body_Object_t *)luaX_newobject(L, sizeof(Body_Object_t), &(Body_Object_t){
            .body = body,
            .shape = shape,
            .type = BODY_TYPE_BOX,
            .size = {
                .box = {
                    .width = width,
                    .height = height
                }
            }
        }, OBJECT_TYPE_BODY, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p created w/ type `%s`", self, type);

    return 1;
}

static int body_new_2sn_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TSTRING)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    const char *type = (const char *)LUAX_STRING(L, 1);
    size_t radius = (size_t)LUAX_INTEGER(L, 2);

    Physics_t *physics = (Physics_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_PHYSICS));

    cpBody *body = cpBodyNew(0.0, 0.0);
    if (!body) {
        return luaL_error(L, "can't create body");
    }
//    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p created for world %p", body, physics->world);

    const Map_Entry_t *entry = map_find(L, type, _types, 3);
    cpBodySetType(body, (cpBodyType)entry->value);

    cpShape *shape = cpCircleShapeNew(body, (cpFloat)radius, (cpVect){ .x = 0.0, .y = 0.0 });
    if (!shape) {
        cpBodyFree(body);
        return luaL_error(L, "can't create shape");
    }

    cpSpaceAddBody(physics->space, body);
    cpSpaceAddShape(physics->space, shape);

    Body_Object_t *self = (Body_Object_t *)luaX_newobject(L, sizeof(Body_Object_t), &(Body_Object_t){
            .body = body,
            .shape = shape,
            .type = BODY_TYPE_CIRCLE,
            .size = {
                .circle = {
                    .radius = radius
                }
            }
        }, OBJECT_TYPE_BODY, META_TABLE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p created w/ type `%s`", self, type);

    return 1;
}

static int body_new_v_1o(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(2, body_new_2sn_1o)
        LUAX_OVERLOAD_ARITY(4, body_new_4snnn_1o)
    LUAX_OVERLOAD_END
}

static int body_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    Physics_t *physics = (Physics_t *)LUAX_USERDATA(L, lua_upvalueindex(USERDATA_PHYSICS));

    cpSpaceRemoveShape(physics->space, self->shape);
    cpShapeFree(self->shape);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shape %p destroyed", self->shape);

    cpSpaceRemoveBody(physics->space, self->body);
    cpBodyFree(self->body);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p destroyed", self->body);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p finalized", self);

    return 0;
}

static int body_mass_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    const cpBody *body = self->body;
    const cpFloat mass = cpBodyGetMass(body);

    lua_pushnumber(L, (lua_Number)mass);

    return 1;
}

static int body_mass_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);
    cpFloat mass = (cpFloat)LUAX_NUMBER(L, 2);

    cpBody *body = self->body;
    cpBodySetMass(body, mass);

    return 0;
}

static int body_mass_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, body_mass_1o_1n)
        LUAX_OVERLOAD_ARITY(2, body_mass_2on_0)
    LUAX_OVERLOAD_END
}

static int body_momentum_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    const cpBody *body = self->body;
    const cpFloat momentum = cpBodyGetMoment(body);

    lua_pushnumber(L, (lua_Number)momentum);

    return 1;
}

static int body_momentum_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);
    cpFloat momentum = (cpFloat)LUAX_NUMBER(L, 2);

    cpBody *body = self->body;
    if (self->type == BODY_TYPE_BOX) {
        momentum = cpMomentForBox(momentum, self->size.box.width, self->size.box.height);
    } else
    if (self->type == BODY_TYPE_CIRCLE) {
        momentum = cpMomentForCircle(momentum, self->size.circle.radius, 0.0, (cpVect){ .x = 0.0, .y = 0.0 });
    }
    cpBodySetMoment(body, momentum);

    return 0;
}

static int body_momentum_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, body_momentum_1o_1n)
        LUAX_OVERLOAD_ARITY(2, body_momentum_2on_0)
    LUAX_OVERLOAD_END
}

static int body_position_1o_2n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    const cpBody *body = self->body;
    const cpVect position = cpBodyGetPosition(body);

    lua_pushnumber(L, (lua_Number)position.x);
    lua_pushnumber(L, (lua_Number)position.y);

    return 2;
}

static int body_position_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);
    cpFloat x = (cpFloat)LUAX_NUMBER(L, 2);
    cpFloat y = (cpFloat)LUAX_NUMBER(L, 3);

    cpBody *body = self->body;
    cpBodySetPosition(body, (cpVect){ .x = x, .y = y });

    return 0;
}

static int body_position_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, body_position_1o_2n)
        LUAX_OVERLOAD_ARITY(3, body_position_3onn_0)
    LUAX_OVERLOAD_END
}

static int body_velocity_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    const cpBody *body = self->body;
    const cpVect velocity = cpBodyGetVelocity(body);

    lua_pushnumber(L, (lua_Number)velocity.x);
    lua_pushnumber(L, (lua_Number)velocity.y);

    return 2;
}

static int body_velocity_3onn_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);
    cpFloat x = (cpFloat)LUAX_NUMBER(L, 2);
    cpFloat y = (cpFloat)LUAX_NUMBER(L, 3);

    cpBody *body = self->body;
    cpBodySetVelocity(body, (cpVect){ .x = x, .y = y });

    return 0;
}

static int body_velocity_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, body_velocity_1o_2nn)
        LUAX_OVERLOAD_ARITY(3, body_velocity_3onn_0)
    LUAX_OVERLOAD_END
}

static int body_angle_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    const cpBody *body = self->body;
    const cpFloat angle = cpBodyGetAngle(body);

    lua_pushnumber(L, (lua_Number)angle);

    return 1;
}

static int body_angle_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);
    cpFloat angle = (cpFloat)LUAX_NUMBER(L, 2);

    cpBody *body = self->body;
    cpBodySetAngle(body, angle);

    return 0;
}

static int body_angle_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_ARITY(1, body_angle_1o_1n)
        LUAX_OVERLOAD_ARITY(2, body_angle_2on_0)
    LUAX_OVERLOAD_END
}
