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

#ifndef __CORE_SYSTEMS_PHYSICS_H__
#define __CORE_SYSTEMS_PHYSICS_H__

#include <chipmunk/chipmunk.h>

#include <stdbool.h>

typedef struct Physics_Configuration_s {
    const char *path;
} Physics_Configuration_t;

typedef struct Physics_s {
    Physics_Configuration_t configuration;

    cpSpace *space;
} Physics_t;

extern Physics_t *Physics_create(const Physics_Configuration_t *configuration);
extern void Physics_destroy(Physics_t *physics);

extern bool Physics_update(Physics_t *physics, float delta_time);

#endif  /* __CORE_SYSTEMS_PHYSICS_H__ */
