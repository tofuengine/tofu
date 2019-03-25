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

#include "modules/collections.h"
#include "modules/events.h"
#include "modules/graphics.h"

const Module_Entry_t _modules[] = {
//  { "<module-name>", "<module-source>" }
    { "collections", collections_wren },
    { "events", events_wren },
    { "graphics", graphics_wren },
    { NULL, NULL }
};
const Class_Entry_t _classes[] = {
//  { "<module-name>", "<class-name>", <allocator>, <deallocator> }
    { "collections", "Grid", grid_allocate, grid_finalize },
    { "graphics", "Bank", graphics_bank_allocate, graphics_bank_finalize },
    { "graphics", "Font", graphics_font_allocate, graphics_font_finalize },
    { NULL, NULL, NULL, NULL }
};
const Method_Entry_t _methods[] = {
//  { "<module-name>", "<class-name>", <is-static>, "<signature>", <function> }
    { "collections", "Grid", false, "width", grid_width },
    { "collections", "Grid", false, "height", grid_height },
    { "collections", "Grid", false, "fill(_)", grid_fill },
    { "collections", "Grid", false, "row(_,_,_,_)", grid_row },
    { "collections", "Grid", false, "peek(_,_)", grid_peek },
    { "collections", "Grid", false, "poke(_,_,_)", grid_poke },
    { "events", "Input", true, "isKeyDown(_)", events_input_iskeydown },
    { "events", "Input", true, "isKeyUp(_)", events_input_iskeyup },
    { "events", "Input", true, "isKeyPressed(_)", events_input_iskeypressed },
    { "events", "Input", true, "isKeyReleased(_)", events_input_iskeyreleased },
    { "events", "Environment", true, "quit()", events_environment_quit },
    { "graphics", "Bank", false, "sprite(_,_,_,_,_,_)", graphics_bank_sprite },
    { "graphics", "Font", false, "text(_,_,_,_,_,_)", graphics_font_text },
    { "graphics", "Canvas", true, "width", graphics_canvas_width },
    { "graphics", "Canvas", true, "height", graphics_canvas_height },
    { "graphics", "Canvas", true, "palette(_)", graphics_canvas_palette },
    { "graphics", "Canvas", true, "point(_,_,_)", graphics_canvas_point },
    { "graphics", "Canvas", true, "polygon(_,_,_)", graphics_canvas_polygon },
    { "graphics", "Canvas", true, "circle(_,_,_,_,_)", graphics_canvas_circle },
    { NULL, NULL, false, NULL, NULL }
};
