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

#include "physics.h"

#include <libs/log.h>

#include <malloc.h>

#define LOG_CONTEXT "physics"

Physics_t *Physics_create(const Physics_Configuration_t *configuration)
{
    Physics_t *physics = malloc(sizeof(Physics_t));
    if (!physics) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate physics");
        return NULL;
    }

    *physics = (Physics_t){
            .configuration = *configuration
        };

    physics->space = cpSpaceNew();
    if (!physics->space) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't create space");
        free(physics);
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "physics %p w/ space %p created", physics, physics->space);

    return physics;
}

void Physics_destroy(Physics_t *physics)
{
    cpSpaceFree(physics->space);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "physics space %p destroyed", physics->space);

    free(physics);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "physics freed");
}

bool Physics_update(Physics_t *physics, float delta_time)
{
    cpSpaceStep(physics->space, delta_time);
    return true;
}
