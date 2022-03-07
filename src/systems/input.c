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

#include "input.h"

#include <libs/log.h>
#include <libs/stb.h>

#include <math.h>

#define LOG_CONTEXT "input"

typedef void (*Input_Handler_t)(Input_t *input);

static void _keyboard_handler(Input_t *input)
{
    static const int keys[Input_Buttons_t_CountOf] = {
        GLFW_KEY_UP,
        GLFW_KEY_DOWN,
        GLFW_KEY_LEFT,
        GLFW_KEY_RIGHT,
        GLFW_KEY_Q,
        GLFW_KEY_R,
        GLFW_KEY_W,
        GLFW_KEY_E,
        GLFW_KEY_Z,
        GLFW_KEY_S,
        GLFW_KEY_X,
        GLFW_KEY_D,
        GLFW_KEY_ENTER,
        GLFW_KEY_SPACE
    };

    GLFWwindow *window = input->window;
    Input_Button_t *buttons = input->state.buttons;
    for (size_t i = Input_Buttons_t_First; i <= Input_Buttons_t_Last; ++i) {
        Input_Button_t *button = &buttons[i];
        button->was = button->is; // Store current state and clear it.
        button->is = glfwGetKey(window, keys[i]) == GLFW_PRESS;
    }
}

static inline void _move_and_bound_cursor(Input_Cursor_t *cursor, float x, float y)
{
    cursor->x = x;
    cursor->y = y;

    if (cursor->x < cursor->area.x0) {
        cursor->x = cursor->area.x0;
    } else
    if (cursor->x > cursor->area.x1) {
        cursor->x = cursor->area.x1;
    }
    if (cursor->y < cursor->area.y0) {
        cursor->y = cursor->area.y0;
    } else
    if (cursor->y > cursor->area.y1) {
        cursor->y = cursor->area.y1;
    }
}

static void _mouse_handler(Input_t *input)
{
    static const int mouse_buttons[Input_Cursor_Buttons_t_CountOf] = {
        GLFW_MOUSE_BUTTON_MIDDLE,
        GLFW_MOUSE_BUTTON_RIGHT,
        GLFW_MOUSE_BUTTON_LEFT,
    };

    GLFWwindow *window = input->window;

    Input_Cursor_t *cursor = &input->state.cursor;
    Input_Button_t *buttons = cursor->buttons;
    for (size_t i = Input_Cursor_Buttons_t_First; i <= Input_Cursor_Buttons_t_Last; ++i) {
        if (mouse_buttons[i] == -1) {
            continue;
        }
        Input_Button_t *button = &buttons[i];
        button->was = button->is;
        button->is = glfwGetMouseButton(window, mouse_buttons[i]) == GLFW_PRESS;
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    const float scale = input->configuration.cursor.scale;
    _move_and_bound_cursor(cursor, (float)x * scale + 0.5f, (float)y * scale + 0.5f);
}

// http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html
// http://blog.hypersect.com/interpreting-analog-sticks/
static inline Input_Controller_Stick_t _gamepad_stick(float x, float y, float deadzone, float range)
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

static inline float _gamepad_trigger(float magnitude, float deadzone, float range)
{
    // TODO: optimize by calculating angle/magnitude only if the trigger status is requested.
    if (magnitude < deadzone) {
        return 0.0f;
    } else {
        return fminf(1.0f, (magnitude - deadzone) / range);
    }
}

static void _gamepad_handler(Input_t *input)
{
    static const int gamepad_buttons[Input_Buttons_t_CountOf] = {
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
    Input_Controller_t *controllers = input->state.controllers;

    for (int jid = 0; jid <= GLFW_JOYSTICK_LAST; ++jid) {
        if (!glfwJoystickPresent(jid) == GLFW_FALSE || !glfwJoystickIsGamepad(jid)) { // Skip not present or not gamepad joysticks.
            continue;
        }

        GLFWgamepadstate gamepad;
        int result = glfwGetGamepadState(jid, &gamepad);
        if (result == GLFW_FALSE) {
            Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "can't get gamepad #%d state", jid);
            continue;
        }

        Input_Controller_t *controller = &controllers[jid];
        Input_Button_t *buttons = controller->buttons;
        for (size_t i = Input_Buttons_t_First; i <= Input_Buttons_t_Last; ++i) {
            Input_Button_t *button = &buttons[i];
            button->was = button->is; // Store current state and clear it.
            button->is = gamepad.buttons[gamepad_buttons[i]] == GLFW_PRESS;
        }

        if (controller->flags & INPUT_FLAG_DPAD) {
            const float x = gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X]; // Left stick controls the DPAD.
            const float y = gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
            if (fabsf(x) > configuration->gamepad.sensitivity) {
                buttons[x < 0.0f ? INPUT_BUTTON_LEFT : INPUT_BUTTON_RIGHT].is = true;
            }
            if (fabsf(y) > configuration->gamepad.sensitivity) {
                buttons[y < 0.0f ? INPUT_BUTTON_DOWN : INPUT_BUTTON_UP].is = true;
            }
        }

        const float deadzone = configuration->gamepad.deadzone;
        const float range = configuration->gamepad.range;

        Input_Controller_Stick_t *sticks = controller->sticks;
        sticks[INPUT_CONTROLLER_STICK_LEFT] = _gamepad_stick(gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X], gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y], deadzone, range);
        sticks[INPUT_CONTROLLER_STICK_RIGHT] = _gamepad_stick(gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y], deadzone, range);

        Input_Controller_Triggers_t *triggers = &controller->triggers;
        triggers->left = _gamepad_trigger(gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER], deadzone, range);
        triggers->right = _gamepad_trigger(gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER], deadzone, range);
    }
}

static size_t _controllers_detect(Input_t *input)
{
    Input_Controller_t *controllers = input->state.controllers;

    size_t count = 0U;
    for (size_t i = 0; i < INPUT_CONTROLLERS_COUNT; ++i) { // Detect the available gamepads.
        Input_Controller_t *controller = &controllers[i];

        controller->id = i;

        bool available = glfwJoystickIsGamepad(i) == GLFW_TRUE;

        if (controller->available != available) {
            controller->available = available;
            if (available) {
                Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "gamepad #%d found (GUID `%s`, name `%s`)", i, glfwGetJoystickGUID(i), glfwGetGamepadName(i));
                ++count;
            } else {
                Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "gamepad #%d detached", i);
            }
        }
    }

    return count;
}

Input_t *Input_create(const Input_Configuration_t *configuration, GLFWwindow *window)
{
    int result = glfwUpdateGamepadMappings(configuration->mappings);
    if (result == GLFW_FALSE) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't update gamepad mappings");
        return NULL;
    }
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "input gamepad mappings updated");

    Input_t *input = malloc(sizeof(Input_t));
    if (!input) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't allocate input");
        return NULL;
    }

    *input = (Input_t){
            .configuration = *configuration,
            .window = window,
            .state = {
                .cursor =  {
                    .area = {
                        .x0 = (float)configuration->cursor.area.x,
                        .y0 = (float)configuration->cursor.area.y,
                        .x1 = (float)(configuration->cursor.area.x + configuration->cursor.area.width - 1),
                        .y1 = (float)(configuration->cursor.area.y + configuration->cursor.area.height - 1)
                    }
                }
            }
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "enabling sticky input mode");
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%s mouse cursor", configuration->cursor.hide ? "hiding" : "showing");
    glfwSetInputMode(window, GLFW_CURSOR, configuration->cursor.hide ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    size_t count = _controllers_detect(input);
    if (count == 0) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "no gamepads detected");
    } else {
        Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "%d gamepads detected", count);
    }

    return input;
}

void Input_destroy(Input_t *input)
{
    free(input);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "input freed");
}

static void _controllers_update(Input_t *input, float delta_time)
{
    (void)_controllers_detect(input);
}

static void _buttons_update(Input_t *input, float delta_time)
{
    // NOP.
}

static inline void _buttons_copy(Input_Button_t *target, size_t target_first, const Input_Button_t *source, size_t source_first, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        target[target_first + i] = source[source_first + i];
    }
}

static void _cursor_update(Input_t *input, float delta_time)
{
    Input_Controller_t *controllers = input->state.controllers;

    for (size_t i = 0; i < INPUT_CONTROLLERS_COUNT; ++i) {
        Input_Controller_t *controller = &controllers[i];

        if (controller->available && (controller->flags & INPUT_FLAG_CURSOR)) { // First available controller cursor-mapped will control the cursor.
            Input_Cursor_t *cursor = &input->state.cursor;

            const Input_Controller_Stick_t *stick = &controller->sticks[INPUT_CONTROLLER_STICK_RIGHT]; // Right stick for cursor movement.
            const float delta = input->configuration.cursor.speed * delta_time;
            _move_and_bound_cursor(cursor, cursor->x + stick->x * delta, cursor->y + stick->y * delta);

            _buttons_copy(cursor->buttons, Input_Cursor_Buttons_t_First, // Copy gamepad input as cursor buttons.
                controller->buttons, INPUT_CONTROLLER_BUTTON_Y,
                Input_Cursor_Buttons_t_CountOf);

            break;
        }
    }
}

bool Input_update(Input_t *input, float delta_time)
{
    _controllers_update(input, delta_time);
    _buttons_update(input, delta_time);
    _cursor_update(input, delta_time);

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

static void _buttons_process(Input_t *input)
{
    _buttons_sync(input->state.buttons, Input_Buttons_t_First, Input_Buttons_t_CountOf);

    Input_Controller_t *controllers = input->state.controllers;
    for (size_t i = 0; i < INPUT_CONTROLLERS_COUNT; ++i) {
        Input_Controller_t *controller = &controllers[i];

        if (!controller->available && !(controller->flags & INPUT_FLAG_EMULATED)) {
            continue;
        }

        if (controller->flags & INPUT_FLAG_EMULATED) {
            _buttons_copy(controller->buttons, Input_Controller_Buttons_t_First,
                input->state.buttons, Input_Buttons_t_First,
                Input_Controller_Buttons_t_CountOf);
        } else {
            _buttons_sync(controller->buttons, Input_Controller_Buttons_t_First, Input_Controller_Buttons_t_CountOf);
        }
    }
}

void Input_process(Input_t *input)
{
    static const Input_Handler_t handlers[Input_Handlers_t_CountOf] = {
        _keyboard_handler,
        _mouse_handler,
        _gamepad_handler
    };

    glfwPollEvents();

    for (size_t i = Input_Handlers_t_First; i <= Input_Handlers_t_Last; ++i) {
        handlers[i](input);
    }

    _buttons_process(input);

    const Input_Configuration_t *configuration = &input->configuration;
    if (configuration->keyboard.exit_key) {
        if (glfwGetKey(input->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "exit key pressed");
            glfwSetWindowShouldClose(input->window, true);
        }
    }
}

Input_Controller_t *Input_get_controller(Input_t *input, size_t id)
{
    if (id >= INPUT_CONTROLLERS_COUNT) {
        return NULL;
    }
    return &input->state.controllers[id];
}

Input_Cursor_t *Input_get_cursor(Input_t *input, size_t id)
{
    return &input->state.cursor;
}

bool Input_cursor_is_available(const Input_Cursor_t *cursor)
{
    return true; // TODO: should really check if available or emulated?
}

Input_Button_t Input_cursor_get_button(const Input_Cursor_t *cursor, Input_Cursor_Buttons_t button)
{
    return cursor->buttons[button];
}

Input_Position_t Input_cursor_get_position(const Input_Cursor_t *cursor)
{
    return (Input_Position_t){
            .x = (int)cursor->x, // FIXME: round values?
            .y = (int)cursor->y
        };
}

void Input_cursor_set_position(Input_Cursor_t *cursor, Input_Position_t position)
{
    cursor->x = (float)position.x + 0.5f; // Center on mid-pixel, as movements are float-based (to support dpad/stick)
    cursor->y = (float)position.y + 0.5f;
}

Input_Area_t Input_cursor_get_area(const Input_Cursor_t *cursor)
{
    return (Input_Area_t){ // FIXME: round values?
            .x = (int)cursor->area.x0,
            .y = (int)cursor->area.y0,
            .width = (size_t)(cursor->area.x1 - cursor->area.x0 + 1.0f),
            .height = (size_t)(cursor->area.y1 - cursor->area.y0 + 1.0f)
        };
}

void Input_cursor_set_area(Input_Cursor_t *cursor, Input_Area_t area)
{
    cursor->area.x0 = (float)area.x + 0.5f;
    cursor->area.y0 = (float)area.y + 0.5f;
    cursor->area.x1 = (float)area.x + (float)area.width - 0.5f; // Cursor area is right-bottom inclusive.
    cursor->area.y1 = (float)area.y + (float)area.height - 0.5f;
}

bool Input_controller_is_available(const Input_Controller_t *controller)
{
    return controller->available || (controller->flags & INPUT_FLAG_EMULATED);
}

int Input_controller_get_flags(const Input_Controller_t *controller)
{
    return controller->flags;
}

void Input_controller_set_flags(Input_Controller_t *controller, int flags)
{
    controller->flags = flags;
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
