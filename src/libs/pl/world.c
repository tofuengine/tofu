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

#include <chipmunk/chipmunk.h>

#include <libs/log.h>

#define LOG_CONTEXT "pl-world"

PL_World_t *PL_world_create(void)
{
    cpSpace *space = cpSpaceNew();
    if (!space) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create space");
        return NULL;
    }

    cpSpaceSetIterations(space, 30);

    return (PL_World_t *)space;
}

void PL_world_destroy(PL_World_t *world)
{
    cpSpaceFree((cpSpace *)world);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "space %p freed", world);
}

PL_Vector_t PL_world_get_gravity(const PL_World_t *world)
{
    cpVect gravity = cpSpaceGetGravity((const cpSpace *)world);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p gravity is <%f.5, %f.5>", world, gravity.x, gravity.y);

    return (PL_Vector_t){ .x = gravity.x, .y = gravity.y };
}

void PL_world_set_gravity(PL_World_t *world, PL_Vector_t gravity)
{
    cpSpaceSetGravity((cpSpace *)world, (cpVect){ .x = gravity.x, .y = gravity.y });
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p gravity set to <%f.5, %f.5>", world, gravity.x, gravity.y);
}

PL_Float_t PL_world_get_damping(const PL_World_t *world)
{
    float damping = cpSpaceGetDamping((const cpSpace *)world);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p damping is %f.5", world, damping);

    return (PL_Float_t)damping;
}

void PL_world_set_damping(PL_World_t *world, PL_Float_t damping)
{
    cpSpaceSetDamping((cpSpace *)world, (cpFloat)damping);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p damping set to %f.5", world, damping);
}

void PL_world_add_body(PL_World_t *world, const PL_Body_t *body)
{
    cpSpaceAddBody((cpSpace *)world, (cpBody *)body);
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "body %p added to world %p", body, world);
}

void PL_world_add_shape(PL_World_t *world, const PL_Shape_t *shape)
{
    cpSpaceAddShape((cpSpace *)world, (cpShape *)shape);
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "shape %p added to world %p", shape, world);
}

//void _remove_shape(cpBody *body, cpShape *shape, void *data)
//{
//    cpSpaceRemoveShape((cpSpace *)data, (cpShape *)shape);
//}

void PL_world_remove_body(PL_World_t *world, const PL_Body_t *body)
{
//    cpBodyEachShape(body, _remove_shape, world);
    cpSpaceRemoveBody((cpSpace *)world, (cpBody *)body);
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "body %p removed from world %p", body, world);
}

void PL_world_remove_shape(PL_World_t *world, const PL_Shape_t *shape)
{
    cpSpaceRemoveShape((cpSpace *)world, (cpShape *)shape);
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "shape %p removed from world %p", shape, world);
}

void PL_world_update(PL_World_t *world, PL_Float_t delta_time)
{
    cpSpaceStep((cpSpace *)world, (cpFloat)delta_time);
}
