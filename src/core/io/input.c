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

#define LOG_CONTEXT "input"

#define STICK_THRESHOLD 0.5f

static const uint8_t _mappings[] = {
#include "gamecontrollerdb.inc"
    0x00
};

static void _keyboard_handler(GLFWwindow *window, Input_Button_t buttons[Input_Buttons_t_CountOf], Input_Cursor_t *cursor, const Input_Configuration_t *configuration)
{
    static const int keys[] = {
        GLFW_KEY_UP,
        GLFW_KEY_DOWN,
        GLFW_KEY_LEFT,
        GLFW_KEY_RIGHT,
        GLFW_KEY_LEFT_CONTROL,
        GLFW_KEY_RIGHT_CONTROL,
        GLFW_KEY_Z,
        GLFW_KEY_S,
        GLFW_KEY_X,
        GLFW_KEY_D,
        GLFW_KEY_ENTER,
        GLFW_KEY_SPACE,
        GLFW_KEY_ESCAPE
    };

    for (int i = Input_Buttons_t_First; i <= INPUT_BUTTON_RESET; ++i) {
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

static void _gamepad_handler(GLFWwindow *window, Input_Button_t buttons[Input_Buttons_t_CountOf], Input_Cursor_t *cursor, const Input_Configuration_t *configuration)
{
    static const int gamepad_buttons[] = {
        GLFW_GAMEPAD_BUTTON_DPAD_UP,
        GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
        GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
        GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
        GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
        GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
        GLFW_GAMEPAD_BUTTON_Y,
        GLFW_GAMEPAD_BUTTON_X,
        GLFW_GAMEPAD_BUTTON_B,
        GLFW_GAMEPAD_BUTTON_A,
        GLFW_GAMEPAD_BUTTON_BACK,
        GLFW_GAMEPAD_BUTTON_START,
        GLFW_GAMEPAD_BUTTON_GUIDE
    };

    GLFWgamepadstate state;
    int result = glfwGetGamepadState(GLFW_JOYSTICK_1, &state);
    if (result == GLFW_TRUE) {
        // Simulate D-PAD with left stick.
        if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -STICK_THRESHOLD) {
            state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] = GLFW_PRESS;
        } else
        if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > STICK_THRESHOLD) {
            state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] = GLFW_PRESS;
        }
        if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -STICK_THRESHOLD) {
            state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] = GLFW_PRESS;
        } else
        if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > STICK_THRESHOLD) {
            state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] = GLFW_PRESS;
        }

        // Simulate mouse with right stick.
        cursor->vx = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
        cursor->vy = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];

        for (int i = Input_Buttons_t_First; i <= INPUT_BUTTON_RESET; ++i) {
            Input_Button_t *button = &buttons[i];

            bool was_down = button->state.down;
            bool is_down = state.buttons[gamepad_buttons[i]] == GLFW_PRESS;

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
}

static void _mouse_handler(GLFWwindow *window, Input_Button_t buttons[Input_Buttons_t_CountOf], Input_Cursor_t *cursor, const Input_Configuration_t *configuration)
{
    static const int mouse_buttons[Input_Buttons_t_CountOf] = {
        GLFW_MOUSE_BUTTON_LEFT,
        GLFW_MOUSE_BUTTON_MIDDLE,
        GLFW_MOUSE_BUTTON_RIGHT
    };

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

bool Input_initialize(Input_t *input, const Input_Configuration_t *configuration, GLFWwindow *window)
{
    // TODO: should perform a single "zeroing" call and the set the single fields?

    *input = (Input_t){
            .configuration = *configuration,
            .window = window,
            .time = 0
        };

    input->handlers[INPUT_HANDLER_KEYBOARD] = configuration->use_keyboard ? _keyboard_handler : NULL;
    input->handlers[INPUT_HANDLER_GAMEPAD] = configuration->use_gamepad ? _gamepad_handler : NULL;
    input->handlers[INPUT_HANDLER_MOUSE] = configuration->use_mouse ? _mouse_handler : NULL;

    return Input_configure(input, (const char *)_mappings);
}

void Input_terminate(Input_t *input)
{
}

void Input_update(Input_t *input, float delta_time)
{
    input->time += delta_time;

    Input_Button_t *buttons = input->buttons;
    Input_Cursor_t *cursor = &input->cursor;

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

    cursor->x += cursor->vx * delta_time; // TODO: we need to put a speed factor (configurable).
    cursor->y += cursor->vy * delta_time;
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
    cursor->vx = 0.0f; // TODO: add dampening?
    cursor->vy = 0.0f;
}

void Input_process(Input_t *input)
{
    glfwPollEvents();

    GLFWwindow *window = input->window;
    Input_Button_t *buttons = input->buttons;
    Input_Cursor_t *cursor = &input->cursor;
    Input_Configuration_t *configuration = &input->configuration;

    for (int i = Input_Handlers_t_First; i <= Input_Handlers_t_Last; ++i) {
        if (!input->handlers[i]) {
            continue;
        }
        input->handlers[i](window, buttons, cursor, configuration);
    }

    if (input->configuration.exit_key_enabled) {
        if (glfwGetKey(input->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(input->window, true);
        }
    }
}

void Input_auto_repeat(Input_t *input, Input_Buttons_t id, float period)
{
    input->buttons[id] = (Input_Button_t){
            .period = period,
            .time = 0.0f
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "auto-repeat set to %.3fs for button #%d", period, id);
}

bool Input_configure(Input_t *input, const char *mappings)
{
    int result = glfwUpdateGamepadMappings(mappings);
    if (result == GLFW_FALSE) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't update gamepad mappings");
        return false;
    }
    return true;
}