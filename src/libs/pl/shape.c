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

#include <libs/log.h>

#define LOG_CONTEXT "pl-shape"

PL_Shape_t *PL_shape_create_circle(PL_Body_t *body, PL_Float_t radius, PL_Vector_t offset)
{
    cpShape *circle = cpCircleShapeNew(body->body, (cpFloat)radius, (cpVect){ .x = offset.x, .y = offset.y });
    if (!circle) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create circle");
        return NULL;
    }

    PL_Shape_t *shape = malloc(sizeof(PL_Shape_t));
    if (!shape) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate shape");
        cpSpaceFree(circle);
        return NULL;
    }

    *shape = (PL_Shape_t){
            .shape = circle
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shape %p created w/ circle %p", shape, circle);

    return shape;
}

PL_Shape_t *PL_shape_create_box(PL_Body_t *body, PL_Float_t width, PL_Float_t height, PL_Float_t radius)
{
    cpShape *box = cpBoxShapeNew(body->body, (cpFloat)width, (cpFloat)height, (cpFloat)radius);
    if (!box) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create box");
        return NULL;
    }

    PL_Shape_t *shape = malloc(sizeof(PL_Shape_t));
    if (!shape) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate shape");
        cpSpaceFree(box);
        return NULL;
    }

    *shape = (PL_Shape_t){
            .shape = box
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shape %p create w/ box %p", shape, box);

    return shape;
}

void PL_shape_destroy(PL_Shape_t *shape)
{
    cpShapeFree(shape->shape);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shape circle/box %p freed", shape->shape);

    free(shape);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "shape %p freed", shape);
}

void PL_shape_set_density(PL_Shape_t *shape, PL_Float_t density)
{
    cpShapeSetDensity(shape->shape, (cpFloat)density);
}

void PL_shape_set_elasticity(PL_Shape_t *shape, PL_Float_t elasticity)
{
    cpShapeSetElasticity(shape->shape, (cpFloat)elasticity);
}

void PL_shape_set_friction(PL_Shape_t *shape, PL_Float_t friction)
{
    cpShapeSetFriction(shape->shape, (cpFloat)friction);
}

void PL_shape_set_surface_velocity(PL_Shape_t *shape, PL_Vector_t surfaceVelocity)
{
    cpShapeSetSurfaceVelocity(shape->shape, (cpVect){ .x = surfaceVelocity.x, .y = surfaceVelocity.y });
}

PL_AABB_t PL_shape_get_aabb(const PL_Shape_t *shape)
{
    cpBB bb = cpShapeGetBB(shape->shape);
    return (PL_AABB_t){ .x0 = bb.l, .y0 = bb.t, .x1 = bb.r, .y1 = bb.b };
}
