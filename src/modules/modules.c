/*
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

#include "modules.h"

// Lua exposed C functions names are "mangled" according to this format
//
//   <function name>_<input arguments>_<return values>
//
// where `function name` is a generic identifier, `input arguments` and `return values` have this format
//
//   \d+[bBnNsStTuUfFeEoO]*
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
//   e -> enum (i.e. a string from a list of possible ones)
//   o -> object (i.e. userdata with optionally encoded type)
//
// Example:
//
//   void blit_8onnnnNNN_0();
//   void cursor_1o_2nn();
//

// FIXME: better namespace/naming usage for the modules? `arrays.h` -> `core_arrays.h`?
#include "internal/udt.h"
#include "bank.h"
#include "batch.h"
#include "body.h"
#include "canvas.h"
#include "controller.h"
#include "cursor.h"
#include "display.h"
#include "file.h"
#include "font.h"
#include "grid.h"
#include "image.h"
#include "keyboard.h"
#include "log.h"
#include "math.h"
#include "noise.h"
#include "palette.h"
#include "program.h"
#include "source.h"
#include "speakers.h"
#include "storage.h"
#include "system.h"
#include "tweener.h"
#include "wave.h"
#include "world.h"
#include "xform.h"

#define _LOG_TAG "modules"
#include <libs/log.h>
#include <libs/luax.h>

void modules_initialize(lua_State *L, const void *userdatas[])
{
    udt_preload_modules(L, userdatas, (const luaL_Reg[]){
            { "tofu.core.log", log_loader },
            { "tofu.core.math", math_loader },
            { "tofu.core.system", system_loader },
            { "tofu.generators.noise", noise_loader },
            { "tofu.generators.tweener", tweener_loader },
            { "tofu.generators.wave", wave_loader },
            { "tofu.graphics.bank", bank_loader },
            { "tofu.graphics.batch", batch_loader },
            { "tofu.graphics.canvas", canvas_loader },
            { "tofu.graphics.display", display_loader },
            { "tofu.graphics.font", font_loader },
            { "tofu.graphics.image", image_loader },
            { "tofu.graphics.palette", palette_loader },
            { "tofu.graphics.program", program_loader },
            { "tofu.graphics.xform", xform_loader },
            { "tofu.input.controller", controller_loader },
            { "tofu.input.cursor", cursor_loader },
            { "tofu.input.keyboard", keyboard_loader },
            { "tofu.io.file", file_loader },
            { "tofu.io.storage", storage_loader },
            { "tofu.physics.body", body_loader },
            { "tofu.physics.world", world_loader },
            { "tofu.sound.source", source_loader },
            { "tofu.sound.speakers", speakers_loader }, // FIXME: find a better name.
            { "tofu.util.grid", grid_loader },
            { NULL, NULL }
        });
}
