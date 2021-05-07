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

#include "configuration.h"

#include <libs/imath.h>
#include <version.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static inline void _parse_version(const char *version_string, int *major, int *minor, int *revision)
{
    *major = *minor = *revision = 0; // Set to zero, minimun enforced value in case some parts are missing.
    sscanf(version_string, "%d.%d.%d", major, minor, revision);
}

static void _on_parameter(Configuration_t *configuration, const char *context, const char *key, const char *value)
{
    char fqn[256] = { 0 };
    if (context && context[0] != '\0') { // Skip context if not provided.
        strcpy(fqn, context);
        strcat(fqn, "-");
    }
    strcat(fqn, key);

    if (strcmp(fqn, "system-identity") == 0) {
        strcpy(configuration->system.identity, value);
    } else
    if (strcmp(fqn, "system-version") == 0) {
        int major, minor, revision;
        _parse_version(value, &major, &minor, &revision);
        configuration->system.version.major = major;
        configuration->system.version.minor = minor;
        configuration->system.version.revision = revision;
    } else
    if (strcmp(fqn, "system-debug") == 0) {
        configuration->system.debug = strcmp(value, "true") == 0;
    } else
    if (strcmp(fqn, "system-icon") == 0) {
        strcpy(configuration->system.icon, value);
    } else
    if (strcmp(fqn, "system-mappings") == 0) {
        strcpy(configuration->system.mappings, value);
    } else
    if (strcmp(fqn, "display-title") == 0) {
        strcpy(configuration->display.title, value);
    } else
    if (strcmp(fqn, "display-width") == 0) {
        configuration->display.width = (size_t)strtoul(value, NULL, 0);
    } else
    if (strcmp(fqn, "display-height") == 0) {
        configuration->display.height = (size_t)strtoul(value, NULL, 0);
    } else
    if (strcmp(fqn, "display-scale") == 0) {
        configuration->display.scale = (size_t)strtoul(value, NULL, 0);
    } else
    if (strcmp(fqn, "display-fullscreen") == 0) {
        configuration->display.fullscreen = strcmp(value, "true") == 0;
    } else
    if (strcmp(fqn, "display-vertical-sync") == 0) {
        configuration->display.vertical_sync = strcmp(value, "true") == 0;
    } else
    if (strcmp(fqn, "display-effect") == 0) {
        strcpy(configuration->display.effect, value);
    } else
    if (strcmp(fqn, "audio-device-index") == 0) {
        configuration->audio.device_index = (int)strtol(value, NULL, 0);
    } else
    if (strcmp(fqn, "audio-master-volume") == 0) {
        configuration->audio.master_volume = (float)strtod(value, NULL);
    } else
    if (strcmp(fqn, "keyboard-enabled") == 0) {
        configuration->keyboard.enabled = strcmp(value, "true") == 0;
    } else
    if (strcmp(fqn, "keyboard-exit-key") == 0) {
        configuration->keyboard.exit_key = strcmp(value, "true") == 0;
    } else
    if (strcmp(fqn, "cursor-enabled") == 0) {
        configuration->cursor.enabled = strcmp(value, "true") == 0;
    } else
    if (strcmp(fqn, "cursor-hide") == 0) {
        configuration->cursor.hide = strcmp(value, "true") == 0;
    } else
    if (strcmp(fqn, "cursor-speed") == 0) {
        configuration->cursor.speed = (float)strtod(value, NULL);
    } else
    if (strcmp(fqn, "gamepad-enabled") == 0) {
        configuration->gamepad.enabled = strcmp(value, "true") == 0;
    } else
    if (strcmp(fqn, "gamepad-sensitivity") == 0) {
        configuration->gamepad.sensitivity = (float)strtod(value, NULL);
    } else
    if (strcmp(fqn, "gamepad-inner-deadzone") == 0) {
        configuration->gamepad.inner_deadzone = (float)strtod(value, NULL);
    } else
    if (strcmp(fqn, "gamepad-outer-deadzone") == 0) {
        configuration->gamepad.outer_deadzone = (float)strtod(value, NULL);
    } else
    if (strcmp(fqn, "gamepad-emulate-dpad") == 0) {
        configuration->gamepad.emulate_dpad = strcmp(value, "true") == 0;
    } else
    if (strcmp(fqn, "gamepad-emulate-cursor") == 0) {
        configuration->gamepad.emulate_cursor = strcmp(value, "true") == 0;
    } else
    if (strcmp(fqn, "engine-frames-per-seconds") == 0) {
        configuration->engine.frames_per_seconds = (size_t)strtoul(value, NULL, 0);
        configuration->engine.skippable_frames = configuration->engine.frames_per_seconds / 5; // Keep synched. About 20% of the frequency (FPS).
    } else
    if (strcmp(fqn, "engine-skippable-frames") == 0) {
        size_t suggested = configuration->engine.frames_per_seconds / 5;
        configuration->engine.skippable_frames = (size_t)imin((int)strtol(value, NULL, 0), (int)suggested); // TODO: not sure if `imin` or `imax`. :P
    } else
    if (strcmp(fqn, "engine-frames-limit") == 0) {
        configuration->engine.frames_limit = (size_t)strtoul(value, NULL, 0);
    }
}

static const char *_next(const char *ptr, char *line)
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

static bool _parse_context(char *line, char *context)
{
    size_t length = strlen(line);
    if (line[0] != '[' || line[length - 1] != ']') { // Contexts are declared with square brackets.
        return false;
    }

    context[0] = '\0'; // Avoid `strncpy()` to handle null-terminator with more sense.
    strncat(context, line + 1, length - 2);

    return true;
}

static bool _parse_pair(char *line, const char **key, const char **value)
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

static void _normalize_identity(Configuration_t *configuration)
{
    if (configuration->system.identity[0] == '\0') {
        size_t length = strlen(configuration->display.title);
        for (size_t i = 0, j = 0; i < length; ++i) {
            int c = configuration->display.title[i];
            if (!isalnum(c)) {
                continue;
            }
            configuration->system.identity[j++] = c;
        }
    }

    size_t length = strlen(configuration->system.identity);
    for (size_t i = 0; i < length; ++i) {
        int c = configuration->system.identity[i];
        configuration->system.identity[i] = tolower(c); // Game identity is lowercase.
    }
}

void Configuration_parse(Configuration_t *configuration, const char *data)
{
    *configuration = (Configuration_t){
            .system = {
                .identity = { 0 },
                .version = { TOFU_VERSION_MAJOR, TOFU_VERSION_MINOR, TOFU_VERSION_REVISION },
                .debug = true,
                .icon = "icon.png",
                .mappings = "gamecontrollerdb.txt"
            },
            .display = {
                .title = ".: Tofu Engine :.",
                .width = 320,
                .height = 240,
                .scale = 0,
                .fullscreen = false,
                .vertical_sync = false,
                .effect = "effect.glsl"
            },
            .audio = {
                .device_index = -1, // Pick the default device.
                .master_volume = 1.0f
            },
            .keyboard = {
                .enabled = true,
                .exit_key = true
            },
            .cursor = {
                .enabled = true,
                .hide = true,
                .speed = 128.0f
            },
            .gamepad = {
                .enabled = true,
                .sensitivity = 0.5f,
                .inner_deadzone = 0.25f,
                .outer_deadzone = 0.0f,
                .emulate_dpad = true,
                .emulate_cursor = true
            },
            .engine = {
                .frames_per_seconds = 60,
                .skippable_frames = 3, // About 20% of the FPS amount.
#ifdef __CAP_TO_60__
                .frames_limit = 60, // 60 FPS capping as a default. TODO: make it run-time configurable?
#else
                .frames_limit = 0,
#endif
            }
        };
    if (!data) {
        return;
    }

    char context[128] = { 0 };
    char line[256];
    for (const char *ptr = data; ptr;) {
        ptr = _next(ptr, line);
        if (_parse_context(line, context)) {
            continue;
        }
        const char *key, *value;
        if (!_parse_pair(line, &key, &value)) {
            continue;
        }
        _on_parameter(configuration, context, key, value);
    }

    _normalize_identity(configuration);
}

void Configuration_override(Configuration_t *configuration, int argc, const char *argv[])
{
    char pair[256];
    for (int i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "--", 2) != 0) { // Long-options only.
            continue;
        }
        strcpy(pair, argv[i] + 2); // Skip "--" marker.
        const char *key, *value;
        if (!_parse_pair(pair, &key, &value)) {
            continue;
        }
        _on_parameter(configuration, NULL, key, value); // Context is already fused in the parameter key.
    }
}
