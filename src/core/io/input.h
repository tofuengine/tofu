/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
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
    INPUT_BUTTON_LT,
    INPUT_BUTTON_RT,
    INPUT_BUTTON_Y,
    INPUT_BUTTON_X,
    INPUT_BUTTON_B,
    INPUT_BUTTON_A,
    INPUT_BUTTON_SELECT,
    INPUT_BUTTON_START,
    INPUT_BUTTON_RESET,
    INPUT_BUTTON_MOUSE_LEFT,
    INPUT_BUTTON_MOUSE_MIDDLE,
    INPUT_BUTTON_MOUSE_RIGHT,
    Input_Buttons_t_Last = INPUT_BUTTON_MOUSE_RIGHT,
    Input_Buttons_t_CountOf
} Input_Buttons_t;

typedef struct _Input_Button_State_t {
    uint8_t down : 1;
    uint8_t pressed : 1;
    uint8_t released : 1;
    uint8_t triggered : 1;
    uint8_t : 4;
} Input_Button_State_t;

typedef struct _Input_Button_t {
    Input_Button_State_t state;
    float period;
    float time;
} Input_Button_t;

typedef struct _Input_Cursor_t {
    float x, y;
    float vx, vy;
    struct {
        int x0, y0, x1, y1;
    } area;
} Input_Cursor_t;

typedef enum _Input_Handlers_t {
    Input_Handlers_t_First = 0,
    INPUT_HANDLER_KEYBOARD = Input_Handlers_t_First,
    INPUT_HANDLER_GAMEPAD,
    INPUT_HANDLER_MOUSE,
    Input_Handlers_t_Last = INPUT_HANDLER_MOUSE,
    Input_Handlers_t_CountOf
} Input_Handlers_t;

typedef struct _Input_Configuration_t {
    bool exit_key_enabled;
    bool use_keyboard;
    bool use_gamepad;
    bool use_mouse;
    float scale; // Refers to the screen-to-canvas scaling factor.
    // TODO: key-remapping?
} Input_Configuration_t;

typedef void (*Input_Handler_t)(GLFWwindow *window, Input_Button_t buttons[Input_Buttons_t_CountOf], Input_Cursor_t *cursor, const Input_Configuration_t *configuration);

typedef struct _Input_t {
    Input_Configuration_t configuration;

    GLFWwindow *window;

    double time;

    Input_Button_t buttons[Input_Buttons_t_CountOf];
    Input_Cursor_t cursor;

    Input_Handler_t handlers[Input_Handlers_t_CountOf];
} Input_t;

extern bool Input_initialize(Input_t *input, const Input_Configuration_t *configuration, GLFWwindow *window);
extern void Input_terminate(Input_t *input);

extern void Input_update(Input_t *input, float delta_time);
extern void Input_process(Input_t *input);

extern void Input_auto_repeat(Input_t *input, Input_Buttons_t id, float period);

extern bool Input_configure(Input_t *input, const char *mappings);

#endif  /* __INPUT_H__ */