/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

#include "log.h"

#include "internal/udt.h"

#include <core/config.h>
#define _LOG_TAG "log"
#include <libs/log.h>

static int log_info_v_0(lua_State *L);
static int log_warning_v_0(lua_State *L);
static int log_error_v_0(lua_State *L);
static int log_fatal_v_0(lua_State *L);

int log_loader(lua_State *L)
{
    return udt_newmodule(L,
        (const struct luaL_Reg[]){
            // -- operations --
            { "info", log_info_v_0 },
            { "warning", log_warning_v_0 },
            { "error", log_error_v_0 },
            { "fatal", log_fatal_v_0 },
            { NULL, NULL }
        },
        (const luaX_Const[]){
            { NULL, LUA_CT_NIL, { 0 } }
        });
}

static int _write(lua_State *L, Log_Levels_t level)
{
    int argc = lua_gettop(L);
    lua_getglobal(L, "tostring"); // F
    for (int i = 1; i <= argc; ++i) {
        lua_pushvalue(L, -1); // F -> F F
        lua_pushvalue(L, i); // F F -> F F I
        lua_call(L, 1, 1); // F F I -> F R
        const char *s = lua_tostring(L, -1);
        if (!s) {
            return luaL_error(L, "`tostring` must return a string `log_write`");
        }
        Log_write(level, _LOG_TAG, (i > 1) ? "\t%s" : "%s", s);
        lua_pop(L, 1); // F R -> F
    }
    lua_pop(L, 1); // F -> <empty>

    return 0;
}

static int log_info_v_0(lua_State *L)
{
    return _write(L, LOG_LEVELS_INFO);
}

static int log_warning_v_0(lua_State *L)
{
    return _write(L, LOG_LEVELS_WARNING);
}

static int log_error_v_0(lua_State *L)
{
    return _write(L, LOG_LEVELS_ERROR);
}

static int log_fatal_v_0(lua_State *L)
{
    return _write(L, LOG_LEVELS_FATAL);
}
