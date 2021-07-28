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

#ifndef __PL_BODY_H__
#define __PL_BODY_H__

#include "common.h"

#include <stdbool.h>

typedef enum _PL_Body_Types_t {
    PL_BODY_TYPE_DYNAMIC,
    PL_BODY_TYPE_KINEMATIC,
    PL_BODY_TYPE_STATIC,
} PL_Body_Types_t;

typedef void *PL_Body_t;

extern PL_Body_t *PL_body_create(void);
extern void PL_body_destroy(PL_Body_t *body);
extern void PL_body_set_type(PL_Body_t *body, PL_Body_Types_t type);
extern void PL_body_set_enabled(PL_Body_t *body, bool enable);
extern bool PL_body_is_enabled(const PL_Body_t *body);

extern PL_Float_t PL_body_get_mass(const PL_Body_t *body);
extern void PL_body_set_mass(PL_Body_t *body, PL_Float_t mass);

extern PL_Vector_t PL_body_get_position(const PL_Body_t *body);
extern void PL_body_set_position(PL_Body_t *body, PL_Vector_t position);

extern void PL_body_set_centre_of_gravity(PL_Body_t *body, PL_Vector_t centre_of_gravity);
extern PL_Float_t PL_body_get_angle(const PL_Body_t *body);
extern void PL_body_set_angle(PL_Body_t *body, PL_Float_t angle);
extern PL_Float_t PL_body_get_momentum(const PL_Body_t *body);
extern void PL_body_set_momentum(PL_Body_t *body, PL_Float_t momentum);
extern void PL_body_set_momentum_for_box(PL_Body_t *body, PL_Float_t momentum, PL_Float_t width, PL_Float_t height);
extern void PL_body_set_momentum_for_circle(PL_Body_t *body, PL_Float_t momentum, PL_Float_t radius);

extern PL_Vector_t PL_body_get_velocity(const PL_Body_t *body);
extern void PL_body_set_velocity(PL_Body_t *body, PL_Vector_t velocity);
extern void PL_body_set_force(PL_Body_t *body, PL_Vector_t force);

extern void PL_body_set_angular_velocity(PL_Body_t *body, PL_Float_t angular_velocity);
extern void PL_body_set_torque(PL_Body_t *body, PL_Float_t torque);

#endif  /* __PL_BODY_H__ */
