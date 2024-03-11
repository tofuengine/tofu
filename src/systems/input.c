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

#include "input.h"

#include <libs/fmath.h>
#define _LOG_TAG "input"
#include <libs/log.h>
#include <libs/stb.h>

typedef void (*Input_Process_t)(Input_t *input);
typedef void (*Input_Update_t)(Input_t *input, float delta_time);

static void _keyboard_process(Input_t *input)
{
    static const int keys[Input_Keyboard_Buttons_t_CountOf] = {
        GLFW_KEY_1,
        GLFW_KEY_2,
        GLFW_KEY_3,
        GLFW_KEY_4,
        GLFW_KEY_5,
        GLFW_KEY_6,
        GLFW_KEY_7,
        GLFW_KEY_8,
        GLFW_KEY_9,
        GLFW_KEY_0,
        GLFW_KEY_Q,
        GLFW_KEY_W,
        GLFW_KEY_E,
        GLFW_KEY_R,
        GLFW_KEY_T,
        GLFW_KEY_Y,
        GLFW_KEY_U,
        GLFW_KEY_I,
        GLFW_KEY_O,
        GLFW_KEY_P,
        GLFW_KEY_A,
        GLFW_KEY_S,
        GLFW_KEY_D,
        GLFW_KEY_F,
        GLFW_KEY_G,
        GLFW_KEY_H,
        GLFW_KEY_J,
        GLFW_KEY_K,
        GLFW_KEY_L,
        GLFW_KEY_Z,
        GLFW_KEY_X,
        GLFW_KEY_C,
        GLFW_KEY_V,
        GLFW_KEY_B,
        GLFW_KEY_N,
        GLFW_KEY_M,
        GLFW_KEY_UP,
        GLFW_KEY_DOWN,
        GLFW_KEY_LEFT,
        GLFW_KEY_RIGHT,
        GLFW_KEY_ENTER,
        GLFW_KEY_SPACE,
        GLFW_KEY_F1,
        GLFW_KEY_F2,
        GLFW_KEY_F3,
        GLFW_KEY_F4,
        GLFW_KEY_F5,
        GLFW_KEY_F6,
        GLFW_KEY_F7,
        GLFW_KEY_F8,
        GLFW_KEY_F9,
        GLFW_KEY_F10,
        GLFW_KEY_F11,
        GLFW_KEY_F12
    };

    Input_Keyboard_t *keyboard = &input->state.keyboard;
    Input_Button_t *buttons = keyboard->buttons;

    GLFWwindow *window = input->window;
    for (size_t i = Input_Keyboard_Buttons_t_First; i <= Input_Keyboard_Buttons_t_Last; ++i) {
        Input_Button_t *button = &buttons[i];
        button->was = button->is; // Store current state and clear it.
        button->is = glfwGetKey(window, keys[i]) == GLFW_PRESS;
    }
}

static void _mouse_process(Input_t *input)
{
    static const int mouse_buttons[Input_Cursor_Buttons_t_CountOf] = {
        GLFW_MOUSE_BUTTON_LEFT,
        GLFW_MOUSE_BUTTON_RIGHT,
        GLFW_MOUSE_BUTTON_MIDDLE
    };

    Input_Cursor_t *cursor = &input->state.cursor;
    Input_Button_t *buttons = cursor->buttons;

    // As for the controllers, we need to reset the cursor state or (in case is emulated) any button press
    // will persist indefinitely.
    for (size_t i = Input_Cursor_Buttons_t_First; i <= Input_Cursor_Buttons_t_Last; ++i) {
        Input_Button_t *button = &buttons[i];
        button->was = button->is;
        button->is = false;
    }

    if (!cursor->enabled) {
        return;
    }

    GLFWwindow *window = input->window;
    for (size_t i = Input_Cursor_Buttons_t_First; i <= Input_Cursor_Buttons_t_Last; ++i) {
        Input_Button_t *button = &buttons[i];
        button->is = glfwGetMouseButton(window, mouse_buttons[i]) == GLFW_PRESS;
    }
}

// http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html
// http://blog.hypersect.com/interpreting-analog-sticks/
static inline Input_Controller_Stick_t _controller_stick(float x, float y, float deadzone, float range)
{
    const float magnitude = sqrtf(x * x + y * y);
    if (magnitude < deadzone) {
        return (Input_Controller_Stick_t){ .x = 0.0f, .y = 0.0f, .angle = 0.0f, .magnitude = 0.0f };
    } else { // Rescale to ensure [0, 1] range. Response curve is left to the final user.
        const float angle = atan2f(y, x);
        const float normalized_magnitude = fminf(1.0f, (magnitude - deadzone) / range);
        const float scale = normalized_magnitude / magnitude;
        return (Input_Controller_Stick_t){ .x = x * scale, .y = y * scale, .angle = angle, .magnitude = normalized_magnitude };
    }
}

static inline float _controller_trigger(float magnitude, float deadzone, float range)
{
    // TODO: optimize by calculating angle/magnitude only if the trigger status is requested.
    if (magnitude < deadzone) {
        return 0.0f;
    } else {
        return fminf(1.0f, (magnitude - deadzone) / range);
    }
}

static void _controller_process(Input_t *input)
{
    static const int controller_buttons[Input_Controller_Buttons_t_CountOf] = {
        GLFW_GAMEPAD_BUTTON_DPAD_UP,
        GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
        GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
        GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
        GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
        GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
        GLFW_GAMEPAD_BUTTON_LEFT_THUMB,
        GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
        GLFW_GAMEPAD_BUTTON_Y,
        GLFW_GAMEPAD_BUTTON_X,
        GLFW_GAMEPAD_BUTTON_B,
        GLFW_GAMEPAD_BUTTON_A,
        GLFW_GAMEPAD_BUTTON_BACK,
        GLFW_GAMEPAD_BUTTON_START,
    };

    const Input_Configuration_t *configuration = &input->configuration;
    const float deadzone = configuration->controller.deadzone;
    const float range = configuration->controller.range;

    Input_Controller_t *controllers = input->state.controllers;

    for (size_t id = 0; id < INPUT_CONTROLLERS_COUNT; ++id) {
        Input_Controller_t *controller = &controllers[id];
        Input_Button_t *buttons = controller->buttons;

        // We need to clear the controller state (and pass back the `is` value to `was`) so that it's moved back
        // not "neutral" in case of disconnection. Otherwise the latest input values would persist indefinitely (or
        // unless plugged back in).
        for (size_t i = Input_Controller_Buttons_t_First; i <= Input_Controller_Buttons_t_Last; ++i) {
            Input_Button_t *button = &buttons[i];
            button->was = button->is; // Store current state and clear it.
            button->is = false;
        }
        controller->sticks[INPUT_CONTROLLER_STICK_LEFT] = (Input_Controller_Stick_t){ 0 };
        controller->sticks[INPUT_CONTROLLER_STICK_RIGHT] = (Input_Controller_Stick_t){ 0 };
        controller->triggers = (Input_Controller_Triggers_t){ 0 };

        const int jid = controller->jid;
        if (jid == -1 || glfwJoystickPresent(jid) == GLFW_FALSE || glfwJoystickIsGamepad(jid) == GLFW_FALSE) { // Skip not present or not gamepad joysticks.
            continue;
        }

        GLFWgamepadstate gamepad;
        int result = glfwGetGamepadState(jid, &gamepad);
        if (result == GLFW_FALSE) {
            LOG_W("can't get controller #%d state", jid);
            continue;
        }

        for (size_t i = Input_Controller_Buttons_t_First; i <= Input_Controller_Buttons_t_Last; ++i) {
            Input_Button_t *button = &buttons[i];
            button->is = gamepad.buttons[controller_buttons[i]] == GLFW_PRESS;
        }

        Input_Controller_Stick_t *sticks = controller->sticks;
        sticks[INPUT_CONTROLLER_STICK_LEFT] = _controller_stick(gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X], gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y], deadzone, range);
        sticks[INPUT_CONTROLLER_STICK_RIGHT] = _controller_stick(gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y], deadzone, range);

        Input_Controller_Triggers_t *triggers = &controller->triggers;
        triggers->left = _controller_trigger(gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER], deadzone, range);
        triggers->right = _controller_trigger(gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER], deadzone, range);
    }
}

static inline void _initialize_keyboard(Input_Keyboard_t *keyboard, const Input_Configuration_t *configuration)
{
    // NOP.
}

static inline void _initialize_cursor(Input_Cursor_t *cursor, const Input_Configuration_t *configuration)
{
    const size_t pw = configuration->screen.physical.width;
    const size_t ph = configuration->screen.physical.height;
    const size_t vw = configuration->screen.virtual.width;
    const size_t vh = configuration->screen.virtual.height;

    cursor->area.x0 = 0.0f;
    cursor->area.y0 = 0.0f;
    cursor->area.x1 = (float)(vw - 1);
    cursor->area.y1 = (float)(vh - 1);

    cursor->scale.x = (float)vw / (float)pw; // Since aspect-ratio is enforced on source, it'is pedantic to have X/Y separate ratios.
    cursor->scale.y = (float)vh / (float)ph; // (but we want to generalize, so we stick to this choice)

    cursor->enabled = configuration->cursor.enabled;
}

static size_t _controllers_detect(Input_Controller_t *controllers, bool used_gamepads[GLFW_JOYSTICK_LAST + 1])
{
    size_t count = 0;

    for (size_t id = 0; id < INPUT_CONTROLLERS_COUNT; ++id) { // First loop, check for controller disconnection.
        Input_Controller_t *controller = &controllers[id];

        if (controller->jid == -1) {
            continue;
        }

        bool available = glfwJoystickPresent(controller->jid) == GLFW_TRUE && glfwJoystickIsGamepad(controller->jid) == GLFW_TRUE;
        if (!available) {
            LOG_D("controller #%d w/ jid #%d detached", id, controller->jid);
            used_gamepads[controller->jid] = false;
            controller->jid = -1;
            continue;
        }

        ++count;
    }

    for (size_t id = 0; id < INPUT_CONTROLLERS_COUNT; ++id) { // Bind a new gamepad to unavailable controllers, if any.
        Input_Controller_t *controller = &controllers[id];

        if (controller->jid != -1) {
            continue;
        }

        for (int jid = 0; jid <= GLFW_JOYSTICK_LAST; ++jid) {
            if (used_gamepads[jid]) {
                continue;
            }

            bool available = glfwJoystickPresent(jid) == GLFW_TRUE && glfwJoystickIsGamepad(jid) == GLFW_TRUE;
            if (!available) {
                continue;
            }

            ++count;
            controller->jid = jid;
            used_gamepads[controller->jid] = true;
            LOG_D("controller #%d found (jid #%d, GUID `%s`, name `%s`)", id, jid, glfwGetJoystickGUID(jid), glfwGetGamepadName(jid));
        }
    }

    return count;
}

static inline size_t _initialize_controllers(Input_Controller_t *controllers, bool used_gamepads[GLFW_JOYSTICK_LAST + 1])
{
    for (size_t id = 0; id < INPUT_CONTROLLERS_COUNT; ++id) {
        Input_Controller_t *controller = &controllers[id];

        controller->id = id; // Set internal controller identifier and clear the gamepad/joystick id.
        controller->jid = -1;
    }
    LOG_D("controller(s) initialized");

    size_t count = _controllers_detect(controllers, used_gamepads);
    if (count == 0) {
        LOG_W("no controllers detected");
    } else {
        LOG_I("%d controller(s) detected", count);
    }

    return count;
}

Input_t *Input_create(const Input_Configuration_t *configuration, GLFWwindow *window)
{
    int result = glfwUpdateGamepadMappings(configuration->mappings);
    if (result == GLFW_FALSE) {
        LOG_E("can't update controller mappings");
        return NULL;
    }
    LOG_D("input controller mappings updated");

    Input_t *input = malloc(sizeof(Input_t));
    if (!input) {
        LOG_E("can't allocate input");
        return NULL;
    }

    *input = (Input_t){
            .configuration = *configuration,
            .window = window
        };

    _initialize_keyboard(&input->state.keyboard, configuration);

    _initialize_cursor(&input->state.cursor, configuration);

    input->state.controllers_count = _initialize_controllers(input->state.controllers, input->state.used_gamepads);

    LOG_D("enabling sticky input mode");
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    return input;
}

void Input_destroy(Input_t *input)
{
    free(input);
    LOG_D("input freed");
}

static inline void _keyboard_update(Input_t *input, float delta_time)
{
    // NOP.
}

static inline void _move_and_bound_cursor(Input_Cursor_t *cursor, float x, float y)
{
    cursor->position.x = FCLAMP(x, cursor->area.x0, cursor->area.x1);
    cursor->position.y = FCLAMP(y, cursor->area.y0, cursor->area.y1);
}

static inline void _cursor_update(Input_t *input, float delta_time)
{
    Input_Cursor_t *cursor = &input->state.cursor;
    if (cursor->enabled) {
        GLFWwindow *window = input->window;

        // Note: getting the cursor position is slow, so we are doing it only in the update step.
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        const float scale_x = cursor->scale.x;
        const float scale_y = cursor->scale.y;
        _move_and_bound_cursor(cursor, (float)x * scale_x + 0.5f, (float)y * scale_y + 0.5f);
#if defined(TOFU_INPUT_CURSOR_IS_EMULATED)
    } else {
        const Input_Controller_t *controllers = input->state.controllers;
        for (size_t i = 0; i < INPUT_CONTROLLERS_COUNT; ++i) {
            const Input_Controller_t *controller = &controllers[i];

            if (controller->jid != -1) { // First available controller cursor-mapped will control the cursor.
                const Input_Controller_Stick_t *stick = &controller->sticks[INPUT_CONTROLLER_STICK_RIGHT]; // Right stick for cursor movement.
                const float delta = input->configuration.cursor.speed * delta_time;
                _move_and_bound_cursor(cursor, cursor->position.x + stick->x * delta, cursor->position.y + stick->y * delta);

                break;
            }
        }
#endif  /* TOFU_INPUT_CURSOR_IS_EMULATED */
    }
}

static inline void _controllers_update(Input_t *input, float delta_time)
{
    input->age += delta_time;

    // We don't need to update the controller detection in real-time, as the controllers' update function already
    // handles the "not initialized or disconnected" case.
    while (input->age >= TOFU_INPUT_CONTROLLER_DETECTION_PERIOD) {
        input->age -= TOFU_INPUT_CONTROLLER_DETECTION_PERIOD;
        input->state.controllers_count = _controllers_detect(input->state.controllers, input->state.used_gamepads);
    }
}

bool Input_update(Input_t *input, float delta_time)
{
    static const Input_Update_t updaters[] = {
        _keyboard_update,
        _cursor_update,
        _controllers_update,
        NULL
    };

    for (const Input_Update_t *updater = updaters; *updater; ++updater) {
        (*updater)(input, delta_time);
    }

    return true;
}

static inline void _buttons_sync(Input_Button_t *buttons, size_t first, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        Input_Button_t *button = &buttons[first + i];

        bool was_down = button->was;
        bool is_down = button->is;

        button->down = is_down;
        button->pressed = !was_down && is_down;
        button->released = was_down && !is_down;
    }
}

#if defined(TOFU_INPUT_CONTROLLER_IS_EMULATED) || defined(TOFU_INPUT_CURSOR_IS_EMULATED)
typedef struct Int_To_Int_s {
    int from, to;
} Int_To_Int_t;
#endif

#if defined(TOFU_INPUT_CONTROLLER_IS_EMULATED)
#define _KEYBOARD_A_CONTROLLER_ID   0
#define _KEYBOARD_B_CONTROLLER_ID   1

static Int_To_Int_t _keyboard_to_controller_0[] = {
    { INPUT_KEYBOARD_BUTTON_W, INPUT_CONTROLLER_BUTTON_UP },
    { INPUT_KEYBOARD_BUTTON_S, INPUT_CONTROLLER_BUTTON_DOWN },
    { INPUT_KEYBOARD_BUTTON_A, INPUT_CONTROLLER_BUTTON_LEFT },
    { INPUT_KEYBOARD_BUTTON_D, INPUT_CONTROLLER_BUTTON_RIGHT },
    { INPUT_KEYBOARD_BUTTON_C, INPUT_CONTROLLER_BUTTON_Y },
    { INPUT_KEYBOARD_BUTTON_F, INPUT_CONTROLLER_BUTTON_X },
    { INPUT_KEYBOARD_BUTTON_V, INPUT_CONTROLLER_BUTTON_B },
    { INPUT_KEYBOARD_BUTTON_G, INPUT_CONTROLLER_BUTTON_A },
    { INPUT_KEYBOARD_BUTTON_X, INPUT_CONTROLLER_BUTTON_SELECT },
    { INPUT_KEYBOARD_BUTTON_Z, INPUT_CONTROLLER_BUTTON_START },
    { -1, -1 }
};

static Int_To_Int_t _keyboard_to_controller_1[] = {
    { INPUT_KEYBOARD_BUTTON_UP, INPUT_CONTROLLER_BUTTON_UP },
    { INPUT_KEYBOARD_BUTTON_DOWN, INPUT_CONTROLLER_BUTTON_DOWN },
    { INPUT_KEYBOARD_BUTTON_LEFT, INPUT_CONTROLLER_BUTTON_LEFT },
    { INPUT_KEYBOARD_BUTTON_RIGHT, INPUT_CONTROLLER_BUTTON_RIGHT },
    { INPUT_KEYBOARD_BUTTON_K, INPUT_CONTROLLER_BUTTON_Y },
    { INPUT_KEYBOARD_BUTTON_O, INPUT_CONTROLLER_BUTTON_X },
    { INPUT_KEYBOARD_BUTTON_L, INPUT_CONTROLLER_BUTTON_B },
    { INPUT_KEYBOARD_BUTTON_P, INPUT_CONTROLLER_BUTTON_A },
    { INPUT_KEYBOARD_BUTTON_M, INPUT_CONTROLLER_BUTTON_SELECT },
    { INPUT_KEYBOARD_BUTTON_N, INPUT_CONTROLLER_BUTTON_START },
    { -1, -1 }
};
#endif

#if defined(TOFU_INPUT_CURSOR_IS_EMULATED)
#define _CURSOR_CONTROLLER_ID   0

static Int_To_Int_t _controller_to_cursor[] = {
    { INPUT_CONTROLLER_BUTTON_Y, INPUT_CURSOR_BUTTON_LEFT },
    { INPUT_CONTROLLER_BUTTON_X, INPUT_CURSOR_BUTTON_RIGHT },
    { INPUT_CONTROLLER_BUTTON_B, INPUT_CURSOR_BUTTON_MIDDLE },
    { -1, -1 }
};
#endif

#if defined(TOFU_INPUT_CONTROLLER_IS_EMULATED) || defined(TOFU_INPUT_CURSOR_IS_EMULATED)
static inline void _buttons_accumulate(Input_Button_t *target, const Input_Button_t *source, const Int_To_Int_t *mapping)
{
    for (size_t i = 0; mapping[i].from != -1; ++i) {
        if (target[mapping[i].to].is) { // Don't update if already pressed.
            continue;
        }
        target[mapping[i].to] = source[mapping[i].from];
    }
}
#endif

static inline void _buttons_process(Input_t *input)
{
    Input_Keyboard_t *keyboard = &input->state.keyboard;
    Input_Cursor_t *cursor = &input->state.cursor;
    Input_Controller_t *controllers = input->state.controllers;

    _buttons_sync(keyboard->buttons, Input_Keyboard_Buttons_t_First, Input_Keyboard_Buttons_t_CountOf);
    _buttons_sync(cursor->buttons, Input_Cursor_Buttons_t_First, Input_Cursor_Buttons_t_CountOf);
    for (size_t i = 0; i < INPUT_CONTROLLERS_COUNT; ++i) {
        Input_Controller_t *controller = &controllers[i];
        _buttons_sync(controller->buttons, Input_Controller_Buttons_t_First, Input_Controller_Buttons_t_CountOf);
    }

#if defined(TOFU_INPUT_CONTROLLER_IS_EMULATED)
    _buttons_accumulate(controllers[_KEYBOARD_A_CONTROLLER_ID].buttons, keyboard->buttons, _keyboard_to_controller_0);
    _buttons_accumulate(controllers[_KEYBOARD_B_CONTROLLER_ID].buttons, keyboard->buttons, _keyboard_to_controller_1);
#endif

#if defined(TOFU_INPUT_CURSOR_IS_EMULATED)
    const Input_Controller_t *controller = &controllers[_CURSOR_CONTROLLER_ID];
    if (!cursor->enabled) {
        _buttons_accumulate(cursor->buttons, controller->buttons, _controller_to_cursor);
    }
#endif
}

void Input_process(Input_t *input)
{
    static const Input_Process_t processors[] = {
        _keyboard_process,
        _mouse_process,
        _controller_process,
        NULL
    };

    for (const Input_Process_t *process = processors; *process; ++process) {
        (*process)(input);
    }

    _buttons_process(input);

    const Input_Configuration_t *configuration = &input->configuration;
    if (configuration->keyboard.exit_key && glfwGetKey(input->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        LOG_I("exit key pressed");
        glfwSetWindowShouldClose(input->window, true);
    }
}

Input_Keyboard_t *Input_get_keyboard(Input_t *input)
{
    return &input->state.keyboard;
}

Input_Cursor_t *Input_get_cursor(Input_t *input)
{
    return &input->state.cursor;
}

Input_Controller_t *Input_get_controller(Input_t *input, size_t id)
{
    if (id >= INPUT_CONTROLLERS_COUNT) {
        return NULL;
    }
    return &input->state.controllers[id];
}

size_t Input_get_controllers_count(const Input_t *input)
{
    return input->state.controllers_count;
}

bool Input_keyboard_is_available(const Input_Keyboard_t *keyboard)
{
    return true;
}

Input_Button_t Input_keyboard_get_button(const Input_Keyboard_t *keyboard, Input_Keyboard_Buttons_t button)
{
    return keyboard->buttons[button];
}

bool Input_cursor_is_available(const Input_Cursor_t *cursor)
{
#if defined(TOFU_INPUT_CURSOR_IS_EMULATED)
    return true;
#else
    return cursor->enabled;
#endif
}

Input_Button_t Input_cursor_get_button(const Input_Cursor_t *cursor, Input_Cursor_Buttons_t button)
{
    return cursor->buttons[button];
}

Input_Position_t Input_cursor_get_position(const Input_Cursor_t *cursor)
{
    return (Input_Position_t){
            .x = (int)cursor->position.x, // No need for rounding.
            .y = (int)cursor->position.y
        };
}

void Input_cursor_set_position(Input_Cursor_t *cursor, Input_Position_t position)
{
    cursor->position.x = (float)position.x + 0.5f; // Center on mid-pixel, as movements are float-based (to support dpad/stick)
    cursor->position.y = (float)position.y + 0.5f;
}

bool Input_controller_is_available(const Input_Controller_t *controller)
{
#if defined(TOFU_INPUT_CONTROLLER_IS_EMULATED)
    return controller->jid != -1 || controller->id < 2; // Controllers #0 and #1 are keyboard emulated, anyway.
#else
    return controller->jid != -1;
#endif
}

Input_Button_t Input_controller_get_button(const Input_Controller_t *controller, Input_Controller_Buttons_t button)
{
    return controller->buttons[button];
}

Input_Controller_Triggers_t Input_controller_get_triggers(const Input_Controller_t *controller)
{
    return controller->triggers;
}

Input_Controller_Stick_t Input_controller_get_stick(const Input_Controller_t *controller, Input_Controller_Sticks_t stick)
{
    return controller->sticks[stick];
}
