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

#include "shape.h"

#include <chipmunk/chipmunk.h>

#include <libs/log.h>

#define LOG_CONTEXT "pl-shape"

PL_Shape_t *PL_shape_create_circle(PL_Body_t *body, PL_Float_t radius, PL_Vector_t offset)
{
    cpShape *shape = cpCircleShapeNew((cpBody *)body, (cpFloat)radius, (cpVect){ .x = offset.x, .y = offset.y });
    if (!shape) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create circle");
        return NULL;
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "circular shape %p created", shape);

    return (PL_Shape_t *)shape;
}

PL_Shape_t *PL_shape_create_box(PL_Body_t *body, PL_Float_t width, PL_Float_t height, PL_Float_t radius)
{
    cpShape *shape = cpBoxShapeNew((cpBody *)body, (cpFloat)width, (cpFloat)height, (cpFloat)radius);
    if (!shape) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create box");
        return NULL;
    }

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "boxed shape %p create", shape);

    return (PL_Shape_t *)shape;
}

void PL_shape_destroy(PL_Shape_t *shape)
{
//    cpSpaceRemoveShape(cpShapeGetSpace((cpShape *)shape), (cpShape *)shape);
    cpShapeFree((cpShape *)shape);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shape %p freed", shape);
}

void PL_shape_set_density(PL_Shape_t *shape, PL_Float_t density)
{
    cpShapeSetDensity((cpShape *)shape, (cpFloat)density);
}

void PL_shape_set_elasticity(PL_Shape_t *shape, PL_Float_t elasticity)
{
    cpShapeSetElasticity((cpShape *)shape, (cpFloat)elasticity);
}

void PL_shape_set_friction(PL_Shape_t *shape, PL_Float_t friction)
{
    cpShapeSetFriction((cpShape *)shape, (cpFloat)friction);
}

void PL_shape_set_surface_velocity(PL_Shape_t *shape, PL_Vector_t surfaceVelocity)
{
    cpShapeSetSurfaceVelocity((cpShape *)shape, (cpVect){ .x = surfaceVelocity.x, .y = surfaceVelocity.y });
}

PL_AABB_t PL_shape_get_aabb(const PL_Shape_t *shape)
{
    cpBB bb = cpShapeGetBB((const cpShape *)shape);
    return (PL_AABB_t){ .x0 = bb.l, .y0 = bb.t, .x1 = bb.r, .y1 = bb.b };
}
