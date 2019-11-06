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

#include "input.h"

bool Input_initialize(Input_t *input, const Input_Configuration_t *configuration, GLFWwindow *window)
{
    *input = (Input_t){
            .configuration = *configuration,
            .window = window
        };
    return true;
}

void Input_terminate(Input_t *input)
{
    *input = (Input_t){ 0 };
}

void Input_process(Input_t *input, float delta_time)
{
    static const int keys[Input_Keys_t_CountOf] = {
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
        GLFW_KEY_SPACE
    };

    glfwPollEvents();

    for (int i = 0; i < Input_Keys_t_CountOf; ++i) {
        Input_Key_State_t *key_state = &input->keys_state[i];
        bool was_down = key_state->down;
        bool is_down = glfwGetKey(input->window, keys[i]) == GLFW_PRESS;
/*
        if (key_state->auto_trigger > 0.0f) {
            while (key_state->accumulator >= key_state->auto_trigger) {
                key_state->accumulator -= key_state->auto_trigger;
                is_down = true;
            }
        }
*/
        key_state->down = is_down;
        key_state->pressed = !was_down && is_down;
        key_state->released = was_down && !is_down;
    }

    if (input->configuration.exit_key_enabled) {
        if (glfwGetKey(input->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(input->window, true);
        }
    }
}
