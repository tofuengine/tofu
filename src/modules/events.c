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

#include "events.h"

#include "../environment.h"
#include "../log.h"

#include <string.h>

const char events_wren[] =
    "foreign class Environment {\n"
    "\n"
    "    foreign static fps\n"
    "    foreign static quit()\n"
    "\n"
    "}\n"
    "\n"
    "foreign class Input {\n"
    "\n"
    "    static up { 0 }\n"
    "    static down { 1 }\n"
    "    static left { 2 }\n"
    "    static right { 3 }\n"
    "    static y { 4 }\n"
    "    static x { 5 }\n"
    "    static b { 6 }\n"
    "    static a { 7 }\n"
    "    static select { 8 }\n"
    "    static start { 9 }\n"
    "\n"
    "    foreign static isKeyDown(key)\n"
    "    foreign static isKeyUp(key)\n"
    "    foreign static isKeyPressed(key)\n"
    "    foreign static isKeyReleased(key)\n"
    "\n"
    "}\n"
;

void events_input_iskeydown_call1(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);

    bool is_down = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].down : false;
    wrenSetSlotBool(vm, 0, is_down);
}

void events_input_iskeyup_call1(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);

    bool is_down = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].down : false;
    wrenSetSlotBool(vm, 0, !is_down);
}

void events_input_iskeypressed_call1(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);

    bool is_pressed = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].pressed : false;
    wrenSetSlotBool(vm, 0, is_pressed);
}

void events_input_iskeyreleased_call1(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);

    bool is_released = (key >= Display_Keys_t_First && key <= Display_Keys_t_Last) ? environment->display->keys_state[key].released : false;
    wrenSetSlotBool(vm, 0, is_released);
}

void events_environment_fps_get(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    wrenSetSlotDouble(vm, 0, environment->fps);
}

void events_environment_quit_call0(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    environment->should_close = true;
}
