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

#include "configuration.h"

#include <core/resolution.h>
#include <core/version.h>
#include <libs/imath.h>
#include <libs/log.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LOG_CONTEXT "configuration"

#define LINE_LENGTH                 256
#define PARAMETER_LENGTH            128
#define PARAMETER_CONTEXT_LENGTH    64
#define PARAMETER_KEY_LENGTH        (PARAMETER_LENGTH - PARAMETER_CONTEXT_LENGTH)

static inline void _parse_version(const char *version_string, int *major, int *minor, int *revision)
{
    *major = *minor = *revision = 0; // Set to zero, minimum enforced value in case some parts are missing.
    sscanf(version_string, "%d.%d.%d", major, minor, revision);
}

static void _on_parameter(Configuration_t *configuration, const char *context, const char *key, const char *value)
{
    char fqn[PARAMETER_LENGTH] = { 0 };
    if (context && context[0] != '\0') { // Skip context if not provided.
        strncpy(fqn, context, PARAMETER_CONTEXT_LENGTH);
        strcat(fqn, "-");
    }
    strncat(fqn, key, PARAMETER_KEY_LENGTH);

    if (strcmp(fqn, "system-identity") == 0) {
        strncpy(configuration->system.identity, value, MAX_VALUE_LENGTH - 1);
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
        strncpy(configuration->system.icon, value, MAX_VALUE_LENGTH - 1);
    } else
    if (strcmp(fqn, "system-mappings") == 0) {
        strncpy(configuration->system.mappings, value, MAX_VALUE_LENGTH - 1);
    } else
    if (strcmp(fqn, "system-quit-on-close") == 0) {
        configuration->system.quit_on_close = strcmp(value, "true") == 0;
    } else
    if (strcmp(fqn, "display-title") == 0) {
        strncpy(configuration->display.title, value, MAX_VALUE_LENGTH - 1);
    } else
    if (strcmp(fqn, "display-resolution") == 0) {
        const Resolution_t *resolution = Resolution_find(value);
        if (resolution) {
            configuration->display.width = resolution->width;
            configuration->display.height = resolution->height;
        } else {
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "unknown resolution variant `%s`", value);
        }
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
        strncpy(configuration->display.effect, value, MAX_VALUE_LENGTH - 1);
    } else
    if (strcmp(fqn, "audio-device-index") == 0) {
        configuration->audio.device_index = (int)strtol(value, NULL, 0);
    } else
    if (strcmp(fqn, "audio-master-volume") == 0) {
        configuration->audio.master_volume = (float)strtod(value, NULL);
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
    if (strcmp(fqn, "controller-inner-deadzone") == 0) {
        configuration->controller.inner_deadzone = (float)strtod(value, NULL);
    } else
    if (strcmp(fqn, "controller-outer-deadzone") == 0) {
        configuration->controller.outer_deadzone = (float)strtod(value, NULL);
    } else
    if (strcmp(fqn, "engine-frames-per-seconds") == 0) {
        configuration->engine.frames_per_seconds = (size_t)strtoul(value, NULL, 0);
        configuration->engine.skippable_frames = configuration->engine.frames_per_seconds / 20; // Keep synched. About 5% of the frequency (FPS).
    } else
    if (strcmp(fqn, "engine-skippable-frames") == 0) {
        size_t suggested = configuration->engine.frames_per_seconds / 20;
        configuration->engine.skippable_frames = (size_t)imin((int)strtol(value, NULL, 0), (int)suggested); // TODO: not sure if `imin` or `imax`. :P
    } else
    if (strcmp(fqn, "engine-frames-limit") == 0) {
        configuration->engine.frames_limit = (size_t)strtoul(value, NULL, 0);
    }
}

static const char *_next(const char *ptr, char *line, size_t n)
{
    bool comment = false;
    const char *end_of_data = line + n - 1; // Leave room for the null-terminator.
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
        if (line >= end_of_data) {
            continue; // Don't halt the loop as we need to consume a whole line in the source buffer.
        }
        *(line++) = c;
    }
    *line = '\0';
    return ptr;
}

static bool _parse_context(char *line, char *context, size_t n)
{
    size_t length = strlen(line);
    if (line[0] != '[' || line[length - 1] != ']') { // Contexts are declared with square brackets.
        return false;
    }

    const char *end_of_data = context + n - 1; // Leave room for the null-terminator.
    for (const char *ptr = line + 1; *ptr != ']'; ++ptr) {
        if (context >= end_of_data) {
            break;
        }
        *(context++) = *ptr;
    }
    *context = '\0';

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
            char c = configuration->display.title[i];
            if (!isalnum(c)) {
                continue;
            }
            configuration->system.identity[j++] = c;
        }
    }

    size_t length = strlen(configuration->system.identity);
    for (size_t i = 0; i < length; ++i) {
        char c = configuration->system.identity[i];
        configuration->system.identity[i] = (char)tolower(c); // Game identity is lowercase.
    }
}

Configuration_t *Configuration_create(const char *data)
{
    Configuration_t *configuration = malloc(sizeof(Configuration_t));
    if (!configuration) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate configuration");
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "configuration allocated");

    *configuration = (Configuration_t){
            .system = {
                .identity = { 0 },
                .version = { TOFU_VERSION_MAJOR, TOFU_VERSION_MINOR, TOFU_VERSION_REVISION },
                .debug = true,
                .icon = "assets/png/icon.png",
                .mappings = "assets/txt/gamecontrollerdb.txt",
                .quit_on_close = true
            },
            .display = {
                .title = ".: Tofu Engine :.",
                .width = 320,
                .height = 240,
                .scale = 0,
                .fullscreen = false,
                .vertical_sync = false,
                .effect = "assets/glsl/passthru.glsl"
            },
            .audio = {
                .device_index = -1, // Pick the default device.
                .master_volume = 1.0f
            },
            .keyboard = {
                .exit_key = true
            },
            .cursor = {
                .enabled = true,
                .hide = true,
                .speed = 128.0f
            },
            .controller = {
                .inner_deadzone = 0.25f,
                .outer_deadzone = 0.0f
            },
            .engine = {
                .frames_per_seconds = 60,
                .skippable_frames = 3, // About 5% of the FPS amount.
#ifdef __CAP_TO_60__
                .frames_limit = 60, // 60 FPS capping as a default. TODO: make it run-time configurable?
#else
                .frames_limit = 0,
#endif
            }
        };
    if (!data) {
        return configuration;
    }

    char context[PARAMETER_CONTEXT_LENGTH] = { 0 };
    for (const char *ptr = data; ptr;) {
        char line[LINE_LENGTH] = { 0 };
        ptr = _next(ptr, line, LINE_LENGTH);
        if (_parse_context(line, context, PARAMETER_CONTEXT_LENGTH)) {
            continue;
        }
        const char *key, *value;
        if (!_parse_pair(line, &key, &value)) {
            continue;
        }
        _on_parameter(configuration, context, key, value);
    }

    _normalize_identity(configuration);

    return configuration;
}

void Configuration_destroy(Configuration_t *configuration)
{
    free(configuration);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "configuration freed");
}

void Configuration_override(Configuration_t *configuration, int argc, const char *argv[])
{
    for (int i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "--", 2) != 0) { // Long-options only.
            continue;
        }
        char pair[LINE_LENGTH] = { 0 };
        strncpy(pair, argv[i] + 2, LINE_LENGTH - 1); // Skip "--" marker.
        const char *key, *value;
        if (!_parse_pair(pair, &key, &value)) {
            continue;
        }
        _on_parameter(configuration, NULL, key, value); // Context is already fused in the parameter key.
    }
}
