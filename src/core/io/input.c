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

#include <libs/log.h>

bool Input_initialize(Input_t *input, const Input_Configuration_t *configuration, GLFWwindow *window)
{
    // TODO: should perform a single "zeroing" call and the set the single fields?

    *input = (Input_t){
            .configuration = *configuration,
            .window = window,
            .time = 0.0f
        };
    return true;
}

void Input_terminate(Input_t *input)
{
}

void Input_update(Input_t *input, float delta_time)
{
    input->time += delta_time;

    for (int i = 0; i < Input_Keys_t_CountOf; ++i) {
        Input_Key_t *key = &input->keys[i];

        if (!key->state.triggered) {
            continue;
        }

        key->state.pressed = false; // Consume both flags.
        key->state.released = false;

        key->time += delta_time;

        while (key->time >= key->period) {
            Log_write(LOG_LEVELS_TRACE, "<INPUT> #%d %.3fs", i, key->time);
            key->time -= key->period;

            key->state.down = !key->state.down;
            key->state.pressed = key->state.down;
            key->state.released = !key->state.down;
            Log_write(LOG_LEVELS_TRACE, "<INPUT> #%d %.3fs %d %d %d", i, key->time, key->state.down, key->state.pressed, key->state.released);
        }
    }

}

void Input_process(Input_t *input)
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
        Input_Key_t *key = &input->keys[i];

        bool was_down = key->state.down;
        bool is_down = glfwGetKey(input->window, keys[i]) == GLFW_PRESS;

        if (!key->state.triggered) { // If not triggered use the current physical status.
            key->state.down = is_down;
            key->state.pressed = !was_down && is_down;
            key->state.released = was_down && !is_down;

            if (key->state.pressed && key->period > 0.0f) { // On press, track the trigger state and reset counter.
                key->state.triggered = true;
                key->time = 0.0f;
            }
        } else
        if (!is_down) { // Key released, was triggered.
            key->state.down = false;
            key->state.pressed = false;
            key->state.released = was_down; // Track release is was previously down.

            key->state.triggered = false;
            Log_write(LOG_LEVELS_TRACE, "<INPUT> button #%d held for %.3fs %d %d %d", i, key->time, key->state.down, key->state.pressed, key->state.released);
        }
    }

    if (input->configuration.exit_key_enabled) {
        if (glfwGetKey(input->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(input->window, true);
        }
    }
}

void Input_auto_repeat(Input_t *input, Input_Keys_t id, float period)
{
    Input_Key_t *key = &input->keys[id];

    *key = (Input_Key_t){
            .period = period,
            .time = 0.0f
        };
}
