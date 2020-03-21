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
#include <core/configuration.h>
#include <core/platform.h>
#include <libs/fs/fsaux.h>
#include <libs/log.h>
#include <libs/stb.h>

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#if PLATFORM_ID == PLATFORM_LINUX
  #include <unistd.h>
#elif PLATFORM_ID == PLATFORM_WINDOWS
  #include <windows.h>
#endif

#define ENTRY_ICON "icon.png"
#define ENTRY_GAMECONTROLLER_DB "gamecontrollerdb.txt"

#define _TOFU_CONCAT_VERSION(m, n, r) #m "." #n "." #r "-dev"
#define _TOFU_MAKE_VERSION(m, n, r) _TOFU_CONCAT_VERSION(m, n, r)
#define TOFU_VERSION_NUMBER _TOFU_MAKE_VERSION(TOFU_VERSION_MAJOR, TOFU_VERSION_MINOR, TOFU_VERSION_REVISION)

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

static inline float _calculate_fps(float elapsed)
{
    static float samples[FPS_AVERAGE_SAMPLES] = { 0 };
    static size_t index = 0;
    static float sum = 0.0f; // We are storing just a small time interval, float is enough...

    sum -= samples[index];
    samples[index] = elapsed;
    sum += elapsed;
    index = (index + 1) % FPS_AVERAGE_SAMPLES;

    return (float)FPS_AVERAGE_SAMPLES / sum;
}

static bool _configure(const File_System_t *file_system, Configuration_t *configuration)
{
    File_System_Resource_t *content = FSaux_load(file_system, "tofu.config", FILE_SYSTEM_RESOURCE_STRING);
    if (!content) {
        return false;
    }
    Configuration_parse(configuration, FSAUX_SCHARS(content));
    FSaux_release(content);
    return true;
}

static File_System_Resource_t *_load_icon(const File_System_t *file_system, const char *file)
{
    if (!file || file[0] == '\0') {
        return NULL;
    }

    if (!FSaux_exists(file_system, file)) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "file `%s` doesn't exist", file);
        return NULL;
    }

    return FSaux_load(file_system, file, FILE_SYSTEM_RESOURCE_IMAGE);
}

static File_System_Resource_t *_load_mappings(const File_System_t *file_system, const char *file)
{
    if (!file || file[0] == '\0') {
        return NULL;
    }

    if (!FSaux_exists(file_system, file)) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "file `%s` doesn't exist", file);
        return NULL;
    }

    return FSaux_load(file_system, file, FILE_SYSTEM_RESOURCE_STRING);
}

static void _free_resource(File_System_Resource_t *resource)
{
    if (!resource) {
        return;
    }
    FSaux_release(resource); // FIXME: should this be (and other similar deallocators) be safe by design (like `free()`)?
}

bool Engine_initialize(Engine_t *engine, const char *base_path)
{
    *engine = (Engine_t){ 0 }; // Ensure is cleared at first.

    Log_initialize();
    bool result = FS_initialize(&engine->file_system, base_path);
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize I/O at path `%s`", base_path);
        return false;
    }

    result = _configure(&engine->file_system, &engine->configuration);
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "configuration file is missing");
        return false;
    }

    Log_configure(engine->configuration.debug, NULL);
    Environment_initialize(&engine->environment);

    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "version %s", TOFU_VERSION_NUMBER);

    File_System_Resource_t *icon = _load_icon(&engine->file_system, ENTRY_ICON);
    Log_assert(!icon, LOG_LEVELS_INFO, LOG_CONTEXT, "user-defined icon loaded");
    Display_Configuration_t display_configuration = { // TODO: reorganize configuration.
            .icon = icon ? (GLFWimage){ .width = FSAUX_IWIDTH(icon), .height = FSAUX_IHEIGHT(icon), .pixels = FSAUX_IPIXELS(icon) } : (GLFWimage){ 64, 64, (unsigned char *)_default_icon_pixels },
            .title = engine->configuration.title,
            .width = engine->configuration.width,
            .height = engine->configuration.height,
            .fullscreen = engine->configuration.fullscreen,
            .vertical_sync = engine->configuration.vertical_sync,
            .scale = engine->configuration.scale,
            .hide_cursor = engine->configuration.hide_cursor
        };
    result = Display_initialize(&engine->display, &display_configuration);
    _free_resource(icon);
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize display");
        FS_terminate(&engine->file_system);
        return false;
    }

    File_System_Resource_t *mappings = _load_mappings(&engine->file_system, ENTRY_GAMECONTROLLER_DB);
    Log_assert(!mappings, LOG_LEVELS_INFO, LOG_CONTEXT, "user-defined controller mappings loaded");
    Input_Configuration_t input_configuration = {
            .mappings = mappings ? FSAUX_SCHARS(mappings) : (const char *)_default_mappings,
            .exit_key_enabled = engine->configuration.exit_key_enabled,
#ifdef __INPUT_SELECTION__
            .keyboard_enabled = engine->configuration.keyboard_enabled,
            .gamepad_enabled = engine->configuration.gamepad_enabled,
            .mouse_enabled = engine->configuration.mouse_enabled,
#endif
            .emulate_dpad = engine->configuration.emulate_dpad,
            .emulate_mouse = engine->configuration.emulate_mouse,
            .cursor_speed = engine->configuration.cursor_speed,
            .gamepad_sensitivity = engine->configuration.gamepad_sensitivity,
            .gamepad_deadzone = engine->configuration.gamepad_inner_deadzone,
            .gamepad_range = 1.0f - engine->configuration.gamepad_inner_deadzone - engine->configuration.gamepad_outer_deadzone,
            .scale = 1.0f / (float)engine->display.configuration.scale
        };
    result = Input_initialize(&engine->input, &input_configuration, engine->display.window);
    _free_resource(mappings);
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize input");
        Display_terminate(&engine->display);
        FS_terminate(&engine->file_system);
        return false;
    }

    result = Audio_initialize(&engine->audio, &(Audio_Configuration_t){ .channels = 2, .sample_rate = 44100, .voices = 8 });
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize audio");
        Input_terminate(&engine->input);
        Display_terminate(&engine->display);
        FS_terminate(&engine->file_system);
        return false;
    }

    // The interpreter is the first to be loaded, since it also manages the configuration. Later on, we will call to
    // initialization function once the sub-systems are ready.
    const void *userdatas[] = {
            &engine->interpreter,
            &engine->file_system,
            &engine->environment,
            &engine->display,
            &engine->input,
            NULL
        };
    result = Interpreter_initialize(&engine->interpreter, &engine->file_system, userdatas);
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize interpreter");
        Audio_terminate(&engine->audio);
        Input_terminate(&engine->input);
        Display_terminate(&engine->display);
        FS_terminate(&engine->file_system);
        return false;
    }

    return true;
}

void Engine_terminate(Engine_t *engine)
{
    Interpreter_terminate(&engine->interpreter); // Terminate the interpreter to unlock all resources.
    Audio_terminate(&engine->audio);
    Display_terminate(&engine->display);
    Input_terminate(&engine->input);

    Environment_terminate(&engine->environment);

    FS_terminate(&engine->file_system);
#if DEBUG
    stb_leakcheck_dumpmem();
#endif
}

void Engine_run(Engine_t *engine)
{
    const float delta_time = 1.0f / (float)engine->configuration.fps;
    const size_t skippable_frames = engine->configuration.skippable_frames;
    const float reference_time = engine->configuration.fps_cap == 0 ? 0.0f : 1.0f / engine->configuration.fps_cap;
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "now running, update-time is %.6fs w/ %d skippable frames, reference-time is %.6fs", delta_time, skippable_frames, reference_time);

    // Track time using double to keep the min resolution consistent over time!
    // https://randomascii.wordpress.com/2012/02/13/dont-store-that-in-a-float/
    double previous = glfwGetTime();
    float lag = 0.0f;

    // https://nkga.github.io/post/frame-pacing-analysis-of-the-game-loop/
    for (bool running = true; running && !engine->environment.quit && !Display_should_close(&engine->display); ) {
        const double current = glfwGetTime();
        const float elapsed = (float)(current - previous);
        previous = current;

        engine->environment.fps = _calculate_fps(elapsed);
#ifdef __DEBUG_ENGINE_FPS__
        static size_t count = 0;
        if (++count == 250) {
            Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "currently running at %.0f FPS", engine->environment.fps);
            count = 0;
        }
#endif

        Input_process(&engine->input);

        running = running && Interpreter_process(&engine->interpreter); // Lazy evaluate `running`, will avoid calls when error.

        lag += elapsed; // Count a maximum amount of skippable frames in order no to stall on slower machines.
        for (size_t frames = skippable_frames; frames && (lag >= delta_time); --frames) {
            engine->environment.time += delta_time;
            running = running && Interpreter_update(&engine->interpreter, delta_time); // Fixed update.
            lag -= delta_time;
        }

//        running = running && Interpreter_update_variable(&engine->interpreter, elapsed); // Variable update.
        Audio_update(&engine->audio, elapsed); // Update the subsystems w/ regard to the variable time.
        Input_update(&engine->input, elapsed);
        Display_update(&engine->display, elapsed);

        running = running && Interpreter_render(&engine->interpreter, lag / delta_time);

        Display_present(&engine->display);

        if (reference_time != 0.0f) {
            const float frame_time = (float)(glfwGetTime() - current);
            const float leftover = reference_time - frame_time;
            if (leftover > 0.0f) {
                _wait_for(leftover); // FIXME: Add minor compensation to reach cap value?
            }
        }
    }
}
