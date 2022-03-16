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

#ifndef __SYSTEMS_INPUT_H__
#define __SYSTEMS_INPUT_H__

#include <config.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdbool.h>
#include <stdint.h>

typedef enum Input_Keyboard_Buttons_e {
    Input_Keyboard_Buttons_t_First = 0,
    INPUT_KEYBOARD_BUTTON_1 = Input_Keyboard_Buttons_t_First,
    INPUT_KEYBOARD_BUTTON_2,
    INPUT_KEYBOARD_BUTTON_3,
    INPUT_KEYBOARD_BUTTON_4,
    INPUT_KEYBOARD_BUTTON_5,
    INPUT_KEYBOARD_BUTTON_6,
    INPUT_KEYBOARD_BUTTON_7,
    INPUT_KEYBOARD_BUTTON_8,
    INPUT_KEYBOARD_BUTTON_9,
    INPUT_KEYBOARD_BUTTON_0,
    INPUT_KEYBOARD_BUTTON_Q,
    INPUT_KEYBOARD_BUTTON_W,
    INPUT_KEYBOARD_BUTTON_E,
    INPUT_KEYBOARD_BUTTON_R,
    INPUT_KEYBOARD_BUTTON_T,
    INPUT_KEYBOARD_BUTTON_Y,
    INPUT_KEYBOARD_BUTTON_U,
    INPUT_KEYBOARD_BUTTON_I,
    INPUT_KEYBOARD_BUTTON_O,
    INPUT_KEYBOARD_BUTTON_P,
    INPUT_KEYBOARD_BUTTON_A,
    INPUT_KEYBOARD_BUTTON_S,
    INPUT_KEYBOARD_BUTTON_D,
    INPUT_KEYBOARD_BUTTON_F,
    INPUT_KEYBOARD_BUTTON_G,
    INPUT_KEYBOARD_BUTTON_H,
    INPUT_KEYBOARD_BUTTON_J,
    INPUT_KEYBOARD_BUTTON_K,
    INPUT_KEYBOARD_BUTTON_L,
    INPUT_KEYBOARD_BUTTON_Z,
    INPUT_KEYBOARD_BUTTON_X,
    INPUT_KEYBOARD_BUTTON_C,
    INPUT_KEYBOARD_BUTTON_V,
    INPUT_KEYBOARD_BUTTON_B,
    INPUT_KEYBOARD_BUTTON_N,
    INPUT_KEYBOARD_BUTTON_M,
    INPUT_KEYBOARD_BUTTON_UP,
    INPUT_KEYBOARD_BUTTON_DOWN,
    INPUT_KEYBOARD_BUTTON_LEFT,
    INPUT_KEYBOARD_BUTTON_RIGHT,
    INPUT_KEYBOARD_BUTTON_ENTER,
    INPUT_KEYBOARD_BUTTON_SPACE,
    Input_Keyboard_Buttons_t_Last = INPUT_KEYBOARD_BUTTON_SPACE,
    Input_Keyboard_Buttons_t_CountOf
} Input_Keyboard_Buttons_t;

typedef enum Input_Controller_Buttons_e {
    Input_Controller_Buttons_t_First = 0,
    INPUT_CONTROLLER_BUTTON_UP = Input_Controller_Buttons_t_First,
    INPUT_CONTROLLER_BUTTON_DOWN,
    INPUT_CONTROLLER_BUTTON_LEFT,
    INPUT_CONTROLLER_BUTTON_RIGHT,
    INPUT_CONTROLLER_BUTTON_LB, // Bumper.
    INPUT_CONTROLLER_BUTTON_RB,
    INPUT_CONTROLLER_BUTTON_LT, // Thumb.
    INPUT_CONTROLLER_BUTTON_RT,
    INPUT_CONTROLLER_BUTTON_Y,
    INPUT_CONTROLLER_BUTTON_X,
    INPUT_CONTROLLER_BUTTON_B,
    INPUT_CONTROLLER_BUTTON_A,
    INPUT_CONTROLLER_BUTTON_SELECT,
    INPUT_CONTROLLER_BUTTON_START,
    Input_Controller_Buttons_t_Last = INPUT_CONTROLLER_BUTTON_START,
    Input_Controller_Buttons_t_CountOf
} Input_Controller_Buttons_t;

typedef enum Input_Cursor_Buttons_e {
    Input_Cursor_Buttons_t_First = 0,
    INPUT_CURSOR_BUTTON_LEFT = Input_Cursor_Buttons_t_First,
    INPUT_CURSOR_BUTTON_RIGHT,
    INPUT_CURSOR_BUTTON_MIDDLE,
    Input_Cursor_Buttons_t_Last = INPUT_CURSOR_BUTTON_MIDDLE,
    Input_Cursor_Buttons_t_CountOf
} Input_Cursor_Buttons_t;

typedef struct Input_Button_s {
    uint8_t was : 1; // Transient buffer.
    uint8_t is : 1; // Ditto.
    uint8_t down : 1;
    uint8_t pressed : 1;
    uint8_t released : 1;
    uint8_t : 3;
} Input_Button_t;

typedef enum Input_Controller_Sticks_e {
    Input_Controller_Sticks_t_First = 0,
    INPUT_CONTROLLER_STICK_LEFT = Input_Controller_Sticks_t_First,
    INPUT_CONTROLLER_STICK_RIGHT,
    Input_Controller_Sticks_t_Last = INPUT_CONTROLLER_STICK_RIGHT,
    Input_Controller_Sticks_t_CountOf
} Input_Controller_Sticks_t;

typedef struct Input_Controller_Stick_s {
    float x, y;
    float angle, magnitude;
} Input_Controller_Stick_t;

typedef struct Input_Controller_Triggers_s {
    float left, right;
} Input_Controller_Triggers_t;

#define INPUT_CONTROLLERS_COUNT 4

typedef enum Input_Handlers_e {
    Input_Handlers_t_First = 0,
    INPUT_HANDLER_KEYBOARD = Input_Handlers_t_First,
    INPUT_HANDLER_MOUSE,
    INPUT_HANDLER_GAMEPAD,
    Input_Handlers_t_Last = INPUT_HANDLER_GAMEPAD,
    Input_Handlers_t_CountOf
} Input_Handlers_t;

typedef struct Input_Configuration_s {
    const char *mappings;
    struct {
        struct {
            size_t width, height;
        } physical;
        struct {
            size_t width, height;
        } virtual;
    } screen;
    struct {
        bool exit_key;
    } keyboard;
    struct {
        bool enabled;
        bool hide;
        float speed;
    } cursor;
    struct {
        float deadzone; // TODO: what is anti-deadzone?
        float range;
    } controller;
} Input_Configuration_t;

typedef struct Input_Keyboard_s {
    Input_Button_t buttons[Input_Keyboard_Buttons_t_CountOf];
} Input_Keyboard_t;

typedef struct Input_Cursor_s {
    bool enabled;
    Input_Button_t buttons[Input_Cursor_Buttons_t_CountOf];
    float x, y; // FIXME: use `Input_Position_t` datatype?
    struct {
        float x, y;
    } scale;
    struct {
        float x0, y0;
        float x1, y1;
    } area;
} Input_Cursor_t;

typedef struct Input_Controller_s {
    size_t id;
    int jid; // When greater than `-1`, the controller is AVAILABLE.
    Input_Button_t buttons[Input_Controller_Buttons_t_CountOf];
    Input_Controller_Stick_t sticks[Input_Controller_Sticks_t_CountOf];
    Input_Controller_Triggers_t triggers;
} Input_Controller_t;

typedef struct Input_s {
    Input_Configuration_t configuration;

    GLFWwindow *window;

    struct {
        Input_Keyboard_t keyboard;
        Input_Cursor_t cursor;
        Input_Controller_t controllers[INPUT_CONTROLLERS_COUNT];
        size_t controllers_count;
        bool used_gamepads[GLFW_JOYSTICK_LAST + 1];
    } state;

    double age;
} Input_t;

typedef struct Input_Position_s {
    int x, y;
} Input_Position_t;

typedef struct Input_Area_s {
    int x, y;
    size_t width, height;
} Input_Area_t;

extern Input_t *Input_create(const Input_Configuration_t *configuration, GLFWwindow *window);
extern void Input_destroy(Input_t *input);

extern bool Input_update(Input_t *input, float delta_time);
extern void Input_process(Input_t *input);

extern Input_Keyboard_t *Input_get_keyboard(Input_t *input);
extern Input_Cursor_t *Input_get_cursor(Input_t *input);
extern Input_Controller_t *Input_get_controller(Input_t *input, size_t id);
extern size_t Input_get_controllers_count(const Input_t *input);

extern bool Input_keyboard_is_available(const Input_Keyboard_t *keyboard);
extern Input_Button_t Input_keyboard_get_button(const Input_Keyboard_t *keyboard, Input_Keyboard_Buttons_t button);

extern bool Input_cursor_is_available(const Input_Cursor_t *cursor);
extern Input_Button_t Input_cursor_get_button(const Input_Cursor_t *cursor, Input_Cursor_Buttons_t button);
extern Input_Position_t Input_cursor_get_position(const Input_Cursor_t *cursor);
extern void Input_cursor_set_position(Input_Cursor_t *cursor, Input_Position_t position);

extern bool Input_controller_is_available(const Input_Controller_t *controller);
extern Input_Button_t Input_controller_get_button(const Input_Controller_t *controller, Input_Controller_Buttons_t button);
extern Input_Controller_Triggers_t Input_controller_get_triggers(const Input_Controller_t *controller);
extern Input_Controller_Stick_t Input_controller_get_stick(const Input_Controller_t *controller, Input_Controller_Sticks_t stick);

#endif  /* __SYSTEMS_INPUT_H__ */
