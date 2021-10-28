/*
 * MIT License
 * 
 * Copyright (c) 2019-2021 Marco Lizza
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

#include "modules.h"

// Lua exposed C functions names are "mangled" according to this format
//
//   <function name>_<input arguments>_<return values>
//
// where `function name` is a generic identifier, `input arguments` and `return values` have this format
//
//   \d+[bBnNsStTuUfFoO]*
//
// The integer number indicates the amount of arguments/return-values and the sequence of characters encodes the
// types. Uppercase characters are used for *optional* when optional. The encoded types are the following:
//
//   b -> boolean
//   n -> number
//   s -> string
//   t -> table
//   u -> userdata
//   f -> function
//   o -> object (i.e. userdata with optionally encoded type)
//
// Example:
//
//   void blit_8onnnnNNN_0();
//   void cursor_1o_2nn();
//

#include <libs/log.h>
#include <libs/luax.h>

// FIXME: better namespace/naming usage for the modules? `arrays.h` -> `core_arrays.h`?
#include "modules/arrays.h"
#include "modules/bank.h"
#include "modules/batch.h"
#include "modules/body.h"
#include "modules/canvas.h"
#include "modules/class.h"
#include "modules/display.h"
#include "modules/file.h"
#include "modules/font.h"
#include "modules/grid.h"
#include "modules/input.h"
#include "modules/iterators.h"
#include "modules/log.h"
#include "modules/math.h"
#include "modules/noise.h"
#include "modules/palette.h"
#include "modules/program.h"
#include "modules/source.h"
#include "modules/speakers.h"
#include "modules/system.h"
#include "modules/timers.h"
#include "modules/tweener.h"
#include "modules/vector.h"
#include "modules/wave.h"
#include "modules/world.h"
#include "modules/xform.h"

#define LOG_CONTEXT "modules"

// TODO: http://www.ilikebigbits.com/2017_06_01_float_or_double.html

static int _create_module(lua_State *L, const luaL_Reg *classes)
{
    lua_newtable(L);
    for (const luaL_Reg *class = classes; class->func; ++class) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "initializing class `%s`", class->name);
        if (class->func(L) != 1) {
            return luaL_error(L, "can't initialize class `%s`", class->name);
        }
        lua_setfield(L, -2, class->name);
    }
    return 1;
}

static void _preload_modules(lua_State *L, int nup, const luaL_Reg *modules)
{
#ifdef INSIST
    luaX_insisttable(L, "tofu");
    for (const luaL_Reg *module = modules; module->func; ++module) {
        luaX_pushvalues(L, nup);
        luaX_require(L, module->name, module->func, nup, 1);
        lua_setfield(L, -1, module->name);
    }
    lua_pop(L, nup);
#else
    for (const luaL_Reg *module = modules; module->func; ++module) {
        Log_write(LOG_LEVELS_DEBUG, LOG_CONTEXT, "preloading module `%s`", module->name);
        luaX_pushvalues(L, nup);
        luaX_preload(L, module->name, module->func, nup);
    }
    lua_pop(L, nup);
#endif
}

static int core_loader(lua_State *L)
{
    return _create_module(L, (const luaL_Reg[]){
            { "Class", class_loader },
            { "Log", log_loader },
            { "Math", math_loader },
            { "System", system_loader },
            { NULL, NULL }
        });
}

static int events_loader(lua_State *L)
{
    return _create_module(L, (const luaL_Reg[]){
            { "Input", input_loader },
            { NULL, NULL }
        });
}

static int generators_loader(lua_State *L)
{
    return _create_module(L, (const luaL_Reg[]){
            { "Noise", noise_loader },
            { "Tweener", tweener_loader },
            { "Wave", wave_loader },
            { NULL, NULL }
        });
}

static int graphics_loader(lua_State *L)
{
    return _create_module(L, (const luaL_Reg[]){
            { "Bank", bank_loader },
            { "Batch", batch_loader },
            { "Canvas", canvas_loader },
            { "Display", display_loader },
            { "Font", font_loader },
            { "Palette", palette_loader },
            { "Program", program_loader },
            { "XForm", xform_loader },
            { NULL, NULL }
        });
}

static int io_loader(lua_State *L)
{
    return _create_module(L, (const luaL_Reg[]){
            { "File", file_loader },
            { NULL, NULL }
        });
}

static int physics_loader(lua_State *L)
{
    return _create_module(L, (const luaL_Reg[]){
            { "Body", body_loader },
            { "World", world_loader },
            { NULL, NULL }
        });
}

static int sound_loader(lua_State *L)
{
    return _create_module(L, (const luaL_Reg[]){
            { "Source", source_loader },
            { "Speakers", speakers_loader }, // FIXME: find a better name.
            { NULL, NULL }
        });
}

static int util_loader(lua_State *L)
{
    return _create_module(L, (const luaL_Reg[]){
            { "Arrays", arrays_loader },
            { "Grid", grid_loader },
            { "Iterators", iterators_loader },
            { "Vector", vector_loader },
            { NULL, NULL }
        });
}

void modules_initialize(lua_State *L, int nup)
{
    _preload_modules(L, nup, (const luaL_Reg[]){
            { "tofu.core", core_loader }, // TODO: core should be loaded for first?
            { "tofu.events", events_loader },
            { "tofu.generators", generators_loader },
            { "tofu.graphics", graphics_loader },
            { "tofu.io", io_loader },
            { "tofu.physics", physics_loader },
            { "tofu.sound", sound_loader },
            { "tofu.timers", timers_loader },
            { "tofu.util", util_loader },
            { NULL, NULL }
        });
}
