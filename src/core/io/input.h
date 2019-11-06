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

typedef enum _Input_Keys_t {
    Input_Keys_t_First = 0,
    INPUT_KEY_UP = Input_Keys_t_First,
    INPUT_KEY_DOWN,
    INPUT_KEY_LEFT,
    INPUT_KEY_RIGHT,
    INPUT_KEY_LT,
    INPUT_KEY_RT,
    INPUT_KEY_Y,
    INPUT_KEY_X,
    INPUT_KEY_B,
    INPUT_KEY_A,
    INPUT_KEY_SELECT,
    INPUT_KEY_START,
    Input_Keys_t_Last = INPUT_KEY_START,
    Input_Keys_t_CountOf
} Input_Keys_t;

typedef struct _Input_Key_State_t { // TODO: use explicit masks?
    uint8_t down : 1;
    uint8_t pressed : 1;
    uint8_t released : 1;
    uint8_t : 5;
} Input_Key_State_t;

typedef struct _Input_Configuration_t {
    bool exit_key_enabled;
} Input_Configuration_t;

typedef struct _Input_t {
    Input_Configuration_t configuration;

    GLFWwindow *window;

    Input_Key_State_t keys_state[Input_Keys_t_CountOf];
} Input_t;

extern bool Input_initialize(Input_t *input, const Input_Configuration_t *configuration, GLFWwindow *window);
extern void Input_terminate(Input_t *input);

extern void Input_process(Input_t *input, float delta_time);

#endif  /* __INPUT_H__ */