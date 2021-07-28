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

#ifndef __PL_WORLD_H__
#define __PL_WORLD_H__

#include "common.h"
#include "body.h"
#include "shape.h"

typedef void *PL_World_t;

extern PL_World_t *PL_world_create(void);
extern void PL_world_destroy(PL_World_t *world);

extern PL_Vector_t PL_world_get_gravity(const PL_World_t *world);
extern void PL_world_set_gravity(PL_World_t *world, PL_Vector_t gravity);
extern PL_Float_t PL_world_get_damping(const PL_World_t *world);
extern void PL_world_set_damping(PL_World_t *world, PL_Float_t damping);

extern void PL_world_add_body(PL_World_t *world, const PL_Body_t *body);
extern void PL_world_remove_body(PL_World_t *world, const PL_Body_t *body);

extern void PL_world_add_shape(PL_World_t *world, const PL_Shape_t *shape);
extern void PL_world_remove_shape(PL_World_t *world, const PL_Shape_t *shape);

extern void PL_world_update(PL_World_t *world, PL_Float_t delta_time);

#endif  /* __PL_WORLD_H__ */
