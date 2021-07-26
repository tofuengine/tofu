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

#include <libs/log.h>

#define LOG_CONTEXT "pl-body"

PL_Body_t *PL_body_create(void)
{
    cpBody *core = cpBodyNew(0.0, 0.0);
    if (!core) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create core body");
        return NULL;
    }

    PL_Body_t *body = malloc(sizeof(PL_Body_t));
    if (!body) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate body");
        cpSpaceBody(core);
        return NULL;
    }

    *body = (PL_Body_t){
            .body = core
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p created w/ core %p", body, core);

    return body;
}

void PL_body_destroy(PL_Body_t *body)
{
    cpSpaceBody(body->body);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body core %p freed", body->body);

    free(body);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "body %p freed", body);
}

void PL_body_set_type(PL_Body_t *body, PL_Body_Types_t type)
{
    switch (type) {
        case PL_BODY_TYPE_DYNAMIC: { cpBodySetType(body->body, CP_BODY_TYPE_DYNAMIC); } break;
        case PL_BODY_TYPE_KINEMATIC: { cpBodySetType(body->body, CP_BODY_TYPE_KINEMATIC); } break;
        case PL_BODY_TYPE_STATIC: { cpBodySetType(body->body, CP_BODY_TYPE_STATIC); } break;
    }
}

void PL_body_set_enabled(PL_Body_t *body, bool enable)
{
    if (enable) {
        cpBodyActivate(body->body);
    } else {
        cpBodySleep(body->body);
    }
}

bool PL_body_is_enabled(const PL_Body_t *body)
{
    return cpBodyIsSleeping(body->body);
}

PL_Vector_t PL_body_get_position(const PL_Body_t *body)
{
    cpVect position = cpBodyGetPosition(body->body);
    return (PL_Vector_t){ .x = position.x, .y = position.y };
}

void PL_body_set_position(PL_Body_t *body, PL_Vector_t position)
{
    cpBodySetPosition(body->body, (cpVect){ .x = position.x, .y = position.y });
    cpSpaceReindexShapesForBody(cpBodyGetSpace(body->body), body->body); // TODO: is this required?
}

void PL_body_set_centre_of_gravity(PL_Body_t *body, PL_Vector_t centre_of_gravity)
{
    cpBodySetCenterOfGravity(body->body, (cpVect){ .x = centre_of_gravity.x, .y = centre_of_gravity.y });
}

PL_Float_t PL_body_get_angle(const PL_Body_t *body)
{
    return (PL_Float_t)cpBodyGetAngle(body->body);
}

void PL_body_set_angle(PL_Body_t *body, PL_Float_t angle)
{
    cpBodySetAngle(body->body, (cpFloat)angle);
}

void PL_body_set_momentum(PL_Body_t *body, PL_Float_t momentum)
{
    cpBodySetMoment(body->body, (cpFloat)momentum);
}

void PL_body_set_velocity(PL_Body_t *body, PL_Vector_t velocity)
{
    cpBodySetVelocity(body->body, (cpVect){ .x = velocity.x, .y = velocity.y });
}

void PL_body_set_force(PL_Body_t *body, PL_Vector_t force)
{
    cpBodySetForce(body->body, (cpVect){ .x = force.x, .y = force.y });
}

void PL_body_set_angular_velocity(PL_Body_t *body, PL_Float_t angular_velocity)
{
    cpBodySetAngularVelocity(body->body, (cpFloat)angular_velocity);
}

void PL_body_set_torque(PL_Body_t *body, PL_Float_t torque)
{
    cpBodySetTorque(body->body, (cpFloat)torque);
}
