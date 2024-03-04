/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "stopwatch.h"

#include <GLFW/glfw3.h>

// Track time using `double` to keep the min resolution consistent over time!
// For intervals (i.e. deltas), `float` is sufficient.
//
// See: https://randomascii.wordpress.com/2012/02/13/dont-store-that-in-a-float/
StopWatch_t stopwatch_init(void)
{
    return (StopWatch_t){
            .marker = glfwGetTime()
        };
}

StopWatch_t stopwatch_clone(const StopWatch_t *stopwatch)
{
    return (StopWatch_t){
            .marker = stopwatch->marker
        };
}

void stopwatch_reset(StopWatch_t *stopwatch)
{
    stopwatch->marker = glfwGetTime();
}

void stopwatch_delta(StopWatch_t *stopwatch, double delta)
{
    stopwatch->marker += delta;
}

float stopwatch_partial(StopWatch_t *stopwatch)
{
    const double now = glfwGetTime();
    const float delta = (float)(now - stopwatch->marker);
    stopwatch->marker = now;
    return delta;
}

float stopwatch_elapsed(const StopWatch_t *stopwatch)
{
    const double now = glfwGetTime();
    return (float)(now - stopwatch->marker);
}
