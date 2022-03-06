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

typedef enum Input_Buttons_e {
    Input_Buttons_t_First = 0,
    INPUT_BUTTON_UP = Input_Buttons_t_First,
    INPUT_BUTTON_DOWN,
    INPUT_BUTTON_LEFT,
    INPUT_BUTTON_RIGHT,
    INPUT_BUTTON_LB, // Bumper.
    INPUT_BUTTON_RB,
    INPUT_BUTTON_LT, // Thumb.
    INPUT_BUTTON_RT,
    INPUT_BUTTON_Y,
    INPUT_BUTTON_X,
    INPUT_BUTTON_B,
    INPUT_BUTTON_A,
    INPUT_BUTTON_SELECT,
    INPUT_BUTTON_START,
    Input_Buttons_t_Last = INPUT_BUTTON_START,
    Input_Buttons_t_CountOf
} Input_Buttons_t;

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

#define INPUT_CONTROLLERS_COUNT (GLFW_JOYSTICK_LAST + 1)

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
        bool exit_key;
    } keyboard;
    struct {
        bool hide;
        float speed;
        float scale;
    } cursor;
    struct {
        float sensitivity;
        float deadzone; // TODO: what is anti-deadzone?
        float range;
    } gamepad;
} Input_Configuration_t;

#define INPUT_FLAG_NONE         0
#define INPUT_FLAG_EMULATED     1
#define INPUT_FLAG_DPAD         2
#define INPUT_FLAG_CURSOR       4
#define INPUT_FLAG_ALL          (INPUT_FLAG_EMULATED | INPUT_FLAG_DPAD | INPUT_FLAG_CURSOR)

typedef struct Input_Controller_s {
    size_t id;
    bool available;
    int flags;
    Input_Button_t buttons[Input_Buttons_t_CountOf];
    Input_Controller_Stick_t sticks[Input_Controller_Sticks_t_CountOf];
    Input_Controller_Triggers_t triggers;
} Input_Controller_t;

typedef struct Input_Cursor_s {
    Input_Button_t buttons[Input_Cursor_Buttons_t_CountOf];
    float x, y; // FIXME: use `Input_Position_t` datatype?
    struct {
        float x0, y0;
        float x1, y1;
    } area;
} Input_Cursor_t;

typedef struct Input_s {
    Input_Configuration_t configuration;

    GLFWwindow *window;

    struct {
        Input_Controller_t controllers[INPUT_CONTROLLERS_COUNT];
        Input_Cursor_t cursor;
        Input_Button_t buttons[Input_Buttons_t_CountOf];
    } state;
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

extern Input_Controller_t *Input_get_controller(Input_t *input, size_t id);
extern Input_Cursor_t *Input_get_cursor(Input_t *input, size_t id);

extern bool Input_cursor_is_available(const Input_Cursor_t *cursor);
extern Input_Button_t Input_cursor_get_button(const Input_Cursor_t *cursor, Input_Cursor_Buttons_t button);
extern Input_Position_t Input_cursor_get_position(const Input_Cursor_t *cursor);
extern void Input_cursor_set_position(Input_Cursor_t *cursor, Input_Position_t position);
extern Input_Area_t Input_cursor_get_area(const Input_Cursor_t *cursor);
extern void Input_cursor_set_area(Input_Cursor_t *cursor, Input_Area_t area);

extern bool Input_controller_is_available(const Input_Controller_t *controller);
extern Input_Button_t Input_controller_get_button(const Input_Controller_t *controller, Input_Controller_Buttons_t button);
extern Input_Controller_Triggers_t Input_controller_get_triggers(const Input_Controller_t *controller);
extern Input_Controller_Stick_t Input_controller_get_stick(const Input_Controller_t *controller, Input_Controller_Sticks_t stick);

#endif  /* __SYSTEMS_INPUT_H__ */
