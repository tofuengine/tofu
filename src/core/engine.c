/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2026 Marco Lizza
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
#include <core/soy/soy.h>
#include <core/version.h>
#define _LOG_TAG "engine"
#include <libs/log.h>
#include <libs/stb.h>
#include <libs/stopwatch.h>
#include <libs/sysinfo.h>

// Value for setting the "zero time" of the engine. This will trick the system
// and get the consistent precision of an integer, with the convenient units
// of a double, as the exponent will remain constant for ~136 years (since the
// time unit is represented in seconds).
//
// See: `Four billion dollar question`, here https://randomascii.wordpress.com/2012/02/13/dont-store-that-in-a-float/
#define _ENGINE_EPOCH 4294967296.0

static Configuration_t *_configure(Storage_t *storage, const char **parameters)
{
    const Storage_Resource_t *resource = Storage_load(storage, "tofu.config", STORAGE_RESOURCE_STRING);
    if (!resource) {
        LOG_F("configuration file is missing");
        goto error_exit;
    }

    Configuration_t *configuration = Configuration_create();
    if (!configuration) {
        LOG_F("can't create configuration");
        goto error_exit;
    }

    Configuration_parse(configuration, SR_SCHARS(resource));
    Configuration_override(configuration, parameters);

    Log_configure(configuration->system.debug, NULL);

    LOG_I("game identity is `%s`", configuration->system.identity);

    if (configuration->system.version.major > TOFU_VERSION_MAJOR
        || configuration->system.version.minor > TOFU_VERSION_MINOR
        || configuration->system.version.revision > TOFU_VERSION_REVISION) {
        LOG_F("engine version mismatch (required %d.%d.%d, current %d.%d.%d)",
            configuration->system.version.major, configuration->system.version.minor, configuration->system.version.revision,
            TOFU_VERSION_MAJOR, TOFU_VERSION_MINOR, TOFU_VERSION_REVISION);
        goto error_destroy_configuration;
    }

    return configuration;

error_destroy_configuration:
    Configuration_destroy(configuration);
error_exit:
    return NULL;
}

static const char *_banner =
                            "        ___________________  _______________ ___        \n"
                            "        \\__    ___/\\_____  \\ \\_   _____/    |   \\       \n"
                            "          |    |    /   |   \\ |    __) |    |   /       \n"
                            "          |    |   /    |    \\|     \\  |    |  /        \n"
                            "          |____|   \\_______  /\\___  /  |______/         \n"
                            "                           \\/     \\/                    \n"
                            "___________ _______    ________.___ _______  ___________\n"
                            "\\_   _____/ \\      \\  /  _____/|   |\\      \\ \\_   _____/\n"
                            " |    __)_  /   |   \\/   \\  ___|   |/   |   \\ |    __)_ \n"
                            " |        \\/    |    \\    \\_\\  \\   /    |    \\|        \\\n"
                            "/_______  /\\____|__  /\\______  /___\\____|__  /_______  /\n"
                            "        \\/         \\/        \\/            \\/        \\/";

static inline void _information(void)
{
    LOG_I("Tofu Engine v%s (%s build)\n%s", TOFU_VERSION_STRING, PLATFORM_NAME, _banner);

    SysInfo_Data_t si = { 0 };
    bool result = SysInfo_inspect(&si);
    if (!result) {
        LOG_E("can't get system information");
        return;
    }
    LOG_I("running on %s %s (%s, %s)", si.system, si.architecture, si.release, si.version);
    // TODO: display also the free RAM
}

Engine_t *Engine_create(const Engine_Options_t *options)
{
    Engine_t *engine = malloc(sizeof(Engine_t));
    if (!engine) {
        LOG_E("can't allocate engine");
        goto error_exit;
    }

    *engine = (Engine_t){ 0 }; // Ensure is cleared at first.

    Log_initialize();

    _information();

    bool initialized = soy_init(); // Initialize the SOY system as soon as possible!
    if (!initialized) {
        LOG_F("can't initialize SOY");
        goto error_free_engine;
    }

    engine->storage = Storage_create(&(const Storage_Configuration_t){
            .kernal_path = options->kernal_path,
            .data_path = options->data_path
        });
    if (!engine->storage) {
        LOG_F("can't initialize storage");
        goto error_deinitialize_soy;
    }
    LOG_I("storage ready");

    engine->configuration = _configure(engine->storage, options->parameters);
    if (!engine->configuration) {
        goto error_destroy_storage;
    }
    LOG_I("configuration ready");

    bool set = Storage_set_identity(engine->storage, engine->configuration->system.identity);
    if (!set) {
        LOG_F("can't set identity");
        goto error_destroy_configuration;
    }
    LOG_D("identity set to `%s`", engine->configuration->system.identity);

    const Storage_Resource_t *effect = Storage_load(engine->storage, engine->configuration->display.effect, STORAGE_RESOURCE_STRING);
    if (!effect) {
        LOG_F("can't load effect");
        goto error_destroy_configuration;
    }
    LOG_D("effect `%s` loaded", engine->configuration->display.effect);

    const Storage_Resource_t *mappings = Storage_load(engine->storage, engine->configuration->system.mappings, STORAGE_RESOURCE_STRING);
    if (!mappings) {
        LOG_F("can't load mappings");
        goto error_destroy_configuration;
    }
    LOG_I("mappings `%s` loaded", engine->configuration->system.mappings);

    engine->display = Display_create(&(const Display_Configuration_t){
            .window = {
                .title = engine->configuration->display.title,
                .width = engine->configuration->display.width,
                .height = engine->configuration->display.height,
                .scale = engine->configuration->display.scale
            },
            .fullscreen = engine->configuration->display.fullscreen,
            .vertical_sync = engine->configuration->display.vertical_sync,
            .quit_on_close = engine->configuration->system.quit_on_close,
            .effect = SR_SCHARS(effect)
        });
    if (!engine->display) {
        LOG_F("can't create display");
        goto error_destroy_configuration;
    }
    LOG_I("display ready");

    const GL_Size_t physical_size = Display_get_physical_size(engine->display);
    const GL_Size_t virtual_size = Display_get_virtual_size(engine->display);
    engine->input = Input_create(&(const Input_Configuration_t){
            .mappings = SR_SCHARS(mappings),
            .screen = {
                .physical = {
                    .width = physical_size.width,
                    .height = physical_size.height
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
        LOG_F("can't initialize input");
        goto error_destroy_display;
    }
    LOG_I("input ready");

    engine->audio = Audio_create(&(const Audio_Configuration_t){
            .device_index = engine->configuration->audio.device_index,
            .master_volume = engine->configuration->audio.master_volume
        });
    if (!engine->audio) {
        LOG_F("can't initialize audio");
        goto error_destroy_input;
    }
    LOG_I("audio ready");

    engine->interpreter = Interpreter_create(engine->storage);
    if (!engine->interpreter) {
        LOG_F("can't initialize interpreter");
        goto error_destroy_audio;
    }
    LOG_I("interpreter ready");

    engine->environment = Environment_create(&(const Environment_Configuration_t){
            .debug = engine->configuration->system.debug,
#if defined(TOFU_ENGINE_SCRIPT_LEVEL_PROFILING)
            .profile = engine->configuration->system.profile
#endif  /* TOFU_ENGINE_SCRIPT_LEVEL_PROFILING */
        }, engine->display, engine->interpreter);
    if (!engine->environment) {
        LOG_F("can't initialize environment");
        goto error_destroy_interpreter;
    }
    LOG_I("environment ready");

    LOG_I("engine ready to boot");
    return engine;

    // Goto clean-up section.
error_destroy_interpreter:
    Interpreter_destroy(engine->interpreter);
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
error_deinitialize_soy:
    soy_deinit();
error_free_engine:
    free(engine);
error_exit:
    return NULL;
}

void Engine_destroy(Engine_t *engine)
{
    Environment_destroy(engine->environment);
    Interpreter_destroy(engine->interpreter); // Terminate the interpreter to unlock all resources.
    Audio_destroy(engine->audio);
    Input_destroy(engine->input);
    Display_destroy(engine->display);
    Configuration_destroy(engine->configuration);
    Storage_destroy(engine->storage);
    soy_deinit();

    free(engine);
    LOG_D("engine freed");
}

bool Engine_boot(Engine_t *engine)
{
    // Initialize the VM, all the sub-systems are ready.
    bool booted = Interpreter_boot(engine->interpreter, (const void *[]){
            engine->storage,
            engine->display,
            engine->input,
            engine->audio,
            engine->interpreter,
            engine->environment,
            NULL
        });
    if (!booted) {
        LOG_F("can't boot engine");
        goto error_exit;
    }

    soy_set_time(_ENGINE_EPOCH);
    LOG_D("engine epoch initialized");

    LOG_I("engine is up");
    return true;

error_exit:
    return false;
}

void Engine_shutdown(Engine_t *engine)
{
    Interpreter_shutdown(engine->interpreter);

    const Environment_State_t *state = Environment_get_state(engine->environment);
    LOG_I("total uptime is %.3f second(s)", state->time);
}

static inline void _process(void)
{
    // We need to call the `glfwPollEvents()` functions periodically so that the input events are
    // served. There's no need to call it on every single frame-loop, we could call it at the beginning
    // of the high priority handling.
    glfwPollEvents();
}

static inline bool _high_priority_update(Engine_t *engine, float delta_time)
{
    return Environment_update(engine->environment, delta_time)
            && Input_update(engine->input, delta_time) // First, update the input, accessed in the interpreter step.
            && Display_update(engine->display, delta_time)
            && Interpreter_update(engine->interpreter, delta_time)  // Update the subsystems w/ fixed steps (fake interrupt based).
            ;
}

static inline bool _low_priority_update(Engine_t *engine, float delta_time)
{
    return Audio_update(engine->audio, delta_time)
#if defined(TOFU_STORAGE_AUTO_COLLECT)
            && Storage_update(engine->storage, delta_time)
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
#if TOFU_INTERPRETER_GC_MODE == GC_MODE_CONTINUOUS
            && Interpreter_collect(engine->interpreter)
#endif
            ;
}

static inline bool _render(const Engine_t *engine, float ratio)
{
    if (!Interpreter_render(engine->interpreter, ratio)) {
        return false;
    }

    Display_present(engine->display);

    return true;
}

void Engine_run(Engine_t *engine)
{
    const float delta_time = 1.0f / (float)engine->configuration->engine.frames_per_seconds; // TODO: runtime configurable?
    const float low_priority_delta_time = 1.0f / (float)engine->configuration->engine.low_priority_frames_per_seconds;
    const size_t skippable_frames = engine->configuration->engine.skippable_frames;
    const float skippable_time = delta_time * (float)skippable_frames; // This is the allowed "fast-forwardable" time window.
    const float reference_time = engine->configuration->engine.frames_limit == 0 ? 0.0f : 1.0f / (float)engine->configuration->engine.frames_limit;
    LOG_I("now running, delta-time is %.6fs (%.6fs low-priority) w/ %d skippable frames, reference-time is %.6fs", delta_time, low_priority_delta_time, skippable_frames, reference_time);

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
    float deltas[Environment_Index_t_CountOf] = { 0 };
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
    StopWatch_t marker = stopwatch_init();
    float lag = 0.0f;
    float low_priority_lag = 0.0f;

    for (bool running = true; running && !Display_should_close(engine->display); ) {
        // If the frame delta time exceeds the maximum allowed skippable one (because the system can't
        // keep the pace we want) we forcibly cap the elapsed time.
        float frame_time = stopwatch_partial(&marker);
#if defined(DEBUG)
        // If we are running in debug mode we could be occasionally be interrupted due to breakpoint stepping.
        // We detect this by using a "max elapsed threshold" value. If we exceed it, we forcibly cap the elapsed
        // time to a single frame `delta_time`.
        if (frame_time >= TOFU_ENGINE_BREAKPOINT_DETECTION_THRESHOLD) {
            frame_time = delta_time;
        }
#endif  /* DEBUG */

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        StopWatch_t stats_marker = stopwatch_clone(&marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        Environment_accumulate(engine->environment, frame_time, deltas);
#else   /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
        Environment_accumulate(engine->environment, frame_time);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

        _process();

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        deltas[ENVIRONMENT_INDEX_PROCESS] = stopwatch_partial(&stats_marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

        // We already capped the `lag` accumulator value (relative to a maximum amount of skippable
        // frames). Now we process all the accumulated frames, if any, or the `lag` variable
        // could make `ratio` fall outside the `[0, 1]` range.
        lag += frame_time;
        if (lag > skippable_time) { // If we accumulated more that we can process just cap...
            lag = skippable_time;
        }
        while (lag >= delta_time) {
            lag -= delta_time;

            running = running && _high_priority_update(engine, delta_time); // Lazy evaluate `running`, will avoid calls when error.

            // Same as above, but we are executing on another time-frame and with `delta_time` steps.
            // As we are putting lower-priority activities here there's no need for us to be as
            // consistent and precise with the updates.
            low_priority_lag += delta_time;
            while (low_priority_lag > low_priority_delta_time) {
                low_priority_lag -= low_priority_delta_time;

                running = running && _low_priority_update(engine, low_priority_delta_time);
            }
        }

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        deltas[ENVIRONMENT_INDEX_UPDATE] = stopwatch_partial(&stats_marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

        running = running && _render(engine, lag / delta_time); // ratio in the range `[0, 1]`

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        deltas[ENVIRONMENT_INDEX_RENDER] = stopwatch_partial(&stats_marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

        const float busy_time = stopwatch_elapsed(&marker);
        const float wait_time = reference_time - busy_time; // When non-positive it means we are not capping. :P
        if (wait_time > __FLT_EPSILON__) {
#if defined(TOFU_ENGINE_WAIT_SKID_COMPENSATION)
            // We wait for the require amount of time but, at the same time, we also calculate the "skid"
            // (i.e. the difference) from the actual waited time. We then take into account for the difference,
            // as this will ensure rock-steady (average) FPS, especially if the system takes over during the
            // yield time and makes the application wait more than expected.
            StopWatch_t wait_marker = stopwatch_init();
#endif  /* TOFU_ENGINE_WAIT_SKID_COMPENSATION */
            soy_wait_for(wait_time);
#if defined(TOFU_ENGINE_WAIT_SKID_COMPENSATION)
            const float actual_wait_time = stopwatch_elapsed(&wait_marker);
            const float skid = actual_wait_time - wait_time; // Positive values means the wait has been longer than expected...
            stopwatch_delta(&marker, skid); // ... so we move the start-of-frame marker to account for the difference.
#endif  /* TOFU_ENGINE_WAIT_SKID_COMPENSATION */
        }

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        deltas[ENVIRONMENT_INDEX_WAIT] = stopwatch_partial(&stats_marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        // The frame-time statistic doesn't take into account of time
        deltas[ENVIRONMENT_INDEX_FRAME] = stopwatch_elapsed(&marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
    }
}
