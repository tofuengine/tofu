/*
 * MIT License
 *
 * Copyright (c) 2019-2026 Marco Lizza
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

#include "init.h"

#include <core/config.h>
#define _LOG_TAG "soy:init"
#include <libs/log.h>
#include <libs/stb.h>

#if TOFU_CORE_BACKEND == TOFU_BACKEND_GLFW
    #include <GLFW/glfw3.h>
#endif

#if TOFU_CORE_BACKEND == TOFU_BACKEND_GLFW
static void _error_callback(int error, const char *description)
{
    LOG_E("[GLFW error %#d] %s", error, description);
}

static void *_allocate(size_t size, void *user)
{
    return malloc(size);
}

static void _deallocate(void* block, void *user)
{
    free(block);
}

static void *_reallocate(void* block, size_t size, void *user)
{
    return realloc(block, size);
}
#endif

bool soy_init(void)
{
#if TOFU_CORE_BACKEND == TOFU_BACKEND_GLFW
    glfwSetErrorCallback(_error_callback);

    bool initialized = glfwInit();
    if (!initialized) {
        LOG_F("can't initialize GLFW");
        goto error_exit;
    }
    LOG_D("GLFW initialized");

    glfwInitAllocator(&(GLFWallocator){
            .allocate = _allocate,
            .deallocate = _deallocate,
            .reallocate = _reallocate,
            .user = NULL
        });
    LOG_D("GLFW allocator set");

    return true;

error_exit:
    return false;
#else
    // Do nothing.
    return false;
#endif
}

void soy_deinit(void)
{
#if TOFU_CORE_BACKEND == TOFU_BACKEND_GLFW
    glfwTerminate();
#else
    // Do nothing.
#endif
}
