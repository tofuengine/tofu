/*
 * Copyright (c) 2019-2020 by Marco Lizza (marco.lizza@gmail.com)
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
 **/

#include "input.h"

#include <libs/log.h>
#include <libs/stb.h>

#include <math.h>

#define LOG_CONTEXT "input"

typedef enum _System_Keys_t {
    SYSTEM_KEY_QUIT,
    SYSTEM_KEY_SWITCH,
    System_Keys_t_CountOf
} System_Keys_t;

typedef struct _Key_State_t {
    bool was, is;
    bool pressed, released;
} Key_State_t;

static int _system_key_ids[System_Keys_t_CountOf] = {
    GLFW_KEY_ESCAPE,
    GLFW_KEY_F1
};

static Key_State_t _system_keys[System_Keys_t_CountOf] = { 0 }; // TODO: move to the input structure.

static const uint8_t _mappings[] = {
#include "gamecontrollerdb.inc"
    0x00
};

static void _keyboard_handler(GLFWwindow *window, Input_State_t *state, const Input_Configuration_t *configuration)
{
    static const int keys[] = {
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

    Input_Button_t *buttons = state->buttons;

    for (int i = Input_Buttons_t_First; i <= INPUT_BUTTON_START; ++i) {
        Input_Button_t *button = &buttons[i];

        bool was_down = button->state.down;
        bool is_down = glfwGetKey(window, keys[i]) == GLFW_PRESS;

        if (!button->state.triggered) { // If not triggered use the current physical status.
            button->state.down = is_down;
            button->state.pressed = !was_down && is_down;
            button->state.released = was_down && !is_down;

            if (button->state.pressed && button->period > 0.0f) { // On press, track the trigger state and reset counter.
                button->state.triggered = true;
                button->time = 0.0f;
                Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "button #%d triggered, %.3fs %d %d %d", i, button->time, button->state.down, button->state.pressed, button->state.released);
            }
        } else
        if (!is_down) {
            button->state.down = false;
            button->state.pressed = false;
            button->state.released = was_down; // Track release is was previously down.

            button->state.triggered = false;
            Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "button #%d held for %.3fs %d %d %d", i, button->time, button->state.down, button->state.pressed, button->state.released);
        }
    }
}

static void _mouse_handler(GLFWwindow *window, Input_State_t *state, const Input_Configuration_t *configuration)
{
    static const int mouse_buttons[Input_Buttons_t_CountOf] = {
        GLFW_MOUSE_BUTTON_LEFT,
        GLFW_MOUSE_BUTTON_MIDDLE,
        GLFW_MOUSE_BUTTON_RIGHT
    };

    Input_Button_t *buttons = state->buttons;
    Input_Cursor_t *cursor = &state->cursor;

    for (int i = INPUT_BUTTON_MOUSE_LEFT; i <= INPUT_BUTTON_MOUSE_RIGHT; ++i) {
        Input_Button_t *button = &buttons[i];

        bool was_down = button->state.down;
        bool is_down = glfwGetMouseButton(window, mouse_buttons[i]) == GLFW_PRESS;

        button->state.down = is_down;
        button->state.pressed = !was_down && is_down;
        button->state.released = was_down && !is_down;
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    cursor->x = (float)x * configuration->scale;
    cursor->y = (float)y * configuration->scale;
}

// http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html
static inline Input_Stick_t _gamepad_stick(float x, float y, float deadzone_threshold)
{
    float angle = atan2f(y, x);
    float magnitude = sqrtf(x * x + y * y);
    if (magnitude < deadzone_threshold) {
        magnitude = 0.0f;
    } else
    if (magnitude > 1.0f) {
        magnitude = 1.0f;
    } else {
        magnitude = (magnitude - deadzone_threshold) / (1.0f - deadzone_threshold); // Rescale to ensure [0, 1] range.
    }
    return (Input_Stick_t){ .x = x, .y = y, .angle = angle, .magnitude = magnitude };
}

static inline float _gamepad_trigger(float magnitude, float deadzone_threshold)
{
    if (magnitude < deadzone_threshold) {
        magnitude = 0.0f;
    } else
    if (magnitude > 1.0f) {
        magnitude = 1.0f;
    }
    return (magnitude - deadzone_threshold) / (1.0f - deadzone_threshold);
}

static void _gamepad_handler(GLFWwindow *window, Input_State_t *state, const Input_Configuration_t *configuration)
{
    static const int gamepad_buttons[] = {
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
        GLFW_GAMEPAD_BUTTON_START
    };

    Input_Button_t *buttons = state->buttons;
    Input_Stick_t *sticks = state->sticks;
    Input_Triggers_t *triggers = &state->triggers;

    if (state->gamepad_id == -1) {
        return;
    }

    GLFWgamepadstate gamepad;
    int result = glfwGetGamepadState(state->gamepad_id, &gamepad);
    if (result == GLFW_TRUE) {
        if (configuration->emulate_dpad) {
            const float x = gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
            const float y = gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
            if (fabsf(x) > configuration->gamepad_sensitivity) {
                gamepad.buttons[x < 0.0f ? GLFW_GAMEPAD_BUTTON_DPAD_LEFT : GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] = GLFW_PRESS;
            }
            if (fabsf(y) > configuration->gamepad_sensitivity) {
                gamepad.buttons[y < 0.0f ? GLFW_GAMEPAD_BUTTON_DPAD_DOWN : GLFW_GAMEPAD_BUTTON_DPAD_UP] = GLFW_PRESS;
            }
        }

        for (int i = Input_Buttons_t_First; i <= INPUT_BUTTON_START; ++i) {
            Input_Button_t *button = &buttons[i];

            bool was_down = button->state.down;
            bool is_down = gamepad.buttons[gamepad_buttons[i]] == GLFW_PRESS;

            if (!button->state.triggered) { // If not triggered use the current physical status.
                button->state.down = is_down;
                button->state.pressed = !was_down && is_down;
                button->state.released = was_down && !is_down;

                if (button->state.pressed && button->period > 0.0f) { // On press, track the trigger state and reset counter.
                    button->state.triggered = true;
                    button->time = 0.0f;
                    Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "button #%d triggered, %.3fs %d %d %d", i, button->time, button->state.down, button->state.pressed, button->state.released);
                }
            } else
            if (!is_down) {
                button->state.down = false;
                button->state.pressed = false;
                button->state.released = was_down; // Track release is was previously down.

                button->state.triggered = false;
                Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "button #%d held for %.3fs %d %d %d", i, button->time, button->state.down, button->state.pressed, button->state.released);
            }
        }

        const float deadzone = configuration->gamepad_deadzone;

        sticks[INPUT_STICK_LEFT] = _gamepad_stick(gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X], gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y], deadzone);
        sticks[INPUT_STICK_RIGHT] = _gamepad_stick(gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y], deadzone);

        triggers->left = _gamepad_trigger(gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER], deadzone);
        triggers->right = _gamepad_trigger(gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER], deadzone);
    }
}

static void _switch(Input_t *input)
{
    Input_State_t *state = &input->state;
#ifndef __INPUT_SELECTION__
    Input_Handler_t *handlers = input->handlers;
#endif

    int gamepad_id = -1;
    for (int i = state->gamepad_id + 1; i < INPUT_GAMEPADS_COUNT; ++i) {
        if (input->gamepads[i]) {
            gamepad_id = i;
            break;
        }
    }

    state->gamepad_id = gamepad_id;
    if (gamepad_id == -1) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "keyboard/mouse input active");
    } else {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "gamepad #%d input active (`%s`)", gamepad_id, glfwGetGamepadName(gamepad_id));
    }

#ifndef __INPUT_SELECTION__
    handlers[INPUT_HANDLER_KEYBOARD] = gamepad_id == -1 ? _keyboard_handler : NULL;
    handlers[INPUT_HANDLER_MOUSE] = gamepad_id == -1 ? _mouse_handler : NULL;
    handlers[INPUT_HANDLER_GAMEPAD] = gamepad_id != -1 ? _gamepad_handler : NULL;
#endif
}

bool Input_initialize(Input_t *input, const Input_Configuration_t *configuration, GLFWwindow *window)
{
    int result = glfwUpdateGamepadMappings((const char *)_mappings);
    if (result == GLFW_FALSE) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't update gamepad mappings");
        return false;
    }

    // TODO: should perform a single "zeroing" call and the set the single fields?
    *input = (Input_t){
            .configuration = *configuration,
            .window = window,
            .time = 0.0,
            .state = (Input_State_t){
                    .gamepad_id = -1
                },
#ifdef __INPUT_SELECTION__
            .handlers = {
                    configuration->keyboard_enabled ? _keyboard_handler : NULL,
                    configuration->mouse_enabled ? _mouse_handler : NULL,
                    configuration->gamepad_enabled ? _gamepad_handler : NULL
                }
#else
            .handlers = { 0 }
#endif
        };

    size_t gamepads_count = 0U;
    for (int i = 0; i < INPUT_GAMEPADS_COUNT; ++i) { // Detect the available gamepads.
        input->gamepads[i] = glfwJoystickIsGamepad(i) == GLFW_TRUE;
        if (input->gamepads[i]) {
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "gamepad #%d found (GUID `%s`, name `%s`)", i, glfwGetJoystickGUID(i), glfwGetGamepadName(i));
            ++gamepads_count;
        }
    }
    if (gamepads_count == 0) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "no gamepads detected");
    } else {
        Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "%d gamepads detected", gamepads_count);
    }

    _switch(input);

    return true;
}

void Input_terminate(Input_t *input)
{
}

void Input_update(Input_t *input, float delta_time)
{
    input->time += delta_time;

    Input_State_t *state = &input->state;
    Input_Button_t *buttons = state->buttons;

    for (int i = Input_Buttons_t_First; i <= Input_Buttons_t_Last; ++i) {
        Input_Button_t *button = &buttons[i];

        if (!button->state.triggered) {
            continue;
        }

        button->state.pressed = false; // Clear the flags, will be eventually updated.
        button->state.released = false;

        button->time += delta_time;

        while (button->time >= button->period) {
            Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "#%d %.3fs", i, button->time);
            button->time -= button->period;

            button->state.down = !button->state.down;
            button->state.pressed = button->state.down;
            button->state.released = !button->state.down;
            Log_write(LOG_LEVELS_TRACE, LOG_CONTEXT, "#%d %.3fs %d %d %d", i, button->time, button->state.down, button->state.pressed, button->state.released);
        }
    }

    if (input->configuration.emulate_mouse) {
        Input_Stick_t *sticks = state->sticks;
        Input_Cursor_t *cursor = &state->cursor;

        const float vx = sticks[INPUT_STICK_RIGHT].x * input->configuration.cursor_speed;
        const float vy = sticks[INPUT_STICK_RIGHT].y * input->configuration.cursor_speed;

        cursor->x += vx * delta_time;
        cursor->y += vy * delta_time;

        if (cursor->x < cursor->area.x0) {
            cursor->x = cursor->area.x0;
        }
        if (cursor->y < cursor->area.y0) {
            cursor->y = cursor->area.y0;
        }
        if (cursor->x > cursor->area.x1) {
            cursor->x = cursor->area.x1;
        }
        if (cursor->y > cursor->area.y1) {
            cursor->y = cursor->area.y1;
        }
    }
}

void Input_process(Input_t *input)
{
    glfwPollEvents();

    GLFWwindow *window = input->window;
    Input_State_t *state = &input->state;
    Input_Configuration_t *configuration = &input->configuration;

    for (int i = Input_Handlers_t_First; i <= Input_Handlers_t_Last; ++i) {
        if (!input->handlers[i]) {
            continue;
        }
        input->handlers[i](window, state, configuration);
    }

    for (int i = 0; i < System_Keys_t_CountOf; ++i) {
        Key_State_t *state = &_system_keys[i];
        state->was = state->is;
        state->is = glfwGetKey(input->window, _system_key_ids[i]) == GLFW_PRESS;
        state->pressed = !state->was && state->is;
        state->released = state->was && !state->is;
    }

    if (input->configuration.exit_key_enabled) {
        if (_system_keys[SYSTEM_KEY_QUIT].pressed) {
            Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "exit key pressed");
            glfwSetWindowShouldClose(input->window, true);
        }
    }

   if (_system_keys[SYSTEM_KEY_SWITCH].pressed) {
        Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "input switch key pressed");
        _switch(input);
    }
}

void Input_auto_repeat(Input_t *input, Input_Buttons_t id, float period)
{
    input->state.buttons[id] = (Input_Button_t){
            .period = period,
            .time = 0.0f
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "auto-repeat set to %.3fs for button #%d", period, id);
}
