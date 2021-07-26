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

#include <libs/log.h>

#define LOG_CONTEXT "pl-world"

PL_World_t *PL_world_create(void)
{
    cpSpace *space = cpSpaceNew();
    if (!space) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create space");
        return NULL;
    }

    PL_World_t *world = malloc(sizeof(PL_World_t));
    if (!world) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate world");
        cpSpaceFree(space);
        return NULL;
    }

    *world = (PL_World_t){
            .space = space
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p created w/ space %p", world, space);

    return world;
}

PL_world_destroy(PL_World_t *world)
{
    cpSpaceFree(world->space);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world space %p freed", world->space);

    free(world);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p freed", world);
}

void PL_world_set_gravity(PL_World_t *world, PL_Vector_t force)
{
    cpSpaceSetGravity(world->space, (cpVect){ .x = force.x, .y = force.y });
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p gravity set to <%f.5, %f.5>", world, force.x, force.y);
}

void PL_world_set_damping(PL_World_t *world, PL_Float_t damping)
{
    cpSpaceSetDamping(world->space, (cpFloat)damping);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "world %p damping set to %f.5", world, damping);
}

void PL_world_add_body(PL_World_t *world, const PL_Body_t *body)
{
    cpSpaceAddBody(world->space, body->body);
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "body %p added to world %p", body, world);
}

void PL_world_remove_body(PL_World_t *world, const PL_Body_t *body)
{
    cpSpaceRemoveBody(world->space, body->body);
    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "body %p removed from world %p", body, world);
}

void PL_world_update(PL_World_t *world, PL_Float_t delta_time)
{
    cpSpaceStep(world->space, (cpFloat)delta_time);
}
