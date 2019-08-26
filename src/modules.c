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

#include "modules.h"

#include "log.h"

#include "core/luax.h"
#include "modules/bank.h"
#include "modules/canvas.h"
#include "modules/class.h"
#include "modules/grid.h"
#include "modules/font.h"
#include "modules/input.h"
#include "modules/file.h"
#include "modules/system.h"
#include "modules/timer.h"

typedef struct _Module_t {
    const char *namespace;
    lua_CFunction loader;
} Module_t;

static const Module_t modules[] = {
    { "tofu.collections.Grid", grid_loader },
    { "tofu.core.System", system_loader },
    { "tofu.events.Input", input_loader },
    { "tofu.graphics.Bank", bank_loader },
    { "tofu.graphics.Canvas", canvas_loader },
    { "tofu.graphics.Font", font_loader },
    { "tofu.io.File", file_loader },
    { "tofu.util.Class", class_loader },
    { "tofu.util.Timer", timer_loader },
    { NULL, NULL }
};

void modules_initialize(lua_State *L, int nup)
{
//        luaL_requiref(L, modules[i].namespace, modules[i].loader, 1);
//        lua_pop(L, 1);  /* remove lib */
    for (int i = 0; modules[i].loader; ++i) {
        for (int j = 0; j < nup; ++j) {
            lua_pushvalue(L, -nup);
        }
        luaX_preload(L, modules[i].namespace, modules[i].loader, nup);
    }
    lua_pop(L, nup);
}
