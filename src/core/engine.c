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

#include "engine.h"

#include <config.h>
#include <platform.h>
#include <libs/log.h>
#include <libs/stb.h>
#include <version.h>

#if PLATFORM_ID == PLATFORM_WINDOWS
  #include <windows.h>
#endif

#define ENTRY_ICON "icon.png"
#define ENTRY_GAMECONTROLLER_DB "gamecontrollerdb.txt"

#define LOG_CONTEXT "engine"

static const unsigned char _default_icon_pixels[] = {
#include <assets/icon.inc>
};

static const uint8_t _default_mappings[] = {
#include <assets/gamecontrollerdb.inc>
    0x00
};

static inline void _wait_for(float seconds)
{
#if PLATFORM_ID == PLATFORM_LINUX
    long millis = (long)(seconds * 1000.0f); // Can use `floorf()`, too.
    if (millis == 0L) {
        sched_yield();
    } else {
        struct timespec ts = (struct timespec){
                .tv_sec = millis / 1000L,
                .tv_nsec = (millis % 1000L) * 1000000L
            };
        nanosleep(&ts, NULL);
    }
#elif PLATFORM_ID == PLATFORM_WINDOWS
    long millis = (long)(seconds * 1000.0f);
    if (millis == 0L) {
        YieldProcessor();
    } else {
        Sleep(millis);
    }
#else
    int micro = (int)(seconds * 1000000.0f);
    if (micro == 0) {
        sched_yield();
    } else {
        usleep(micro); // usleep takes sleep time in us (1 millionth of a second)
    }
#endif
}

static bool _configure(Storage_t *storage, Configuration_t *configuration)
{
    const Storage_Resource_t *resource = Storage_load(storage, "tofu.config", STORAGE_RESOURCE_STRING);
    if (!resource) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "configuration file is missing");
        return false;
    }

    Configuration_parse(configuration, S_SCHARS(resource));

    Log_configure(configuration->system.debug, NULL);

    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "game identity is `%s`", configuration->system.identity);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "running engine version %s", TOFU_VERSION_STRING);

    if (configuration->system.version.major > TOFU_VERSION_MAJOR
        || configuration->system.version.minor > TOFU_VERSION_MINOR
        || configuration->system.version.revision > TOFU_VERSION_REVISION) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "engine version mismatch (required %d.%d.%d, current %d.%d.%d)",
            configuration->system.version.major, configuration->system.version.minor, configuration->system.version.revision,
            TOFU_VERSION_MAJOR, TOFU_VERSION_MINOR, TOFU_VERSION_REVISION);
        return false;
    }

    return true;
}

Engine_t *Engine_create(const char *base_path)
{
    Engine_t *engine = malloc(sizeof(Engine_t));
    if (!engine) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate engine");
        return NULL;
    }

    *engine = (Engine_t){ 0 }; // Ensure is cleared at first.

    Log_initialize();

    const Storage_Configuration_t storage_configuration = {
            .base_path = base_path
        };
    engine->storage = Storage_create(&storage_configuration);
    if (!engine->storage) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize storage at path `%s`", base_path);
        free(engine);
        return NULL;
    }

    bool configured = _configure(engine->storage, &engine->configuration);
    if (!configured) {
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    const Storage_Resource_t *icon = Storage_exists(engine->storage, ENTRY_ICON)
        ? Storage_load(engine->storage, ENTRY_ICON, STORAGE_RESOURCE_IMAGE)
        : NULL;
    Log_assert(!icon, LOG_LEVELS_INFO, LOG_CONTEXT, "user-defined icon loaded");
    Display_Configuration_t display_configuration = { // TODO: use compound-literals.
            .icon = icon ? (GLFWimage){ .width = (int)S_IWIDTH(icon), .height = (int)S_IHEIGHT(icon), .pixels = S_IPIXELS(icon) } : (GLFWimage){ 64, 64, (unsigned char *)_default_icon_pixels },
            .window = {
                .title = engine->configuration.display.title,
                .width = engine->configuration.display.width,
                .height = engine->configuration.display.height,
                .scale = engine->configuration.display.scale
            },
            .fullscreen = engine->configuration.display.fullscreen,
            .vertical_sync = engine->configuration.display.vertical_sync,
            .hide_cursor = engine->configuration.cursor.hide
        };
    engine->display = Display_create(&display_configuration);
    if (!engine->display) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize display");
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    const Storage_Resource_t *mappings = Storage_exists(engine->storage, ENTRY_GAMECONTROLLER_DB)
        ? Storage_load(engine->storage, ENTRY_GAMECONTROLLER_DB, STORAGE_RESOURCE_STRING)
        : NULL;
    Log_assert(!mappings, LOG_LEVELS_INFO, LOG_CONTEXT, "user-defined controller mappings loaded");
    Input_Configuration_t input_configuration = {
            .mappings = mappings ? S_SCHARS(mappings) : (const char *)_default_mappings,
            .keyboard = {
                .enabled = engine->configuration.keyboard.enabled,
                .exit_key = engine->configuration.keyboard.exit_key,
            },
            .cursor = {
                .enabled = engine->configuration.cursor.enabled,
                .speed = engine->configuration.cursor.speed,
                .scale = 1.0f / Display_get_scale(engine->display) // FIXME: pass the sizes?
            },
            .gamepad = {
                .enabled = engine->configuration.gamepad.enabled,
                .sensitivity = engine->configuration.gamepad.sensitivity,
                .deadzone = engine->configuration.gamepad.inner_deadzone, // FIXME: pass inner/outer and let the input code do the math?
                .range = 1.0f - engine->configuration.gamepad.inner_deadzone - engine->configuration.gamepad.outer_deadzone,
                .emulate_dpad = engine->configuration.gamepad.emulate_dpad,
                .emulate_cursor = engine->configuration.gamepad.emulate_cursor
            }
        };
    engine->input = Input_create(&input_configuration, Display_get_window(engine->display));
    if (!engine->input) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize input");
        Display_destroy(engine->display);
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    engine->audio = Audio_create(&(Audio_Configuration_t){
            .master_volume = engine->configuration.audio.master_volume
        });
    if (!engine->audio) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize audio");
        Input_destroy(engine->input);
        Display_destroy(engine->display);
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    engine->environment = Environment_create();
    if (!engine->environment) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize environment");
        Audio_destroy(engine->audio);
        Input_destroy(engine->input);
        Display_destroy(engine->display);
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    // The interpreter is the first to be loaded, since it also manages the configuration. Later on, we will call to
    // initialization function once the sub-systems are ready.
    const void *userdatas[] = {
            engine->storage,
            engine->display,
            engine->input,
            engine->audio,
            engine->environment,
            NULL
        };
    engine->interpreter = Interpreter_create(engine->storage, userdatas);
    if (!engine->interpreter) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize interpreter");
        Environment_destroy(engine->environment);
        Audio_destroy(engine->audio);
        Input_destroy(engine->input);
        Display_destroy(engine->display);
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    return engine;
}

void Engine_destroy(Engine_t *engine)
{
    Interpreter_destroy(engine->interpreter); // Terminate the interpreter to unlock all resources.
    Environment_destroy(engine->environment);
    Audio_destroy(engine->audio);
    Input_destroy(engine->input);
    Display_destroy(engine->display);
    Storage_destroy(engine->storage);

    free(engine);

#if DEBUG
    stb_leakcheck_dumpmem();
#endif
}

void Engine_run(Engine_t *engine)
{
    const float delta_time = 1.0f / (float)engine->configuration.engine.frames_per_seconds;
    const size_t skippable_frames = engine->configuration.engine.skippable_frames;
    const float reference_time = engine->configuration.engine.frames_limit == 0 ? 0.0f : 1.0f / engine->configuration.engine.frames_limit;
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "now running, update-time is %.6fs w/ %d skippable frames, reference-time is %.6fs", delta_time, skippable_frames, reference_time);

    // Track time using double to keep the min resolution consistent over time!
    // https://randomascii.wordpress.com/2012/02/13/dont-store-that-in-a-float/
    double previous = glfwGetTime();
    float lag = 0.0f;

    // https://nkga.github.io/post/frame-pacing-analysis-of-the-game-loop/
    for (bool running = true; running && !Environment_should_quit(engine->environment) && !Display_should_close(engine->display); ) {
        const double current = glfwGetTime();
        const float elapsed = (float)(current - previous);
        previous = current;

        Environment_add_frame(engine->environment, elapsed);

        Input_process(engine->input);

        running = running && Interpreter_input(engine->interpreter); // Lazy evaluate `running`, will avoid calls when error.

        lag += elapsed; // Count a maximum amount of skippable frames in order no to stall on slower machines.
        for (size_t frames = skippable_frames; frames && (lag >= delta_time); --frames) {
            Environment_update(engine->environment, delta_time);
            running = running && Interpreter_update(engine->interpreter, delta_time); // Fixed update.
            running = running && Audio_update(engine->audio, elapsed); // Update the subsystems w/ fixed steps (fake interrupt based).
            running = running && Storage_update(engine->storage, elapsed);
            lag -= delta_time;
        }

//        running = running && Interpreter_update_variable(engine->interpreter, elapsed); // Variable update.
//        running = running && Audio_update_variable(&engine->audio, elapsed);
//        running = running && Storage_update_variable(engine->storage, elapsed);
        Input_update(engine->input, elapsed);
        Display_update(engine->display, elapsed);

        running = running && Interpreter_render(engine->interpreter, lag / delta_time);

        Display_present(engine->display);

        if (reference_time != 0.0f) {
            const float frame_time = (float)(glfwGetTime() - current);
            const float leftover = reference_time - frame_time;
            if (leftover > 0.0f) {
                _wait_for(leftover); // FIXME: Add minor compensation to reach cap value?
            }
        }
    }
}
