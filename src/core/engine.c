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
 * Copyright (c) 2019-2024 Marco Lizza
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
#define _LOG_TAG "engine"
#include <libs/log.h>
#include <libs/stb.h>
#include <libs/stopwatch.h>
#include <libs/sysinfo.h>

#if PLATFORM_ID == PLATFORM_LINUX && defined(TOFU_ENGINE_USE_USLEEP)
    #define _USE_USLEEP
#endif

#if PLATFORM_ID == PLATFORM_WINDOWS
    #include <windows.h>
#elif defined(_USE_USLEEP)
    #include <sched.h>
    #include <unistd.h>
#else
    #include <sched.h>
    #include <time.h>
#endif

#define _EVENTS_INITIAL_CAPACITY 8

// This is the lowest amount of time (in milliseconds) that we are willing to
// suspend the execution (i.e. sleep) in a single loop.
//
// We set it to `1` as the actual sleep call will almost certainly (overall)
// consume a bit more than the requested amount (due to the call overhead). This
// way we are reasonably sure not to oversleep, at the cost of burning with a
// "semi-busy wait" the last millisecond (at most).
//
// See: https://nkga.github.io/post/frame-pacing-analysis-of-the-game-loop/
//      https://github.com/urho3d/urho3d/blob/master/Source/Urho3D/Engine/Engine.cpp#L750
#define _WAIT_SLOT  1

static inline void _wait_for(float seconds)
{
    StopWatch_t now = stopwatch_init();

    for (;;) {
        const double elapsed = stopwatch_elapsed(&now);
        if (elapsed >= seconds) { // The requested time has passed, bail out!
            break;
        }
        long millis = (long)((seconds - elapsed) * 1000.0); // The delta time is expressed in seconds...
        if (millis > _WAIT_SLOT) {
            // If more than a the wait-slot is left to wait then suspend the execution
            // for that amount of time...
            long millis_to_wait = millis - _WAIT_SLOT;
#if PLATFORM_ID == PLATFORM_WINDOWS
            Sleep(millis_to_wait);
#elif defined(_USE_USLEEP)
            usleep(millis_to_wait * 1000L); // usleep takes sleep time in us (1 millionth of a second)
#else
            struct timespec ts = (struct timespec){
                    .tv_sec = (time_t)(millis_to_wait / 1000L),
                    .tv_nsec = (time_t)((millis_to_wait % 1000L) * 1000000L)
                };
#if !defined(_USE_CLOCK_NANOSLEEP)
            nanosleep(&ts, NULL);
#else
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
#endif
#endif
        } else {
            // ... otherwise adopt as semi-busy wait, yielding the processor
            // usage to avoid "clogging" the system and successive "sleep spikes".
#if PLATFORM_ID == PLATFORM_WINDOWS
            YieldProcessor();
#else
            sched_yield();
#endif
        }
    }
}

static Configuration_t *_configure(Storage_t *storage)
{
    const Storage_Resource_t *resource = Storage_load(storage, "tofu.config", STORAGE_RESOURCE_STRING);
    if (!resource) {
        LOG_F("configuration file is missing");
        return NULL;
    }

    Configuration_t *configuration = Configuration_create(SR_SCHARS(resource));
    if (!configuration) {
        LOG_F("can't create configuration");
        return NULL;
    }

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
}

Engine_t *Engine_create(const Engine_Options_t *options)
{
    Engine_t *engine = malloc(sizeof(Engine_t));
    if (!engine) {
        LOG_E("can't allocate engine");
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
        LOG_F("can't initialize storage");
        goto error_free;
    }
    LOG_I("storage ready");

    engine->configuration = _configure(engine->storage);
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

    const Storage_Resource_t *icon = Storage_load(engine->storage, engine->configuration->system.icon, STORAGE_RESOURCE_IMAGE);
    if (!icon) {
        LOG_F("can't load icon");
        goto error_destroy_configuration;
    }
    LOG_D("icon `%s` loaded", engine->configuration->system.icon);

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
            .icon = (GLFWimage){
                .width = (int)SR_IWIDTH(icon),
                .height = (int)SR_IHEIGHT(icon),
                .pixels = SR_IPIXELS(icon)
            },
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

    engine->environment = Environment_create(engine->display, engine->input);
    if (!engine->environment) {
        LOG_F("can't initialize environment");
        goto error_destroy_audio;
    }
    LOG_I("environment ready");

    engine->interpreter = Interpreter_create(engine->storage);
    if (!engine->interpreter) {
        LOG_F("can't initialize interpreter");
        goto error_destroy_environment;
    }

    LOG_I("engine is up and running");
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
    LOG_D("engine freed");

#if defined(STB_LEAKCHECK_INCLUDED)
    stb_leakcheck_dumpmem();
#endif
}

// FIXME: use a bit-mask to track the events?
static const char **_prepare_events(const Engine_t *engine, const char **events) // TODO: move to lower-priority?
{
    arrsetlen(events, 0);

#if defined(TOFU_EVENTS_FOCUS_SUPPORT) || defined(TOFU_EVENTS_CONTROLLER_SUPPORT)
    const Environment_State_t *environment_state = Environment_get_state(engine->environment);
#endif

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

typedef struct _TimeLine_s {
    bool (*update)(Engine_t *engine, float delta_time);
    float delta_time;
    float lag;
} _TimeLine_t;

typedef enum _TimeLine_Identifier_e {
    TIMELINE_IDENTIFIER_HIGH,
    TIMELINE_IDENTIFIER_LOW,
    _TimeLine_Identifier_t_CountOf
} _TimeLine_Identifier_t;

static bool _high_priority_update(Engine_t *engine, float delta_time)
{
    return Environment_update(engine->environment, delta_time)
            && Input_update(engine->input, delta_time) // First, update the input, accessed in the interpreter step.
            && Display_update(engine->display, delta_time)
            && Interpreter_update(engine->interpreter, delta_time)  // Update the subsystems w/ fixed steps (fake interrupt based).
            ;
}

static bool _low_priority_update(Engine_t *engine, float delta_time)
{
    return Audio_update(engine->audio, delta_time)
#if defined(TOFU_STORAGE_AUTO_COLLECT)
            && Storage_update(engine->storage, delta_time)
#endif  /* TOFU_STORAGE_AUTO_COLLECT */
            ;
}

void Engine_run(Engine_t *engine)
{
    const float delta_time = 1.0f / (float)engine->configuration->engine.frames_per_seconds; // TODO: runtime configurable?
    const float low_priority_delta_time = 1.0f / (float)engine->configuration->engine.low_priority_frames_per_seconds;
    const size_t skippable_frames = engine->configuration->engine.skippable_frames;
    const float skippable_time = delta_time * (float)skippable_frames; // This is the allowed "fast-forwardable" time window.
    const float reference_time = engine->configuration->engine.frames_limit == 0 ? 0.0f : 1.0f / (float)engine->configuration->engine.frames_limit;
    LOG_I("now running, update-time is %.6fs w/ %d skippable frames, reference-time is %.6fs", delta_time, skippable_frames, reference_time);

    _TimeLine_t timelines[_TimeLine_Identifier_t_CountOf] = {
            {
                .update = _high_priority_update,
                .delta_time = delta_time,
                .lag = 0.0f
            },
            {
                .update = _low_priority_update,
                .delta_time = low_priority_delta_time,
                .lag = 0.0f
            },
        };

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
    float deltas[5] = { 0 };
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
    StopWatch_t marker = stopwatch_init();

    const char **events = NULL;
    arrsetcap(events, _EVENTS_INITIAL_CAPACITY); // Pre-allocate some entries for the events, reducing reallocation in the main-loop.

    for (bool running = true; running && !Display_should_close(engine->display); ) {
        // If the frame delta time exceeds the maximum allowed skippable one (because the system can't
        // keep the pace we want) we forcibly cap the elapsed time.
        float elapsed = stopwatch_partial(&marker);
#if defined(DEBUG)
        // If we are running in debug mode we could be occasionally be interrupted due to breakpoint stepping.
        // We detect this by using a "max elapsed threshold" value. If we exceed it, we forcibly cap the elapsed
        // time to a single frame `delta_time`.
        if (elapsed >= TOFU_ENGINE_BREAKPOINT_DETECTION_THRESHOLD) {
            elapsed = delta_time;
        }
#endif  /* DEBUG */

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        StopWatch_t stats_marker = stopwatch_clone(&marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

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
        deltas[0] = stopwatch_partial(&stats_marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

        // We already capped the `lag` accumulator value (relative to a maximum amount of skippable
        // frames). Now we process all the accumulated frames, if any, or the `lag` variable
        // could make `ratio` fall outside the `[0, 1]` range.
        for (size_t i = 0; i < _TimeLine_Identifier_t_CountOf; ++i) {
            _TimeLine_t *timeline = &timelines[i];

            timeline->lag += elapsed;
            if (timeline->lag > skippable_time) { // If we accumulated more that we can process just cap...
                timeline->lag = skippable_time;
            }
            while (timeline->lag >= timeline->delta_time) {
                timeline->lag -= timeline->delta_time;

                running = running && timeline->update(engine, timeline->delta_time);
            }
        }

//        running = running && Input_update_variable(engine->storage, elapsed);
//        running = running && Display_update_variable(engine->display, elapsed);
//        running = running && Interpreter_update_variable(engine->interpreter, elapsed); // Variable update.
//        running = running && Audio_update_variable(&engine->audio, elapsed);
//        running = running && Storage_update_variable(engine->storage, elapsed);

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        deltas[1] = stopwatch_partial(&stats_marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

        const _TimeLine_t *timeline = &timeline[TIMELINE_IDENTIFIER_HIGH];
        running = running && Interpreter_render(engine->interpreter, timeline->lag / timeline->delta_time); // ratio in the range `[0, 1]`

        Display_present(engine->display);

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        deltas[2] = stopwatch_partial(&stats_marker);
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
            _wait_for(wait_time);
#if defined(TOFU_ENGINE_WAIT_SKID_COMPENSATION)
            const float actual_wait_time = stopwatch_elapsed(&wait_marker);
            const float skid = actual_wait_time - wait_time; // Positive values means the wait has been longer than expected...
            stopwatch_delta(&marker, skid); // ... so we move the start-of-frame marker to account for the difference.
#endif  /* TOFU_ENGINE_WAIT_SKID_COMPENSATION */
        }

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        deltas[3] = stopwatch_partial(&stats_marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */

#if defined(TOFU_ENGINE_PERFORMANCE_STATISTICS)
        // The frame-time statistic doesn't take into account of time 
        deltas[4] = stopwatch_elapsed(&marker);
#endif  /* TOFU_ENGINE_PERFORMANCE_STATISTICS */
    }

    arrfree(events);
}
