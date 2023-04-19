/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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

#include <core/config.h>
#include <core/platform.h>
#include <core/version.h>
#include <libs/log.h>
#include <libs/stb.h>
#include <libs/sysinfo.h>

#if PLATFORM_ID == PLATFORM_WINDOWS
  #include <windows.h>
#endif

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

static Configuration_t *_configure(Storage_t *storage)
{
    const Storage_Resource_t *resource = Storage_load(storage, "tofu.config", STORAGE_RESOURCE_STRING);
    if (!resource) {
        LOG_F(LOG_CONTEXT, "configuration file is missing");
        return NULL;
    }

    Configuration_t *configuration = Configuration_create(S_SCHARS(resource));
    if (!configuration) {
        LOG_F(LOG_CONTEXT, "can't create configuration");
        return NULL;
    }

    Log_configure(configuration->system.debug, NULL);

    LOG_I(LOG_CONTEXT, "game identity is `%s`", configuration->system.identity);

    if (configuration->system.version.major > TOFU_VERSION_MAJOR
        || configuration->system.version.minor > TOFU_VERSION_MINOR
        || configuration->system.version.revision > TOFU_VERSION_REVISION) {
        LOG_F(LOG_CONTEXT, "engine version mismatch (required %d.%d.%d, current %d.%d.%d)",
            configuration->system.version.major, configuration->system.version.minor, configuration->system.version.revision,
            TOFU_VERSION_MAJOR, TOFU_VERSION_MINOR, TOFU_VERSION_REVISION);
        goto error_destroy_configuration;
    }

    return configuration;

error_destroy_configuration:
    Configuration_destroy(configuration);
    return NULL;
}

static inline void _information(void)
{
    SysInfo_Data_t si = { 0 };
    bool result = SysInfo_inspect(&si);
    if (!result) {
        LOG_E(LOG_CONTEXT, "can't get system information");
        return;
    }

    LOG_I(LOG_CONTEXT, "Tofu Engine v%s (%s build)", TOFU_VERSION_STRING, PLATFORM_NAME);
    LOG_I(LOG_CONTEXT, "running on %s %s (%s, %s)", si.system, si.architecture, si.release, si.version);
}

Engine_t *Engine_create(const Engine_Options_t *options)
{
    Engine_t *engine = malloc(sizeof(Engine_t));
    if (!engine) {
        LOG_E(LOG_CONTEXT, "can't allocate engine");
        return NULL;
    }

    *engine = (Engine_t){ 0 }; // Ensure is cleared at first.

    Log_initialize();

    _information();

    engine->storage = Storage_create(&(const Storage_Configuration_t){
            .kernal_path = options->kernal_path,
            .data_path = options->data_path
        });
    if (!engine->storage) {
        LOG_F(LOG_CONTEXT, "can't initialize storage");
        goto error_free;
    }
    LOG_I(LOG_CONTEXT, "storage ready");

    engine->configuration = _configure(engine->storage);
    if (!engine->configuration) {
        goto error_destroy_storage;
    }
    LOG_I(LOG_CONTEXT, "configuration ready");

    bool set = Storage_set_identity(engine->storage, engine->configuration->system.identity);
    if (!set) {
        LOG_F(LOG_CONTEXT, "can't set identity");
        goto error_destroy_configuration;
    }
    LOG_D(LOG_CONTEXT, "identity set to `%s`", engine->configuration->system.identity);

    const Storage_Resource_t *icon = Storage_load(engine->storage, engine->configuration->system.icon, STORAGE_RESOURCE_IMAGE);
    if (!icon) {
        LOG_F(LOG_CONTEXT, "can't load icon");
        goto error_destroy_configuration;
    }
    LOG_D(LOG_CONTEXT, "icon `%s` loaded", engine->configuration->system.icon);

    const Storage_Resource_t *effect = Storage_load(engine->storage, engine->configuration->display.effect, STORAGE_RESOURCE_STRING);
    if (!effect) {
        LOG_F(LOG_CONTEXT, "can't load effect");
        goto error_destroy_configuration;
    }
    LOG_D(LOG_CONTEXT, "effect `%s` loaded", engine->configuration->display.effect);

    const Storage_Resource_t *mappings = Storage_load(engine->storage, engine->configuration->system.mappings, STORAGE_RESOURCE_STRING);
    if (!mappings) {
        LOG_F(LOG_CONTEXT, "can't load mappings");
        goto error_destroy_configuration;
    }
    LOG_I(LOG_CONTEXT, "mappings `%s` loaded", engine->configuration->system.mappings);

    engine->display = Display_create(&(const Display_Configuration_t){
            .icon = (GLFWimage){ .width = (int)S_IWIDTH(icon), .height = (int)S_IHEIGHT(icon), .pixels = S_IPIXELS(icon) },
            .window = {
                .title = engine->configuration->display.title,
                .width = engine->configuration->display.width,
                .height = engine->configuration->display.height,
                .scale = engine->configuration->display.scale
            },
            .fullscreen = engine->configuration->display.fullscreen,
            .vertical_sync = engine->configuration->display.vertical_sync,
            .quit_on_close = engine->configuration->system.quit_on_close,
            .effect = S_SCHARS(effect)
        });
    if (!engine->display) {
        LOG_F(LOG_CONTEXT, "can't create display");
        goto error_destroy_configuration;
    }
    LOG_I(LOG_CONTEXT, "display ready");

    const GL_Size_t phyisical_size = Display_get_physical_size(engine->display);
    const GL_Size_t virtual_size = Display_get_virtual_size(engine->display);
    engine->input = Input_create(&(const Input_Configuration_t){
            .mappings = S_SCHARS(mappings),
            .screen = {
                .physical = {
                    .width = phyisical_size.width,
                    .height = phyisical_size.height
                },
                .virtual = {
                    .width = virtual_size.width,
                    .height = virtual_size.height
                }
            },
            .keyboard = {
#if defined(DEBUG)
                .exit_key = true
#else
                .exit_key = engine->configuration->keyboard.exit_key
#endif
            },
            .cursor = {
                .enabled = engine->configuration->cursor.enabled,
                .hide = engine->configuration->cursor.hide,
                .speed = engine->configuration->cursor.speed
            },
            .controller = {
                .deadzone = engine->configuration->controller.inner_deadzone, // FIXME: pass inner/outer and let the input code do the math?
                .range = 1.0f - engine->configuration->controller.inner_deadzone - engine->configuration->controller.outer_deadzone,
            }
        }, Display_get_window(engine->display));
    if (!engine->input) {
        LOG_F(LOG_CONTEXT, "can't initialize input");
        goto error_destroy_display;
    }
    LOG_I(LOG_CONTEXT, "input ready");

    engine->audio = Audio_create(&(const Audio_Configuration_t){
            .device_index = engine->configuration->audio.device_index,
            .master_volume = engine->configuration->audio.master_volume
        });
    if (!engine->audio) {
        LOG_F(LOG_CONTEXT, "can't initialize audio");
        goto error_destroy_input;
    }
    LOG_I(LOG_CONTEXT, "audio ready");

    engine->environment = Environment_create(engine->display, engine->input);
    if (!engine->environment) {
        LOG_F(LOG_CONTEXT, "can't initialize environment");
        goto error_destroy_audio;
    }
    LOG_I(LOG_CONTEXT, "environment ready");

    engine->interpreter = Interpreter_create(engine->storage);
    if (!engine->interpreter) {
        LOG_F(LOG_CONTEXT, "can't initialize interpreter");
        goto error_destroy_environment;
    }
    LOG_I(LOG_CONTEXT, "interpreter ready");

    LOG_I(LOG_CONTEXT, "engine is up and running");
    return engine;

    // Goto clean-up section.
error_destroy_environment:
    Environment_destroy(engine->environment);
error_destroy_audio:
    Audio_destroy(engine->audio);
error_destroy_input:
    Input_destroy(engine->input);
error_destroy_display:
    Display_destroy(engine->display);
error_destroy_configuration:
    Configuration_destroy(engine->configuration);
error_destroy_storage:
    Storage_destroy(engine->storage);
error_free:
    free(engine);
    return NULL;
}

void Engine_destroy(Engine_t *engine)
{
    Interpreter_destroy(engine->interpreter); // Terminate the interpreter to unlock all resources.
    Environment_destroy(engine->environment);
    Audio_destroy(engine->audio);
    Input_destroy(engine->input);
    Display_destroy(engine->display);
    Configuration_destroy(engine->configuration);
    Storage_destroy(engine->storage);

    free(engine);
    LOG_D(LOG_CONTEXT, "engine freed");

#if defined(STB_LEAKCHECK_INCLUDED)
    stb_leakcheck_dumpmem();
#endif
}

static const char **_prepare_events(Engine_t *engine, const char **events) // TODO: move to lower-priority?
{
    arrsetlen(events, 0);

    const Environment_State_t *environment_state = Environment_get_state(engine->environment);

#if defined(TOFU_EVENTS_FOCUS_SUPPORT)
    if (environment_state->active.was != environment_state->active.is) {
        arrpush(events, environment_state->active.is ? "on_focus_acquired" : "on_focus_lost");
    }
#endif  /* TOFU_EVENTS_FOCUS_SUPPORT */

#if defined(TOFU_EVENTS_CONTROLLER_SUPPORT)
    if (environment_state->controllers.previous != environment_state->controllers.current) {
        if (environment_state->controllers.current > environment_state->controllers.previous) {
            arrpush(events, "on_controller_connected");
            if (environment_state->controllers.current == 1) {
                arrpush(events, "on_controller_available");
            }
        } else {
            arrpush(events, "on_controller_disconnected");
            if (environment_state->controllers.current == 0) {
                arrpush(events, "on_controller_unavailable");
            }
        }
    }
#endif  /* TOFU_EVENTS_CONTROLLER_SUPPORT */

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
            engine->interpreter,
            NULL
        });
    if (!booted) {
        LOG_F(LOG_CONTEXT, "can't initialize interpreter");
        return;
    }

    const float delta_time = 1.0f / (float)engine->configuration->engine.frames_per_seconds; // TODO: runtime configurable?
    const size_t skippable_frames = engine->configuration->engine.skippable_frames;
    const float skippable_time = delta_time * skippable_frames; // This is the allowed "fast-forwardable" time window.
    const float reference_time = engine->configuration->engine.frames_limit == 0 ? 0.0f : 1.0f / engine->configuration->engine.frames_limit;
    LOG_I(LOG_CONTEXT, "now running, update-time is %.6fs w/ %d skippable frames (skippable-time is %.6fs), reference-time is %.6fs", delta_time, skippable_frames, skippable_time, reference_time);

    // Track time using `double` to keep the min resolution consistent over time!
    // For intervals (i.e. deltas), `float` is sufficient.
    // https://randomascii.wordpress.com/2012/02/13/dont-store-that-in-a-float/
#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
    float deltas[4] = { 0 };
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
    double previous = glfwGetTime();
    float lag = 0.0f;

    const char **events = NULL;
    arrsetcap(events, EVENTS_INITIAL_CAPACITY); // Pre-allocate some entries for the events, reducing reallocation in the main-loop.

    // https://nkga.github.io/post/frame-pacing-analysis-of-the-game-loop/
    for (bool running = true; running && !Display_should_close(engine->display); ) {
        const double current = glfwGetTime();

        // If the frame delta time exceeds the maximum allowed skippable one (because the system can't
        //  keep the pace we want) we forcibly cap the elapsed time.
        float elapsed = (float)(current - previous);
#if defined(DEBUG)
        // If we are running in debug mode we could be occasionally be interrupted due to breakpoint stepping.
        // We detect this by using a "max elapsed threshold" value. If we exceed it, we forcibly cap the elapsed
        // time to a single frame `delta_time`.
        if (elapsed >= TOFU_ENGINE_BREAKPOINT_DETECTION_THRESHOLD) {
            elapsed = delta_time;
        }
#endif  /* DEBUG */
        previous = current;

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        Environment_process(engine->environment, elapsed, deltas);
#else   /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
        Environment_process(engine->environment, elapsed);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

        glfwPollEvents();

        Input_process(engine->input);

        events = _prepare_events(engine, events);

        running = running && Interpreter_process(engine->interpreter, events); // Lazy evaluate `running`, will avoid calls when error.

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        const double process_marker = glfwGetTime();
        deltas[0] = (float)(process_marker - current);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

        lag += elapsed;
        if (lag > skippable_time) { // If the `lag` exceeds what we allow to "skip", cap it!
            lag = skippable_time;
        }
        while (lag >= delta_time) { // `lag` is capped, this loop won't accumulate and stall slower machines.
            running = running && Environment_update(engine->environment, delta_time);
            running = running && Input_update(engine->input, delta_time); // First, update the input, accessed in the interpreter step.
            running = running && Display_update(engine->display, delta_time);
            running = running && Interpreter_update(engine->interpreter, delta_time); // Update the subsystems w/ fixed steps (fake interrupt based).
            running = running && Audio_update(engine->audio, delta_time);
            running = running && Storage_update(engine->storage, delta_time); // Note: we could update audio/storage one every two steps (or more).
            lag -= delta_time;
        }

//        running = running && Input_update_variable(engine->storage, elapsed);
//        running = running && Display_update_variable(engine->display, elapsed);
//        running = running && Interpreter_update_variable(engine->interpreter, elapsed); // Variable update.
//        running = running && Audio_update_variable(&engine->audio, elapsed);
//        running = running && Storage_update_variable(engine->storage, elapsed);

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        const double update_marker = glfwGetTime();
        deltas[1] = (float)(update_marker - process_marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

        running = running && Interpreter_render(engine->interpreter, lag / delta_time);

        Display_present(engine->display);

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        const double render_marker = glfwGetTime();
        deltas[2] = (float)(render_marker - update_marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

        if (reference_time != 0.0f) {
            const float frame_time = (float)(glfwGetTime() - current);
            const float leftover = reference_time - frame_time;
            if (leftover > 0.0f) {
                _wait_for(leftover); // FIXME: Add minor compensation to reach cap value?
            }
        }

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        deltas[3] = (float)(glfwGetTime() - current);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
    }

    arrfree(events);
}
