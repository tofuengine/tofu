/*
 * MIT License
 * 
 * Copyright (c) 2019-2020 Marco Lizza
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

#include "environment.h"

#include <config.h>
#include <libs/log.h>
#include <libs/stb.h>

#define LOG_CONTEXT "environment"

// TODO: http://www.ilikebigbits.com/2017_06_01_float_or_double.html

static bool _starts_with(const char *string, const char *prefix, const char **ptr)
{
    size_t length = strlen(prefix);
    if (strncmp(string, prefix, length) == 0) {
        *ptr = string + length;
        return true;
    }
    return false;
}

Environment_t *Environment_create(int argc, const char *argv[])
{
    Environment_t *environment = malloc(sizeof(Environment_t));
    if (!environment) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate environment");
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "environment allocated");

    const char *base_path = NULL;
    const char **args = NULL;
    for (int i = 1; i < argc; ++i) { // Skip executable name, i.e. argument #0.
        if (_starts_with(argv[i], "--base-path=", &base_path)) { // Skip base mount point, too.
            continue;
        }
        arrpush(args, argv[i]);
    }

    *environment = (Environment_t){
        .base_path = base_path,
        .args = args,
        .quit = false,
        .fps = 0.0f,
        .time = 0.0
    };

    return environment;
}

void Environment_destroy(Environment_t *environment)
{
    arrfree(environment->args);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "arguments freed");

    free(environment);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "environment freed");
}

void Environment_quit(Environment_t *environment)
{
    environment->quit = true;
}

bool Environment_should_quit(const Environment_t *environment)
{
    return environment->quit;
}

double Environment_get_time(const Environment_t *environment)
{
    return environment->time;
}

float Environment_get_fps(const Environment_t *environment)
{
    return environment->fps;
}

static inline float _calculate_fps(float frame_time)
{
    static float samples[FPS_AVERAGE_SAMPLES] = { 0 };
    static size_t index = 0;
    static float sum = 0.0f; // We are storing just a small time interval, float is enough...

    sum -= samples[index];
    samples[index] = frame_time;
    sum += frame_time;
    index = (index + 1) % FPS_AVERAGE_SAMPLES;

    return (float)FPS_AVERAGE_SAMPLES / sum;
}

void Environment_add_frame(Environment_t *environment, float frame_time)
{
    environment->fps = _calculate_fps(frame_time);
#ifdef __DEBUG_ENGINE_FPS__
    static size_t count = 0;
    if (++count == 250) {
        Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "currently running at %.0f FPS", environment->fps);
        count = 0;
    }
#endif
}

void Environment_update(Environment_t *environment, float frame_time)
{
    environment->time += frame_time;
}
