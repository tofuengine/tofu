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

#include "body.h"

#include "internal/udt.h"

#include <core/config.h>
#define _LOG_TAG "body"
#include <libs/log.h>

#include <chipmunk/chipmunk.h>

static int body_new_4ennn_1o(lua_State *L);
static int body_gc_1o_0(lua_State *L);
static int body_center_of_gravity_v_v(lua_State *L);
static int body_type_v_v(lua_State *L);
static int body_mass_v_v(lua_State *L);
static int body_momentum_v_v(lua_State *L);
static int body_position_v_v(lua_State *L);
static int body_velocity_v_v(lua_State *L);
static int body_angle_v_v(lua_State *L);
static int body_elasticity_v_v(lua_State *L);
static int body_density_v_v(lua_State *L);
static int body_sleep_v_v(lua_State *L);
static int body_shape_1o_4snnn(lua_State *L);

int body_loader(lua_State *L)
{
    int nup = luaX_pushupvalues(L);
    return luaX_newmodule(L,
        (luaX_Script){ 0 },
        (const struct luaL_Reg[]){
            // -- constructors/destructors --
            { "new", body_new_4ennn_1o },
            { "__gc", body_gc_1o_0 },
            // -- getters/setters --
            { "center_of_gravity", body_center_of_gravity_v_v },
            { "type", body_type_v_v },
            { "mass", body_mass_v_v },
            { "momentum", body_momentum_v_v },
            { "position", body_position_v_v },
            { "velocity", body_velocity_v_v },
            { "angle", body_angle_v_v },
            { "elasticity", body_elasticity_v_v },
            { "density", body_density_v_v },
            { "sleep", body_sleep_v_v },
            // -- accessors --
            { "shape", body_shape_1o_4snnn },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        }, nup, LUAX_STRING(L, lua_upvalueindex(USERDATA_MODULE_NAME)));
}

static const char *_kinds[Body_Kinds_t_CountOf + 1] = {
    "box",
    "circle",
    NULL
};

static int body_new_4ennn_1o(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Body_Kinds_t kind = (Body_Kinds_t)LUAX_ENUM(L, 1, _kinds);

    cpBody *body = cpBodyNew(0.0, 0.0);
    if (!body) {
        return luaL_error(L, "can't create body");
    }
//    LOG_D("body %p created for world %p", body, physics->world);

    Body_Object_t *self = (Body_Object_t *)luaX_newobject(L, sizeof(Body_Object_t), &(Body_Object_t){
            .body = body,
            .shape = NULL,
            .kind = kind,
            .size = { { 0 } }
        }, OBJECT_TYPE_BODY, LUAX_STRING(L, lua_upvalueindex(USERDATA_MODULE_NAME)));

    if (self->kind == BODY_KIND_BOX) {
        self->size.box.width = (cpFloat)LUAX_NUMBER(L, 2);
        self->size.box.height = (cpFloat)LUAX_NUMBER(L, 3);
        self->size.box.radius = (cpFloat)LUAX_NUMBER(L, 4);
        self->shape = cpBoxShapeNew(body, self->size.box.width, self->size.box.height, self->size.box.radius);
    } else
    if (self->kind == BODY_KIND_CIRCLE) {
        self->size.circle.radius = (cpFloat)LUAX_NUMBER(L, 2);
        self->size.circle.offset.x = (cpFloat)LUAX_NUMBER(L, 3);
        self->size.circle.offset.y = (cpFloat)LUAX_NUMBER(L, 4);
        self->shape = cpCircleShapeNew(body, self->size.circle.radius, self->size.circle.offset);
    }

    LOG_D("body %p created", self);

    return 1;
}

static int body_gc_1o_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    cpShapeFree(self->shape);
    LOG_D("shape %p destroyed", self->shape);

    cpBodyFree(self->body);
    LOG_D("body %p destroyed", self->body);

    LOG_D("body %p finalized", self);

    return 0;
}

static int body_center_of_gravity_1o_2n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (const Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    const cpBody *body = self->body;
    const cpVect cog = cpBodyGetCenterOfGravity(body);

    lua_pushnumber(L, (lua_Number)cog.x);
    lua_pushnumber(L, (lua_Number)cog.y);

    return 2;
}

static int body_center_of_gravity_3onn_0(lua_State *L)
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
    cpBodySetCenterOfGravity(body, (cpVect){ .x = x, .y = y });

    return 0;
}

static int body_center_of_gravity_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(body_center_of_gravity_1o_2n, 1)
        LUAX_OVERLOAD_BY_ARITY(body_center_of_gravity_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static const char *_types[] = {
    "dynamic",
    "kinematic",
    "static",
    NULL
};

static int body_type_1o_1s(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (const Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    const cpBody *body = self->body;
    const cpBodyType type = cpBodyGetType(body);

    lua_pushstring(L, _types[type]); // FIXME: this is an abuse!

    return 1;
}

static int body_type_2oe_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TENUM)
    LUAX_SIGNATURE_END
    Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);
    cpBodyType type = (cpBodyType)LUAX_ENUM(L, 2, _types);

    cpBody *body = self->body;
    cpBodySetType(body, (cpBodyType)type);

    return 0;
}

static int body_type_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(body_type_1o_1s, 1)
        LUAX_OVERLOAD_BY_ARITY(body_type_2oe_0, 2)
    LUAX_OVERLOAD_END
}

static int body_mass_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (const Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

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
        LUAX_OVERLOAD_BY_ARITY(body_mass_1o_1n, 1)
        LUAX_OVERLOAD_BY_ARITY(body_mass_2on_0, 2)
    LUAX_OVERLOAD_END
}

static int body_momentum_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (const Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    const cpBody *body = self->body;
    const cpFloat momentum = cpBodyGetMoment(body);

//    self->momentum = momentum;
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
//    self->momentum = momentum;
    if (self->kind == BODY_KIND_BOX) {
        momentum = cpMomentForBox(momentum, self->size.box.width, self->size.box.height);
    } else
    if (self->kind == BODY_KIND_CIRCLE) {
        momentum = cpMomentForCircle(momentum, self->size.circle.radius, 0.0, self->size.circle.offset);
    }
    cpBodySetMoment(body, momentum);

    return 0;
}

static int body_momentum_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(body_momentum_1o_1n, 1)
        LUAX_OVERLOAD_BY_ARITY(body_momentum_2on_0, 2)
    LUAX_OVERLOAD_END
}

static int body_position_1o_2n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (const Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

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
    cpShape *shape = self->shape; // Or `cpSpaceReindexShapesForBody`.
    cpSpace *space = cpShapeGetSpace(shape);
    if (space) {
        cpSpaceReindexShape(space, shape); // Reindex when moving (mostly for static bodies)
    }

    return 0;
}

static int body_position_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(body_position_1o_2n, 1)
        LUAX_OVERLOAD_BY_ARITY(body_position_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static int body_velocity_1o_2nn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (const Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

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
        LUAX_OVERLOAD_BY_ARITY(body_velocity_1o_2nn, 1)
        LUAX_OVERLOAD_BY_ARITY(body_velocity_3onn_0, 3)
    LUAX_OVERLOAD_END
}

static int body_angle_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (const Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

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
    cpShape *shape = self->shape; // Or `cpSpaceReindexShapesForBody`.
    cpSpace *space = cpShapeGetSpace(shape);
    if (space) {
        cpSpaceReindexShape(space, shape); // Reindex when moving (mostly for static bodies)
    }

    return 0;
}

static int body_angle_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(body_angle_1o_1n, 1)
        LUAX_OVERLOAD_BY_ARITY(body_angle_2on_0, 2)
    LUAX_OVERLOAD_END
}

static int body_elasticity_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (const Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    const cpShape *shape = self->shape;
    const cpFloat elasticity = cpShapeGetElasticity(shape);

    lua_pushnumber(L, (lua_Number)elasticity);

    return 1;
}

static int body_elasticity_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);
    cpFloat elasticity = (cpFloat)LUAX_NUMBER(L, 2);

    cpShape *shape = self->shape;
    cpShapeSetElasticity(shape, elasticity);

    return 0;
}

static int body_elasticity_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(body_elasticity_1o_1n, 1)
        LUAX_OVERLOAD_BY_ARITY(body_elasticity_2on_0, 2)
    LUAX_OVERLOAD_END
}

static int body_density_1o_1n(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (const Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    const cpShape *shape = self->shape;
    const cpFloat density = cpShapeGetDensity(shape);

    lua_pushnumber(L, (lua_Number)density);

    return 1;
}

static int body_density_2on_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TNUMBER)
    LUAX_SIGNATURE_END
    Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);
    cpFloat density = (cpFloat)LUAX_NUMBER(L, 2);

    cpShape *shape = self->shape;
    cpShapeSetDensity(shape, density);

    return 0;
}

static int body_density_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(body_density_1o_1n, 1)
        LUAX_OVERLOAD_BY_ARITY(body_density_2on_0, 2)
    LUAX_OVERLOAD_END
}

static int body_sleep_1o_1b(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (const Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    const cpBody *body = self->body;
    const bool is_sleeping = cpBodyIsSleeping(body) == cpTrue;

    lua_pushboolean(L, is_sleeping);

    return 1;
}

static int body_sleep_2ob_0(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
        LUAX_SIGNATURE_REQUIRED(LUA_TBOOLEAN)
    LUAX_SIGNATURE_END
    Body_Object_t *self = (Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);
    bool is_sleeping = LUAX_BOOLEAN(L, 2);

    cpBody *body = self->body;
    if (is_sleeping) {
        cpBodySleep(body);
    } else {
        cpBodyActivate(body);
//        cpBodyActivateStatic(body, NULL);
    }

    return 0;
}

static int body_sleep_v_v(lua_State *L)
{
    LUAX_OVERLOAD_BEGIN(L)
        LUAX_OVERLOAD_BY_ARITY(body_sleep_1o_1b, 1)
        LUAX_OVERLOAD_BY_ARITY(body_sleep_2ob_0, 2)
    LUAX_OVERLOAD_END
}

static int body_shape_1o_4snnn(lua_State *L)
{
    LUAX_SIGNATURE_BEGIN(L)
        LUAX_SIGNATURE_REQUIRED(LUA_TOBJECT)
    LUAX_SIGNATURE_END
    const Body_Object_t *self = (const Body_Object_t *)LUAX_OBJECT(L, 1, OBJECT_TYPE_BODY);

    if (self->kind == BODY_KIND_BOX) {
        lua_pushstring(L, "box");
        lua_pushnumber(L, (lua_Number)self->size.box.width);
        lua_pushnumber(L, (lua_Number)self->size.box.height);
        lua_pushnumber(L, (lua_Number)self->size.box.radius);
    } else
    if (self->kind == BODY_KIND_CIRCLE) {
        lua_pushstring(L, "circle");
        lua_pushnumber(L, (lua_Number)self->size.circle.radius);
        lua_pushnumber(L, (lua_Number)self->size.circle.offset.x);
        lua_pushnumber(L, (lua_Number)self->size.circle.offset.y);
    }

    return 4;
}
