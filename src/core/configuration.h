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

#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_CONFIGURATION_IDENTITY_LENGTH   128
#define MAX_CONFIGURATION_TITLE_LENGTH      128

typedef struct _Configuration {
    struct {
        char identity[MAX_CONFIGURATION_IDENTITY_LENGTH];
        struct {
            int major, minor, revision;
        } version;
        bool debug;
    } system;
    struct {
        char title[MAX_CONFIGURATION_TITLE_LENGTH];
        size_t width, height, scale;
        bool fullscreen;
        bool vertical_sync;
    } display;
    struct {
        int device_index;
        float master_volume;
    } audio;
    struct {
        bool enabled;
        bool exit_key;
    } keyboard;
    struct {
        bool enabled;
        bool hide;
        float speed;
    } cursor;
    struct {
        bool enabled;
        float sensitivity;
        float inner_deadzone;
        float outer_deadzone;
        bool emulate_dpad;
        bool emulate_cursor;
    } gamepad;
    struct {
        size_t frames_per_seconds;
        size_t skippable_frames;
        size_t frames_limit;
    } engine;
} Configuration_t;

extern void Configuration_parse(Configuration_t *configuration, const char *data); // TODO: allocate this, too?

#endif  /* __CONFIGURATION_H__ */
