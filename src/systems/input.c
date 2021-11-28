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

#include "input.h"

#include <libs/log.h>
#include <libs/stb.h>

#include <math.h>

#define LOG_CONTEXT "input"

typedef void (*Input_Handler_t)(Input_t *input);

static void _default_handler(Input_t *input)
{
    Input_Button_t *buttons = input->buttons;
    for (int i = Input_Buttons_t_First; i <= Input_Buttons_t_Last; ++i) { // Store current state and clear it.
        Input_Button_t *button = &buttons[i];
        button->state.was = button->state.is;
        button->state.is = false;
    }
}

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
        GLFW_KEY_SPACE,
#ifdef __GRAPHICS_CAPTURE_SUPPORT__
        GLFW_KEY_F11,
        GLFW_KEY_F12,
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */
        GLFW_KEY_ESCAPE
    };

    if (!(input->state.mode & INPUT_MODE_KEYBOARD)) {
        return;
    }

    GLFWwindow *window = input->window;
    Input_Button_t *buttons = input->buttons;
    for (int i = Input_Buttons_t_First; i <= Input_Buttons_t_Last; ++i) {
        Input_Button_t *button = &buttons[i];
        button->state.is |= glfwGetKey(window, keys[i]) == GLFW_PRESS;
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
    static const int mouse_buttons[Input_Buttons_t_CountOf] = {
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        GLFW_MOUSE_BUTTON_MIDDLE,
        GLFW_MOUSE_BUTTON_RIGHT,
        GLFW_MOUSE_BUTTON_LEFT,
        -1,
        -1,
#ifdef __GRAPHICS_CAPTURE_SUPPORT__
        -1,
        -1,
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */
        -1
    };

    if (!(input->state.mode & INPUT_MODE_MOUSE)) {
        return;
    }

    GLFWwindow *window = input->window;
    Input_Button_t *buttons = input->buttons;
    for (int i = Input_Buttons_t_First; i <= Input_Buttons_t_Last; ++i) {
        if (mouse_buttons[i] == -1) {
            continue;
        }
        Input_Button_t *button = &buttons[i];
        button->state.is |= glfwGetMouseButton(window, mouse_buttons[i]) == GLFW_PRESS;
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    const float scale = input->configuration.cursor.scale;
    Input_Cursor_t *cursor = &input->cursor;
    _move_and_bound_cursor(cursor, (float)x * scale + 0.5f, (float)y * scale + 0.5f);
}

// http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html
// http://blog.hypersect.com/interpreting-analog-sticks/
static inline Input_Stick_t _gamepad_stick(float x, float y, float deadzone, float range)
{
    const float magnitude = sqrtf(x * x + y * y);
    if (magnitude < deadzone) {
        return (Input_Stick_t){ .x = 0.0f, .y = 0.0f, .angle = 0.0f, .magnitude = 0.0f };
    } else { // Rescale to ensure [0, 1] range. Response curve is left to the final user.
        const float angle = atan2f(y, x);
        const float normalized_magnitude = fminf(1.0f, (magnitude - deadzone) / range);
        const float scale = normalized_magnitude / magnitude;
        return (Input_Stick_t){ .x = x * scale, .y = y * scale, .angle = angle, .magnitude = normalized_magnitude };
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
#ifdef __GRAPHICS_CAPTURE_SUPPORT__
        -1,
        -1,
#endif  /* __GRAPHICS_CAPTURE_SUPPORT__ */
        -1
    };

    Input_State_t *state = &input->state;

    if (!(state->mode & INPUT_MODE_GAMEPAD)) {
        return;
    }

    if (state->gamepad.id == -1) {
        return;
    }

    Input_Button_t *buttons = input->buttons;
    Input_Stick_t *sticks = input->sticks;
    Input_Triggers_t *triggers = &input->triggers;
    const Input_Configuration_t *configuration = &input->configuration;

    GLFWgamepadstate gamepad;
    int result = glfwGetGamepadState(state->gamepad.id, &gamepad);
    if (result == GLFW_FALSE) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "can't get gamepad #%d state", state->gamepad.id);
        return;
    }

    if (configuration->gamepad.emulate_dpad) {
        const float x = gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
        const float y = gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        if (fabsf(x) > configuration->gamepad.sensitivity) {
            buttons[x < 0.0f ? INPUT_BUTTON_LEFT : INPUT_BUTTON_RIGHT].state.is = true;
        }
        if (fabsf(y) > configuration->gamepad.sensitivity) {
            buttons[y < 0.0f ? INPUT_BUTTON_DOWN : INPUT_BUTTON_UP].state.is = true;
        }
    }

    for (int i = Input_Buttons_t_First; i <= Input_Buttons_t_Last; ++i) {
        if (gamepad_buttons[i] == -1) {
            continue;
        }
        Input_Button_t *button = &buttons[i];
        button->state.is |= gamepad.buttons[gamepad_buttons[i]] == GLFW_PRESS;
    }

    const float deadzone = configuration->gamepad.deadzone;
    const float range = configuration->gamepad.range;

    sticks[INPUT_STICK_LEFT] = _gamepad_stick(gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X], gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y], deadzone, range);
    sticks[INPUT_STICK_RIGHT] = _gamepad_stick(gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y], deadzone, range);

    triggers->left = _gamepad_trigger(gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER], deadzone, range);
    triggers->right = _gamepad_trigger(gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER], deadzone, range);
}

static void _gamepad_detect(Input_t *input)
{
    Input_State_t *state = &input->state;

    bool changed = false;

    size_t count = 0U;
    for (int i = 0; i < INPUT_GAMEPADS_COUNT; ++i) { // Detect the available gamepads.
        bool available = glfwJoystickIsGamepad(i) == GLFW_TRUE;
        if (state->gamepad.available[i] != available) {
            state->gamepad.available[i] = available;
            if (available) {
                Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "gamepad #%d found (GUID `%s`, name `%s`)", i, glfwGetJoystickGUID(i), glfwGetGamepadName(i));
                ++count;
            } else {
                Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "gamepad #%d detached", i);
            }

            changed = i == state->gamepad.id;
        }
    }

    state->gamepad.delta = (int)count - (int)state->gamepad.count;
    state->gamepad.count = count;

    if (changed) {
        int id = -1;
        for (int i = 0; i < INPUT_GAMEPADS_COUNT; ++i) { // Find and use the first available gamepad (no multiple input).
            if (state->gamepad.available[i]) {
                id = i;
                break;
            }
        }

        if (id == -1) {
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "keyboard/mouse input active");
        } else {
            Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "gamepad #%d input active (`%s`)", id, glfwGetGamepadName(id));
        }

        state->gamepad.id = id;
    }
}

static inline int _compile_mode(const Input_Configuration_t *configuration)
{
    int mode = INPUT_MODE_NONE;
    if (configuration->keyboard.enabled) {
        mode |= INPUT_MODE_KEYBOARD;
    }
    if (configuration->cursor.enabled) {
        mode |= INPUT_MODE_MOUSE;
    }
    if (configuration->gamepad.emulate_cursor) {
        mode |= INPUT_MODE_GAMEPAD;
    }
    return mode;
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
                .mode = _compile_mode(configuration),
                .gamepad = {
                    .id = -1,
                    .available = { 0 },
                    .count = 0,
                    .delta = 0
                }
            }
        };

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "enabling sticky-keys input mode");
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "%s mouse cursor", configuration->cursor.hide ? "hiding" : "showing");
    glfwSetInputMode(window, GLFW_CURSOR, configuration->cursor.hide ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    _gamepad_detect(input);
    if (input->state.gamepad.count == 0) {
        Log_write(LOG_LEVELS_WARNING, LOG_CONTEXT, "no gamepads detected");
    } else {
        Log_write(LOG_LEVELS_INFO, LOG_CONTEXT, "%d gamepads detected", input->state.gamepad.count);
    }

    return input;
}

void Input_destroy(Input_t *input)
{
    free(input);
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "input freed");
}

static void _gamepad_update(Input_t *input, float delta_time)
{
    Input_State_t *state = &input->state;

    if (!(state->mode & INPUT_MODE_GAMEPAD)) {
        return;
    }

    _gamepad_detect(input);
}

static void _buttons_update(Input_t *input, float delta_time)
{
    Input_State_t *state = &input->state;

    if (!(state->mode & INPUT_MODE_KEYPAD)) {
        return;
    }

    Input_Button_t *buttons = input->buttons;

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
}

static void _cursor_update(Input_t *input, float delta_time)
{
    Input_State_t *state = &input->state;

    if (!(state->mode & INPUT_MODE_MOUSEPAD)) { // Cursor state can be changed by either mouse or gamepad (emulated).
        return;
    }

    if (input->configuration.gamepad.emulate_cursor) {
        const Input_Stick_t *stick = &input->sticks[INPUT_STICK_RIGHT];
        const float delta = input->configuration.cursor.speed * delta_time;

        Input_Cursor_t *cursor = &input->cursor;
        _move_and_bound_cursor(cursor, cursor->x + stick->x * delta, cursor->y + stick->y * delta);
    }
}

bool Input_update(Input_t *input, float delta_time)
{
    _gamepad_update(input, delta_time);
    _buttons_update(input, delta_time);
    _cursor_update(input, delta_time);

    return true;
}

static void _buttons_process(Input_t *input)
{
    Input_State_t *state = &input->state;

    if (!(state->mode & INPUT_MODE_KEYPAD)) { // Buttons' state can be changed by either keyboard or gamepad (emulated).
        return;
    }

    Input_Button_t *buttons = input->buttons;
    for (int i = Input_Buttons_t_First; i <= Input_Buttons_t_Last; ++i) {
        Input_Button_t *button = &buttons[i];

        bool was_down = button->state.was;
        bool is_down = button->state.is;

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

void Input_process(Input_t *input)
{
    static const Input_Handler_t handlers[Input_Handlers_t_CountOf] = {
        _default_handler,
        _keyboard_handler,
        _mouse_handler,
        _gamepad_handler
    };

    glfwPollEvents();

    for (int i = Input_Handlers_t_First; i <= Input_Handlers_t_Last; ++i) {
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

const Input_State_t *Input_get_state(const Input_t *input)
{
    return &input->state;
}

void Input_set_cursor_position(Input_t *input, int x, int y)
{
    input->cursor.x = (float)x + 0.5f; // Center on mid-pixel, as movements are float-based (to support dpad/stick)
    input->cursor.y = (float)y + 0.5f;
}

void Input_set_cursor_area(Input_t *input, int x, int y, size_t width, size_t height)
{
    input->cursor.area.x0 = (float)x + 0.5f;
    input->cursor.area.y0 = (float)y + 0.5f;
    input->cursor.area.x1 = (float)x + (float)width - 0.5f; // Cursor area is right-bottom inclusive.
    input->cursor.area.y1 = (float)y + (float)height - 0.5f;
}

void Input_set_auto_repeat(Input_t *input, Input_Buttons_t button, float period)
{
    input->buttons[button] = (Input_Button_t){
            .period = period,
            .time = 0.0f
        };
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "auto-repeat set to %.3fs for button #%d", period, button);
}

void Input_set_mode(Input_t *input, int mode)
{
    input->state.mode = mode;
    Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "mode set to 0x%04x", mode);
}

const Input_Button_State_t *Input_get_button(const Input_t *input, Input_Buttons_t button)
{
    return &input->buttons[button].state;
}

const Input_Cursor_t *Input_get_cursor(const Input_t *input)
{
    return &input->cursor;
}

const Input_Triggers_t *Input_get_triggers(const Input_t *input)
{
    return &input->triggers;
}

const Input_Stick_t *Input_get_stick(const Input_t *input, Input_Sticks_t stick)
{
    return &input->sticks[stick];
}

float Input_get_auto_repeat(const Input_t *input, Input_Buttons_t button)
{
    return input->buttons[button].period;
}

int Input_get_mode(const Input_t *input)
{
    return input->state.mode;
}
