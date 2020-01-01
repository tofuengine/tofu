/*
 * Copyright (c) 2019-2020 by Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "configuration.h"

#include <libs/imath.h>

#include <stdlib.h>
#include <string.h>

static void on_parameter(Configuration_t *configuration, const char *key, const char *value)
{
    if (strcmp(key, "title") == 0) {
        strncpy(configuration->title, value, MAX_CONFIGURATION_TITLE_LENGTH);
    } else
    if (strcmp(key, "icon") == 0) {
        strncpy(configuration->icon, value, MAX_CONFIGURATION_ICON_LENGTH);
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
        configuration->skippable_frames = (size_t)imin(strtol(value, NULL, 0), suggested); // TODO: not sure if `imin` or `imax`. :P
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
    if (strcmp(key, "use-keyboard") == 0) {
        configuration->use_keyboard = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "use-gamepad") == 0) {
        configuration->use_gamepad = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "use-mouse") == 0) {
        configuration->use_mouse = strcmp(value, "true") == 0;
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

void Configuration_load(Configuration_t *configuration, const char *data)
{
    *configuration = (Configuration_t){
            .title = ".: Tofu Engine :.",
            .icon = { 0 },
            .width = 320,
            .height = 240,
            .scale = 0,
            .fullscreen = false,
            .vertical_sync = false,
            .fps = 60,
            .skippable_frames = 3, // About 20% of the FPS amount.
            .fps_cap = -1, // No capping as a default. TODO: make it run-time configurable?
            .hide_cursor = true,
            .exit_key_enabled = true,
            .use_keyboard = true,
            .use_gamepad = true,
            .use_mouse = true,
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
