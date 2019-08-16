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

#include "configuration.h"

#include "file.h"
#include "log.h"

#include "core/luax.h"

//#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240
#define WINDOW_TITLE    ".: Tofu Engine :."

typedef int (*luaEx_CFunction)(lua_State*, void*);

// This is a static (common) dispatching function that bounces to
// the intended callback function, passing the additional parameter.
static int luaEx_dispatcher(lua_State* state)
{
    void* parameter = (void*)lua_touserdata(state, lua_upvalueindex(1));
    luaEx_CFunction callback = (luaEx_CFunction)lua_touserdata(state, lua_upvalueindex(2));
    return callback(state, parameter);
}

// Register a new global [function], by creating a closure with the passed
// name bound to the common dispatcher, encapsulating the real function pointer
// and the passed parameter.
//
// We are using the light-userdata datatype since we are not going to need to
// interact with the garbage-collection for its management.
void luaEx_register(lua_State* state, const char* name, luaEx_CFunction function, void* parameter)
{
    lua_pushlightuserdata(state, parameter);
    lua_pushlightuserdata(state, function);
    lua_pushcclosure(state, luaEx_dispatcher, 2);
    lua_setglobal(state, name);
}

static int parse(lua_State *L, void *parameters)
{
    Configuration_t *configuration = (Configuration_t *)parameters;

    if (lua_gettop(L) != 1) {
        return luaL_error(L, "<INTERPRETER> function requires 1 argument");
    }
    if (!lua_istable(L, 1)) {
        return luaL_error(L, "<INTERPRETER> function requires a table as argument");
    }

    lua_pushnil(L); // first key
    while (lua_next(L, 1)) {
        const char *key = lua_tostring(L, -2); // uses 'key' (at index -2) and 'value' (at index -1)

        if (strcmp(key, "title") == 0) {
            strncpy(configuration->title, lua_tostring(L, -1), MAX_CONFIGURATION_TITLE_LENGTH);
        } else
        if (strcmp(key, "width") == 0) {
            configuration->width = lua_tointeger(L, -1);
        } else
        if (strcmp(key, "height") == 0) {
            configuration->height = lua_tointeger(L, -1);
        } else
        if (strcmp(key, "depth") == 0) {
            configuration->depth = lua_tointeger(L, -1);
        } else
        if (strcmp(key, "fullscreen") == 0) {
            configuration->fullscreen = lua_toboolean(L, -1);
        } else
#ifndef __NO_AUTOFIT__
        if (strcmp(key, "autofit") == 0) {
            configuration->autofit = lua_toboolean(L, -1);
        } else
#endif
        if (strcmp(key, "fps") == 0) {
            configuration->fps = lua_tointeger(L, -1);
        } else
        if (strcmp(key, "skippable_frames") == 0) {
            configuration->skippable_frames = lua_tointeger(L, -1);
        } else
        if (strcmp(key, "hide_cursor") == 0) {
            configuration->hide_cursor = lua_toboolean(L, -1);
        } else
        if (strcmp(key, "exit-key-enabled") == 0) {
            configuration->exit_key_enabled = lua_toboolean(L, -1);
        } else
        if (strcmp(key, "debug") == 0) {
            configuration->debug = lua_toboolean(L, -1);
        }

        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }

    return 1;
}

void Configuration_initialize(Configuration_t *configuration)
{
    strncpy(configuration->title, WINDOW_TITLE, MAX_CONFIGURATION_TITLE_LENGTH);
    configuration->width = SCREEN_WIDTH;
    configuration->height = SCREEN_HEIGHT;
    configuration->depth = 8;
    configuration->fullscreen = false;
    configuration->autofit = true;
    configuration->fps = 60;
    configuration->skippable_frames = 12; // About 20% of the FTP amount.
    configuration->hide_cursor = true;
    configuration->exit_key_enabled = true;
    configuration->debug = true;
}

void Configuration_load(Configuration_t *configuration, const char *base_path)
{
    lua_State *L = luaL_newstate();
    if (!L) {
        Log_write(LOG_LEVELS_FATAL, "<CONFIGURATION> can't initialize interpreter");
        return;
    }
	luaL_openlibs(L);

    luaX_appendpath(L, base_path);

    luaEx_register(L, "parse", parse, configuration);
    int result = luaL_dostring(L, "parse(require(\"configuration\"))\n");
    if (!result) {
        Log_write(LOG_LEVELS_FATAL, "<CONFIGURATION> can't parse configuration file");
    }

    lua_close(L);
}
