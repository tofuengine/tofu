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

#include "log.h"

#include "core/imath.h"
#include "core/luax.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH        320
#define SCREEN_HEIGHT       240
#define SCREEN_SCALE        0
#define WINDOW_TITLE        ".: Tofu Engine :."
#define FRAMES_PER_SECOND   60

void Configuration_initialize(Configuration_t *configuration)
{
    strncpy(configuration->title, WINDOW_TITLE, MAX_CONFIGURATION_TITLE_LENGTH);
    configuration->width = SCREEN_WIDTH;
    configuration->height = SCREEN_HEIGHT;
    configuration->scale = SCREEN_SCALE;
    configuration->fullscreen = false;
    configuration->update_fps = FRAMES_PER_SECOND;
    configuration->skippable_frames = FRAMES_PER_SECOND / 5; // About 20% of the FPS amount.
    configuration->render_fps = -1;
    configuration->hide_cursor = true;
    configuration->exit_key_enabled = true;
    configuration->debug = true;
}

void Configuration_parse(lua_State *L, Configuration_t *configuration)
{
    if (!lua_istable(L, -1)) {
        Log_write(LOG_LEVELS_WARNING, "<CONFIGURATION> setup method returned no value");
        return;
    }

    lua_pushnil(L); // first key
    while (lua_next(L, -2)) { // Table is at the top, prior pushing NIL!
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
        if (strcmp(key, "scale") == 0) {
            configuration->scale = lua_tointeger(L, -1);
        } else
        if (strcmp(key, "fullscreen") == 0) {
            configuration->fullscreen = lua_toboolean(L, -1);
        } else
        if (strcmp(key, "update_fps") == 0) {
            configuration->update_fps = lua_tointeger(L, -1);
            configuration->skippable_frames = configuration->update_fps / 5; // Keep synched. About 20% of the FPS amount.
        } else
        if (strcmp(key, "skippable-frames") == 0) {
            int suggested = configuration->update_fps / 5;
            configuration->skippable_frames = imin(lua_tointeger(L, -1), suggested); // TODO: not sure if `imin` or `imax`. :P
        } else
        if (strcmp(key, "render-fps") == 0) {
            configuration->render_fps = lua_tointeger(L, -1);
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
}
