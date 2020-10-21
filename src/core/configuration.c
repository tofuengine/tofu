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

#include "configuration.h"

#include <libs/imath.h>

#include <stdlib.h>
#include <string.h>

static void on_parameter(Configuration_t *configuration, const char *key, const char *value)
{
    if (strcmp(key, "title") == 0) {
        strcpy(configuration->title, value);
    } else
    if (strcmp(key, "width") == 0) {
        configuration->width = (size_t)strtoul(value, NULL, 0);
    } else
    if (strcmp(key, "height") == 0) {
        configuration->height = (size_t)strtoul(value, NULL, 0);
    } else
    if (strcmp(key, "scale") == 0) {
        configuration->scale = (size_t)strtoul(value, NULL, 0);
    } else
    if (strcmp(key, "fullscreen") == 0) {
        configuration->fullscreen = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "vertical-sync") == 0) {
        configuration->vertical_sync = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "fps") == 0) {
        configuration->fps = (size_t)strtoul(value, NULL, 0);
        configuration->skippable_frames = configuration->fps / 5; // Keep synched. About 20% of the FPS amount.
    } else
    if (strcmp(key, "skippable-frames") == 0) {
        size_t suggested = configuration->fps / 5;
        configuration->skippable_frames = (size_t)imin((int)strtol(value, NULL, 0), (int)suggested); // TODO: not sure if `imin` or `imax`. :P
    } else
    if (strcmp(key, "fps-cap") == 0) {
        configuration->fps_cap = (size_t)strtoul(value, NULL, 0);
    } else
    if (strcmp(key, "hide-cursor") == 0) {
        configuration->hide_cursor = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "exit-key-enabled") == 0) {
        configuration->exit_key_enabled = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "keyboard-enabled") == 0) {
        configuration->keyboard_enabled = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "gamepad-enabled") == 0) {
        configuration->gamepad_enabled = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "mouse-enabled") == 0) {
        configuration->mouse_enabled = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "emulate-dpad") == 0) {
        configuration->emulate_dpad = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "emulate-mouse") == 0) {
        configuration->emulate_mouse = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "cursor-speed") == 0) {
        configuration->cursor_speed = (float)strtod(value, NULL);
    } else
    if (strcmp(key, "gamepad-sensitivity") == 0) {
        configuration->gamepad_sensitivity = (float)strtod(value, NULL);
    } else
    if (strcmp(key, "gamepad-inner-deadzone") == 0) {
        configuration->gamepad_inner_deadzone = (float)strtod(value, NULL);
    } else
    if (strcmp(key, "gamepad-outer-deadzone") == 0) {
        configuration->gamepad_outer_deadzone = (float)strtod(value, NULL);
    } else
    if (strcmp(key, "debug") == 0) {
        configuration->debug = strcmp(value, "true") == 0;
    }
}

static const char *next(const char *ptr, char *line)
{
    bool comment = false;
    for (;;) {
        char c = *(ptr++);
        if (c == '\0') {
            ptr = NULL;
            break;
        }
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            if (!comment) {
                break;
            }
            comment = false;
            continue;
        }
        if (comment) {
            continue;
        }
        if (c == '#') {
            comment = true;
            continue;
        }
        *(line++) = c;
    }
    *line = '\0';
    return ptr;
}

static bool parse(char *line, const char **key, const char **value)
{
    *key = line;

    while (*line != '=') {
        if (*line == '\0') {
            return false;
        }
        ++line;
    }

    *(line++) = '\0';

    *value = line;

    return true;
}

void Configuration_parse(Configuration_t *configuration, const char *data)
{
    *configuration = (Configuration_t){
            .title = ".: Tofu Engine :.",
            .width = 320,
            .height = 240,
            .scale = 0,
            .fullscreen = false,
            .vertical_sync = false,
            .fps = 60,
            .skippable_frames = 3, // About 20% of the FPS amount.
#ifdef __CAP_TO_60__
            .fps_cap = 60, // 60 FPS capping as a default. TODO: make it run-time configurable?
#else
            .fps_cap = 0,
#endif
            .hide_cursor = true,
            .exit_key_enabled = true,
            .keyboard_enabled = true,
            .gamepad_enabled = true,
            .mouse_enabled = true,
            .emulate_dpad = true,
            .emulate_mouse = true,
            .cursor_speed = 128.0f,
            .gamepad_sensitivity = 0.5f,
            .gamepad_inner_deadzone = 0.25f,
            .gamepad_outer_deadzone = 0.0f,
            .debug = true
        };

    if (!data) {
        return;
    }

    char line[256];
    for (const char *ptr = data; ptr;) {
        ptr = next(ptr, line);
        const char *key, *value;
        if (!parse(line, &key, &value)) {
            break;
        }
        on_parameter(configuration, key, value);
    }
}
