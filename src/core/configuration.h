/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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

#include <core/platform.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_VALUE_LENGTH   128

typedef struct Configuration_s {
    struct {
        char identity[MAX_VALUE_LENGTH];
        struct {
            int major, minor, revision;
        } version;
        bool debug;
        char icon[MAX_VALUE_LENGTH];
        char mappings[MAX_VALUE_LENGTH];
        bool quit_on_close;
    } system;
    struct {
        char title[MAX_VALUE_LENGTH];
        size_t width, height, scale;
        bool fullscreen;
        bool vertical_sync;
        char effect[MAX_VALUE_LENGTH];
    } display;
    struct {
        int device_index;
        float master_volume;
    } audio;
    struct {
        bool exit_key; // TODO: enum type with disabled/notify/autoclose?
    } keyboard;
    struct {
        bool enabled;
        bool hide;
        float speed;
    } cursor;
    struct {
        float inner_deadzone; // TODO: separate into distinct stick and trigger deadzone.
        float outer_deadzone;
    } controller;
    struct {
        size_t frames_per_seconds;
        size_t skippable_frames;
        size_t frames_limit;
    } engine;
} Configuration_t;

extern Configuration_t *Configuration_create(const char *data);
extern void Configuration_destroy(Configuration_t *configuration);

extern void Configuration_override(Configuration_t *configuration, int argc, const char *argv[]);

#endif  /* __CONFIGURATION_H__ */
