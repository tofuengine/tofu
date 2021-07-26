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

#ifndef __PL_SHAPE_H__
#define __PL_SHAPE_H__

#include "common.h"
#include "body.h"

#include <chipmunk/chipmunk.h>

typedef struct _PL_Shape_t {
    cpShape *shape;
} PL_Shape_t;

extern PL_Shape_t *PL_shape_create_circle(PL_Body_t *body, PL_Float_t radius, PL_Vector_t offset);
extern PL_Shape_t *PL_shape_create_box(PL_Body_t *body, PL_Float_t width, PL_Float_t height, PL_Float_t radius);
extern void PL_shape_destroy(PL_Shape_t *shape);

extern void PL_shape_set_density(PL_Shape_t *shape, PL_Float_t density);
extern void PL_shape_set_elasticity(PL_Shape_t *shape, PL_Float_t elasticity);
extern void PL_shape_set_friction(PL_Shape_t *shape, PL_Float_t friction);
extern void PL_shape_set_surface_velocity(PL_Shape_t *shape, PL_Vector_t surfaceVelocity);

extern PL_AABB_t PL_shape_get_aabb(const PL_Shape_t *shape);

#endif  /* __PL_SHAPE_H__ */
