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

#ifndef __INPUT_H__
#define __INPUT_H__

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdbool.h>
#include <stdint.h>

typedef enum _Input_Buttons_t {
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
    INPUT_BUTTON_QUIT,
    Input_Buttons_t_Last = INPUT_BUTTON_QUIT,
    Input_Buttons_t_CountOf
} Input_Buttons_t;

typedef struct _Input_Button_State_t {
    uint8_t was : 1; // Transient buffer.
    uint8_t is : 1; // Ditto.
    uint8_t down : 1;
    uint8_t pressed : 1;
    uint8_t released : 1;
    uint8_t triggered : 1;
    uint8_t : 2;
} Input_Button_State_t;

typedef struct _Input_Button_t {
    Input_Button_State_t state;
    float period;
    float time;
} Input_Button_t;

typedef struct _Input_Cursor_t {
    float x, y;
    struct {
        float x0, y0;
        float x1, y1;
    } area;
} Input_Cursor_t;

typedef enum _Input_Sticks_t {
    Input_Sticks_t_First = 0,
    INPUT_STICK_LEFT = Input_Sticks_t_First,
    INPUT_STICK_RIGHT,
    Input_Sticks_t_Last = INPUT_STICK_RIGHT,
    Input_Sticks_t_CountOf
} Input_Sticks_t;

typedef struct _Input_Stick_t {
    float x, y;
    float angle, magnitude;
} Input_Stick_t;

typedef struct _Input_Triggers_t {
    float left, right;
} Input_Triggers_t;

#define INPUT_GAMEPADS_COUNT    (GLFW_JOYSTICK_LAST + 1)

typedef enum _Input_Handlers_t {
    Input_Handlers_t_First = 0,
    INPUT_HANDLE_DEFAULT = Input_Handlers_t_First,
    INPUT_HANDLER_KEYBOARD,
    INPUT_HANDLER_MOUSE,
    INPUT_HANDLER_GAMEPAD,
    Input_Handlers_t_Last = INPUT_HANDLER_GAMEPAD,
    Input_Handlers_t_CountOf
} Input_Handlers_t;

typedef struct _Input_Configuration_t {
    const char *mappings;
    struct {
        bool enabled;
        bool exit_key;
    } keyboard;
    struct {
        bool enabled;
        bool hide;
        float speed;
        float scale;
    } cursor;
    struct {
        bool enabled;
        float sensitivity;
        float deadzone; // TODO: what is anti-deadzone?
        float range;
        bool emulate_dpad;
        bool emulate_cursor;
    } gamepad;
} Input_Configuration_t;

#define INPUT_MODE_NONE     0
#define INPUT_MODE_KEYBOARD 1
#define INPUT_MODE_MOUSE    2
#define INPUT_MODE_GAMEPAD  4
#define INPUT_MODE_KEYMOUSE (INPUT_MODE_KEYBOARD | INPUT_MODE_MOUSE)
#define INPUT_MODE_ALL      (INPUT_MODE_KEYBOARD | INPUT_MODE_MOUSE | INPUT_MODE_GAMEPAD)

typedef struct _Input_t {
    Input_Configuration_t configuration;

    GLFWwindow *window;

    int mode;
    bool gamepads[INPUT_GAMEPADS_COUNT];
    int gamepad_id;
    Input_Button_t buttons[Input_Buttons_t_CountOf];
    Input_Cursor_t cursor;
    Input_Stick_t sticks[Input_Sticks_t_CountOf];
    Input_Triggers_t triggers;

    double time;
} Input_t;

extern Input_t *Input_create(const Input_Configuration_t *configuration, GLFWwindow *window);
extern void Input_destroy(Input_t *input);

extern void Input_update(Input_t *input, float delta_time);
extern void Input_process(Input_t *input);

extern void Input_set_cursor_position(Input_t *input, float x, float y);
extern void Input_set_cursor_area(Input_t *input, float x0, float y0, float x1, float y1);
extern void Input_set_auto_repeat(Input_t *input, Input_Buttons_t button, float period);
extern void Input_set_mode(Input_t *input, int mode);

extern const Input_Button_State_t *Input_get_button(const Input_t *input, Input_Buttons_t button);
extern const Input_Cursor_t *Input_get_cursor(const Input_t *input);
extern const Input_Triggers_t *Input_get_triggers(const Input_t *input);
extern const Input_Stick_t *Input_get_stick(const Input_t *input, Input_Sticks_t stick);
extern float Input_get_auto_repeat(const Input_t *input, Input_Buttons_t button);
extern int Input_get_mode(const Input_t *input);

#endif  /* __INPUT_H__ */
