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

#include "engine.h"

#include <config.h>
#include <platform.h>
#include <libs/log.h>
#include <libs/stb.h>
#include <libs/sysinfo.h>
#include <version.h>

#if PLATFORM_ID == PLATFORM_WINDOWS
  #include <windows.h>
#endif

#include "utils/options.h"

#define EVENTS_INITIAL_CAPACITY 8

#define LOG_CONTEXT "engine"

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

static Configuration_t *_configure(Storage_t *storage, int argc, const char *argv[])
{
    const Storage_Resource_t *resource = Storage_load(storage, "tofu.config", STORAGE_RESOURCE_STRING);
    if (!resource) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "configuration file is missing");
        return false;
    }

    Configuration_t *configuration = Configuration_create(S_SCHARS(resource));
    Configuration_override(configuration, argc, argv);

    Log_configure(configuration->system.debug, NULL);

    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "game identity is `%s`", configuration->system.identity);

    if (configuration->system.version.major > TOFU_VERSION_MAJOR
        || configuration->system.version.minor > TOFU_VERSION_MINOR
        || configuration->system.version.revision > TOFU_VERSION_REVISION) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "engine version mismatch (required %d.%d.%d, current %d.%d.%d)",
            configuration->system.version.major, configuration->system.version.minor, configuration->system.version.revision,
            TOFU_VERSION_MAJOR, TOFU_VERSION_MINOR, TOFU_VERSION_REVISION);
        Configuration_destroy(configuration);
        return NULL;
    }

    return configuration;
}

static inline void _information(void)
{
    System_Information_t si = { 0 };
    bool result = SI_inspect(&si);
    if (!result) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get system information");
        return;
    }

    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "Tofu Engine v%s (%s build)", TOFU_VERSION_STRING, PLATFORM_NAME);
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "running on %s %s (%s, %s)", si.system, si.architecture, si.release, si.version);
}

Engine_t *Engine_create(int argc, const char *argv[])
{
    options_t options = options_parse_command_line(argc, argv); // We do this early, since options could have effect on everything.

    Engine_t *engine = malloc(sizeof(Engine_t));
    if (!engine) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate engine");
        return NULL;
    }

    *engine = (Engine_t){ 0 }; // Ensure is cleared at first.

    Log_initialize();

    _information();

    engine->storage = Storage_create(&(const Storage_Configuration_t){
            .path = options.path
        });
    if (!engine->storage) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize storage");
        free(engine);
        return NULL;
    }

    engine->configuration = _configure(engine->storage, argc, argv);
    if (!engine->configuration) {
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    bool set = Storage_set_identity(engine->storage, engine->configuration->system.identity);
    if (!set) {
        Configuration_destroy(engine->configuration);
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    const Storage_Resource_t *icon = Storage_load(engine->storage, engine->configuration->system.icon, STORAGE_RESOURCE_IMAGE);
    Log_assert(!icon, LOG_LEVELS_INFO, LOG_CONTEXT, "user-defined icon loaded");
    const Storage_Resource_t *effect = Storage_load(engine->storage, engine->configuration->display.effect, STORAGE_RESOURCE_STRING);
    Log_assert(!effect, LOG_LEVELS_INFO, LOG_CONTEXT, "user-defined effect loaded");
    engine->display = Display_create(&(const Display_Configuration_t){
            .icon = icon ? (GLFWimage){ .width = (int)S_IWIDTH(icon), .height = (int)S_IHEIGHT(icon), .pixels = S_IPIXELS(icon) } : (GLFWimage){ 0 },
            .window = {
                .title = engine->configuration->display.title,
                .width = engine->configuration->display.width,
                .height = engine->configuration->display.height,
                .scale = engine->configuration->display.scale
            },
            .fullscreen = engine->configuration->display.fullscreen,
            .vertical_sync = engine->configuration->display.vertical_sync,
            .quit_on_close = engine->configuration->system.quit_on_close,
            .palette = engine->configuration->display.palette,
            .effect = effect ? S_SCHARS(effect) : NULL
        });
    if (!engine->display) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize display");
        Configuration_destroy(engine->configuration);
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    const Storage_Resource_t *mappings = Storage_load(engine->storage, engine->configuration->system.mappings, STORAGE_RESOURCE_STRING);
    Log_assert(!mappings, LOG_LEVELS_INFO, LOG_CONTEXT, "user-defined controller mappings loaded");
    engine->input = Input_create(&(const Input_Configuration_t){
            .mappings = mappings ? S_SCHARS(mappings) : NULL,
            .keyboard = {
                .enabled = engine->configuration->keyboard.enabled,
                .exit_key = engine->configuration->keyboard.exit_key,
            },
            .cursor = {
                .enabled = engine->configuration->cursor.enabled,
                .hide = engine->configuration->cursor.hide,
                .speed = engine->configuration->cursor.speed,
                .scale = 1.0f / Display_get_scale(engine->display) // FIXME: pass the sizes?
            },
            .gamepad = {
                .enabled = engine->configuration->gamepad.enabled,
                .sensitivity = engine->configuration->gamepad.sensitivity,
                .deadzone = engine->configuration->gamepad.inner_deadzone, // FIXME: pass inner/outer and let the input code do the math?
                .range = 1.0f - engine->configuration->gamepad.inner_deadzone - engine->configuration->gamepad.outer_deadzone,
                .emulate_dpad = engine->configuration->gamepad.emulate_dpad,
                .emulate_cursor = engine->configuration->gamepad.emulate_cursor
            }
        }, Display_get_window(engine->display));
    if (!engine->input) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize input");
        Display_destroy(engine->display);
        Configuration_destroy(engine->configuration);
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    engine->audio = Audio_create(&(const Audio_Configuration_t){
            .device_index = engine->configuration->audio.device_index,
            .master_volume = engine->configuration->audio.master_volume
        });
    if (!engine->audio) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize audio");
        Input_destroy(engine->input);
        Display_destroy(engine->display);
        Configuration_destroy(engine->configuration);
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    engine->physics = Physics_create(&(const Physics_Configuration_t){ 0 });
    if (!engine->physics) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize physics");
        Audio_destroy(engine->audio);
        Input_destroy(engine->input);
        Display_destroy(engine->display);
        Configuration_destroy(engine->configuration);
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    engine->environment = Environment_create(argc, argv, engine->display);
    if (!engine->environment) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize environment");
        Physics_destroy(engine->physics);
        Audio_destroy(engine->audio);
        Input_destroy(engine->input);
        Display_destroy(engine->display);
        Configuration_destroy(engine->configuration);
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    engine->interpreter = Interpreter_create(engine->storage);
    if (!engine->interpreter) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't create interpreter");
        Environment_destroy(engine->environment);
        Physics_destroy(engine->physics);
        Audio_destroy(engine->audio);
        Input_destroy(engine->input);
        Display_destroy(engine->display);
        Configuration_destroy(engine->configuration);
        Storage_destroy(engine->storage);
        free(engine);
        return NULL;
    }

    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "engine: %s", TOFU_VERSION_STRING);

    return engine;
}

void Engine_destroy(Engine_t *engine)
{
    Interpreter_destroy(engine->interpreter); // Terminate the interpreter to unlock all resources.
    Environment_destroy(engine->environment);
    Physics_destroy(engine->physics);
    Audio_destroy(engine->audio);
    Input_destroy(engine->input);
    Display_destroy(engine->display);
    Configuration_destroy(engine->configuration);
    Storage_destroy(engine->storage);

    free(engine);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "engine freed");

#ifdef DEBUG
    stb_leakcheck_dumpmem();
#endif
}

static const char **_prepare_events(Engine_t *engine, const char **events) // TODO: move to lower-priority?
{
    arrsetlen(events, 0);

#ifdef __DISPLAY_FOCUS_SUPPORT__
    const Environment_State_t *environment_state = Environment_get_state(engine->environment);
    if (environment_state->active.was != environment_state->active.is) {
        arrpush(events, environment_state->active.is ? "on_focus_acquired" : "on_focus_lost");
    }
#endif  /* __DISPLAY_FOCUS_SUPPORT__ */

    const Input_State_t *input_state = Input_get_state(engine->input);
    if (input_state->gamepad.delta > 0) {
        arrpush(events, "on_gamepad_connected");
    } else
    if (input_state->gamepad.delta < 0) {
        arrpush(events, "on_gamepad_disconnected");
        if (input_state->gamepad.count == 0) {
            arrpush(events, "on_gamepad_not_available");
        }
    }

    arrpush(events, NULL);

    return events;
}

void Engine_run(Engine_t *engine)
{
    // Initialize the VM now that all the sub-systems are ready.
    bool booted = Interpreter_boot(engine->interpreter, (const void *[]){
            engine->storage,
            engine->display,
            engine->input,
            engine->audio,
            engine->environment,
            engine->physics,
            engine->interpreter,
            NULL
        });
    if (!booted) {
        Log_write(LOG_LEVELS_FATAL, LOG_CONTEXT, "can't initialize interpreter");
        return;
    }

    const float delta_time = 1.0f / (float)engine->configuration->engine.frames_per_seconds; // TODO: runtime configurable?
    const size_t skippable_frames = engine->configuration->engine.skippable_frames;
    const float reference_time = engine->configuration->engine.frames_limit == 0 ? 0.0f : 1.0f / engine->configuration->engine.frames_limit;
    Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "now running, update-time is %.6fs w/ %d skippable frames, reference-time is %.6fs", delta_time, skippable_frames, reference_time);

    // Track time using `double` to keep the min resolution consistent over time!
    // For intervals (i.e. deltas), `float` is sufficient.
    // https://randomascii.wordpress.com/2012/02/13/dont-store-that-in-a-float/
#ifdef __ENGINE_PERFORMANCE_STATISTICS__
    float deltas[4] = { 0 };
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */
    double previous = glfwGetTime();
    float lag = 0.0f;

    const char **events = NULL;
    arrsetcap(events, EVENTS_INITIAL_CAPACITY); // Pre-allocate some entries for the events, reducing reallocation in the main-loop.

    // https://nkga.github.io/post/frame-pacing-analysis-of-the-game-loop/
    for (bool running = true; running && !Display_should_close(engine->display); ) {
        const double current = glfwGetTime();
        const float elapsed = (float)(current - previous);
        previous = current;

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
        Environment_process(engine->environment, elapsed, deltas);
#else
        Environment_process(engine->environment, elapsed);
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */

        Input_process(engine->input);

        events = _prepare_events(engine, events);

        running = running && Interpreter_process(engine->interpreter, events); // Lazy evaluate `running`, will avoid calls when error.

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
        const double process_marker = glfwGetTime();
        deltas[0] = (float)(process_marker - current);
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */

        lag += elapsed; // Count a maximum amount of skippable frames in order no to stall on slower machines.
        for (size_t frames = skippable_frames; frames && (lag >= delta_time); --frames) {
            running = running && Environment_update(engine->environment, delta_time);
            running = running && Input_update(engine->input, delta_time); // First, update the input, accessed in the interpreter step.
            running = running && Interpreter_update(engine->interpreter, delta_time); // Update the subsystems w/ fixed steps (fake interrupt based).
            running = running && Audio_update(engine->audio, delta_time);
            running = running && Physics_update(engine->physics, delta_time);
            running = running && Storage_update(engine->storage, delta_time); // Note: we could update audio/storage one every two steps (or more).
            lag -= delta_time;
        }

//        running = running && Input_update_variable(engine->storage, elapsed);
//        running = running && Interpreter_update_variable(engine->interpreter, elapsed); // Variable update.
//        running = running && Audio_update_variable(&engine->audio, elapsed);
//        running = running && Physics_update_variable(engine->storage, elapsed);
//        running = running && Storage_update_variable(engine->storage, elapsed);
        Display_update(engine->display, elapsed);

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
        const double update_marker = glfwGetTime();
        deltas[1] = (float)(update_marker - process_marker);
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */

        running = running && Interpreter_render(engine->interpreter, lag / delta_time);

        Display_present(engine->display);

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
        const double render_marker = glfwGetTime();
        deltas[2] = (float)(render_marker - update_marker);
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */

#ifdef __GRAPHICS_CAPTURE_SUPPORT__
        const Input_Button_State_t *record_button = Input_get_button(engine->input, INPUT_BUTTON_RECORD);
        if (record_button->pressed) {
            Display_toggle_recording(engine->display, Storage_get_local_path(engine->storage));
        }

        const Input_Button_State_t *capture_button = Input_get_button(engine->input, INPUT_BUTTON_CAPTURE);
        if (capture_button->pressed) {
            Display_grab_snapshot(engine->display, Storage_get_local_path(engine->storage));
        }
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */

        if (reference_time != 0.0f) {
            const float frame_time = (float)(glfwGetTime() - current);
            const float leftover = reference_time - frame_time;
            if (leftover > 0.0f) {
                _wait_for(leftover); // FIXME: Add minor compensation to reach cap value?
            }
        }

#ifdef __ENGINE_PERFORMANCE_STATISTICS__
        deltas[3] = (float)(glfwGetTime() - current);
#endif  /* __ENGINE_PERFORMANCE_STATISTICS__ */
    }

    arrfree(events);
}
