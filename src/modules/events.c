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

#include <raylib/raylib.h>
#include <string.h>

const char events_wren[] =
    "foreign class Environment {\n"
    "\n"
    "    foreign static quit()\n"
    "\n"
    "}\n"
    "\n"
    "foreign class Input {\n"
    "\n"
    "    static up { 265 }\n"
    "    static down { 264 }\n"
    "    static left { 263 }\n"
    "    static right { 262 }\n"
    "    static space { 32 }\n"
    "    static enter { 257 }\n"
    "    static escape { 256 }\n"
    "    static z { 90 }\n"
    "    static x { 88 }\n"
    "    static q { 81 }\n"
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
    // Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);
    bool is_down = IsKeyDown(key);
    wrenSetSlotBool(vm, 0, is_down == true);
}

void events_input_iskeyup_call1(WrenVM *vm)
{
    // Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);
    bool is_up = IsKeyUp(key);
    wrenSetSlotBool(vm, 0, is_up == true);
}

void events_input_iskeypressed_call1(WrenVM *vm)
{
    // Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);
    bool is_pressed = IsKeyPressed(key);
    wrenSetSlotBool(vm, 0, is_pressed == true);
}

void events_input_iskeyreleased_call1(WrenVM *vm)
{
    // Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    int key = (int)wrenGetSlotDouble(vm, 1);
    bool is_released = IsKeyReleased(key);
    wrenSetSlotBool(vm, 0, is_released == true);
}

void events_environment_quit_call0(WrenVM *vm)
{
    Environment_t *environment = (Environment_t *)wrenGetUserData(vm);

    environment->should_close = true;
}

